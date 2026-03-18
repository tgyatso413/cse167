#include <stdlib.h>

#include <cctype>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "Image.h"
#include "PlatformGL.h"
#include "RasterPreviewRenderer.h"
#include "RayTracer.h"
#include "ScenePreviewMesh.h"
#include "Screenshot.h"

static const int width = 800;
static const int height = 600;
static const char *title = "Ray Tracer";
static const glm::vec4 background(0.1f, 0.2f, 0.3f, 1.0f);

static Image image(width, height);
static RayTracer rayTracer(width, height, image);
static RasterPreviewRenderer preview_renderer;

enum class DisplayMode {
    PREVIEW_RASTER,
    RAYTRACE_PENDING,
    RAYTRACE_IMAGE,
};

struct RenderSaveState {
    std::string session_id;
    std::string session_dir;
    int next_run_id = 1;
    int pending_run_id = 0;
    int active_run_id = 0;
    int next_render_index = 1;
    int samples_per_pixel = 1;
    int max_bounces = 1;
    int scene_id = 1;
    bool session_ready = false;
};

static DisplayMode display_mode = DisplayMode::PREVIEW_RASTER;
static RenderSaveState render_save_state;

void printHelp() {
    std::cout << R"(
    Available commands:
      press 'H' to print this message again.
      press Esc to quit.

      Camera Controls:
      press 'O' to save a screenshot.
      press 'W'/'S' for front/back movement.
      press 'A'/'D' for left/right movement.
      press 'Q'/'E' for up/down movement.
      press the arrow keys to rotate camera.
      press 'Z'/'X' to rotate aroud view axis.
      press '-'/'+' to zoom (change of Fovy).
      press 'R' to reset camera.

      Shading Mode:
      press Spacebar to Ray Trace.
      press 'N' for Normal Shading.
      press 'P' for Debug Mode.

      Scene IDs:
      1 = Cornell box (all diffuse)
      2 = Cornell box mirror (mirror sphere)
      3 = Teapot in box
      4 = Open sky

)";
}

bool is_integer(const std::string &str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-' && c != '+') return false;
    }
    return true;
}

std::string zero_pad(int value, int width) {
    std::ostringstream oss;
    oss << std::setw(width) << std::setfill('0') << value;
    return oss.str();
}

std::string sanitize_token(const std::string &token) {
    std::string out;
    out.reserve(token.size());
    for (char c : token) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_')
            out.push_back(c);
        else
            out.push_back('-');
    }
    if (out.empty())
        return "na";
    return out;
}

std::string timestamp_string(const char *format) {
    std::time_t now = std::time(nullptr);
    std::tm local_tm;

#ifdef _WIN32
    if (localtime_s(&local_tm, &now) != 0)
#else
    if (localtime_r(&now, &local_tm) == nullptr)
#endif
        return "00000000_000000";

    std::ostringstream oss;
    oss << std::put_time(&local_tm, format);
    return oss.str();
}

bool ensure_directory_recursive(const std::string &path) {
    if (path.empty())
        return false;

    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec)
        return std::filesystem::is_directory(path);
    return std::filesystem::is_directory(path);
}

long long current_process_id() {
#ifdef _WIN32
    return static_cast<long long>(_getpid());
#else
    return static_cast<long long>(getpid());
#endif
}

std::string make_session_id(int samples_per_pixel, int max_bounces, int scene_id) {
    std::ostringstream oss;
    oss << "session_" << timestamp_string("%Y%m%d_%H%M%S")
        << "_" << current_process_id()
        << "__spp-" << sanitize_token(std::to_string(samples_per_pixel))
        << "_bounces-" << sanitize_token(std::to_string(max_bounces))
        << "_scene-" << sanitize_token(std::to_string(scene_id));
    return oss.str();
}

void initialize_render_save_state(int samples_per_pixel, int max_bounces, int scene_id) {
    render_save_state.samples_per_pixel = samples_per_pixel;
    render_save_state.max_bounces = max_bounces;
    render_save_state.scene_id = scene_id;
    render_save_state.next_run_id = 1;
    render_save_state.pending_run_id = 0;
    render_save_state.active_run_id = 0;
    render_save_state.next_render_index = 1;

    render_save_state.session_id = make_session_id(samples_per_pixel, max_bounces, scene_id);
    render_save_state.session_dir = "renders/" + render_save_state.session_id;

    if (!ensure_directory_recursive("renders") || !ensure_directory_recursive(render_save_state.session_dir)) {
        render_save_state.session_ready = false;
        std::cerr << "Warning: could not create render output directory at "
                  << render_save_state.session_dir << std::endl;
        return;
    }

    render_save_state.session_ready = true;
    std::cout << "Render session directory: " << render_save_state.session_dir << std::endl;
}

BOOL screenshot_topdown_for_display_mode(DisplayMode mode) {
    // Raytrace output is vertically flipped during Image::draw() blit; preview output is not.
    if (mode == DisplayMode::PREVIEW_RASTER)
        return FALSE;
    return TRUE;
}

void save_framebuffer_to_path(const std::string &path, BOOL topdown) {
    int currentwidth = glutGet(GLUT_WINDOW_WIDTH);
    int currentheight = glutGet(GLUT_WINDOW_HEIGHT);
    Screenshot imag = Screenshot(currentwidth, currentheight);
    imag.save(path.c_str(), topdown);
}

std::string build_render_filename(int run_id, int render_index) {
    std::ostringstream oss;
    oss << "run-" << zero_pad(run_id, 4)
        << "__render-" << zero_pad(render_index, 4)
        << ".png";
    return oss.str();
}

void save_numbered_render() {
    if (!render_save_state.session_ready) {
        std::cerr << "Warning: render session folder is unavailable, skipping numbered save." << std::endl;
        return;
    }

    if (render_save_state.active_run_id <= 0) {
        std::cerr << "Warning: no active run id available for numbered screenshot." << std::endl;
        return;
    }

    std::string filename = build_render_filename(render_save_state.active_run_id,
                                                 render_save_state.next_render_index);
    std::string output_path = render_save_state.session_dir + "/" + filename;
    save_framebuffer_to_path(output_path, screenshot_topdown_for_display_mode(display_mode));
    render_save_state.next_render_index++;
}

void draw_raytrace_image() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    image.draw();
    glutSwapBuffers();
    glFlush();
}

void draw_preview_frame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    rayTracer.camera.computeMatrices();
    preview_renderer.draw(rayTracer.camera);
    glutSwapBuffers();
    glFlush();
}

void begin_preview_mode() {
    rayTracer.set_shading_mode(ShadingMode::NORMAL);
    display_mode = DisplayMode::PREVIEW_RASTER;
}

void begin_raytrace_mode(ShadingMode mode) {
    rayTracer.set_shading_mode(mode);

    if (display_mode != DisplayMode::RAYTRACE_PENDING || render_save_state.pending_run_id <= 0)
        render_save_state.pending_run_id = render_save_state.next_run_id++;

    display_mode = DisplayMode::RAYTRACE_PENDING;
}

void initialize(int argc, char **argv) {
    (void)argc;

    printHelp();
    glClearColor(background[0], background[1], background[2], background[3]);
    glViewport(0, 0, width, height);

    int samples_per_pixel = std::atoi(argv[1]);
    int max_bounces = std::atoi(argv[2]);
    int scene_id = std::atoi(argv[3]);

    image.init();
    rayTracer.samples_per_pixel = samples_per_pixel;
    rayTracer.max_bounces = max_bounces;
    rayTracer.init(scene_id);

    initialize_render_save_state(samples_per_pixel, max_bounces, scene_id);

    PreviewMeshData preview_mesh = build_preview_mesh(*rayTracer.scene);
    if (!preview_renderer.init(preview_mesh)) {
        std::cerr << "Failed to initialize raster preview renderer" << std::endl;
        exit(1);
    }

    glEnable(GL_DEPTH_TEST);
}

void display(void) {
    if (display_mode == DisplayMode::PREVIEW_RASTER) {
        draw_preview_frame();
        return;
    }

    if (display_mode == DisplayMode::RAYTRACE_PENDING) {
        if (render_save_state.pending_run_id <= 0)
            render_save_state.pending_run_id = render_save_state.next_run_id++;

        render_save_state.active_run_id = render_save_state.pending_run_id;
        render_save_state.pending_run_id = 0;

        rayTracer.draw([]() {
            draw_raytrace_image();
        });

        display_mode = DisplayMode::RAYTRACE_IMAGE;
        draw_raytrace_image();
        save_numbered_render();
        return;
    }

    draw_raytrace_image();
}

void save_manual_screenshot() {
    if (render_save_state.active_run_id <= 0) {
        save_framebuffer_to_path("test.png", screenshot_topdown_for_display_mode(display_mode));
        return;
    }

    save_numbered_render();
}

void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    switch (key) {
        case 27:
            preview_renderer.shutdown();
            exit(0);
            break;
        case 'h':
            printHelp();
            break;
        case 'o':
            save_manual_screenshot();
            break;

        case 'w':
            begin_preview_mode();
            rayTracer.camera.moveForward(0.2f);
            glutPostRedisplay();
            break;
        case 's':
            begin_preview_mode();
            rayTracer.camera.moveForward(-0.2f);
            glutPostRedisplay();
            break;
        case 'a':
            begin_preview_mode();
            rayTracer.camera.moveRight(-0.2f);
            glutPostRedisplay();
            break;
        case 'd':
            begin_preview_mode();
            rayTracer.camera.moveRight(0.2f);
            glutPostRedisplay();
            break;
        case 'q':
            begin_preview_mode();
            rayTracer.camera.moveUp(-0.2f);
            glutPostRedisplay();
            break;
        case 'e':
            begin_preview_mode();
            rayTracer.camera.moveUp(0.2f);
            glutPostRedisplay();
            break;

        case 'z':
            begin_preview_mode();
            rayTracer.camera.rotateRoll(10.0f);
            glutPostRedisplay();
            break;
        case 'x':
            begin_preview_mode();
            rayTracer.camera.rotateRoll(-10.0f);
            glutPostRedisplay();
            break;

        case '-':
            begin_preview_mode();
            rayTracer.camera.zoom(0.1f);
            glutPostRedisplay();
            break;
        case '+':
            begin_preview_mode();
            rayTracer.camera.zoom(-0.1f);
            glutPostRedisplay();
            break;

        case 'r':
            begin_preview_mode();
            rayTracer.camera.reset();
            glutPostRedisplay();
            break;

        case ' ':
            begin_raytrace_mode(ShadingMode::RAY_TRACE);
            glutPostRedisplay();
            break;
        case 'n':
            begin_preview_mode();
            glutPostRedisplay();
            break;
        case 'p':
            begin_raytrace_mode(ShadingMode::DEBUG);
            glutPostRedisplay();
            break;
        default:
            glutPostRedisplay();
            break;
    }
}

void specialKey(int key, int x, int y) {
    (void)x;
    (void)y;

    begin_preview_mode();
    switch (key) {
        case GLUT_KEY_UP:
            rayTracer.camera.rotateUp(5.0f);
            glutPostRedisplay();
            break;
        case GLUT_KEY_DOWN:
            rayTracer.camera.rotateUp(-5.0f);
            glutPostRedisplay();
            break;
        case GLUT_KEY_RIGHT:
            rayTracer.camera.rotateRight(-5.0f);
            glutPostRedisplay();
            break;
        case GLUT_KEY_LEFT:
            rayTracer.camera.rotateRight(5.0f);
            glutPostRedisplay();
            break;
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <int: samples_per_pixel> <int: max_bounces> <int: scene_id>" << std::endl;
        std::cerr << "Scene IDs: 1=Cornell box, 2=Cornell box mirror, 3=Teapot in box, 4=Open sky" << std::endl;
        return 1;
    }
    for (int i = 1; i < 4; ++i) {
        if (!is_integer(argv[i])) {
            std::cerr << "Error: Argument " << i << " ('" << argv[i] << "') is not a valid integer." << std::endl;
            return 1;
        }
    }

    glutInit(&argc, argv);

#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
#else
    // glutInitContextVersion(3, 1);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
#endif
    glutInitWindowSize(width, height);
    glutCreateWindow(title);
#ifndef __APPLE__
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }
#endif
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    initialize(argc, argv);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);

    glutMainLoop();
    return 0;
}
