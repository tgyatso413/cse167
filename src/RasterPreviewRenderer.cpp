#include "RasterPreviewRenderer.h"

#include "PlatformGL.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>

namespace {
#ifdef __APPLE__
const char* kVertexShaderSource = R"GLSL(
#version 150

in vec3 aPosition;
in vec3 aNormal;

uniform mat4 uViewProj;

out vec3 vNormal;

void main() {
    gl_Position = uViewProj * vec4(aPosition, 1.0);
    vNormal = normalize(aNormal);
}
)GLSL";

const char* kFragmentShaderSource = R"GLSL(
#version 150

in vec3 vNormal;
out vec4 fragColor;

void main() {
    vec3 normal_color = 0.4 * normalize(vNormal) + 0.6;
    fragColor = vec4(normal_color, 1.0);
}
)GLSL";
#else
const char* kVertexShaderSource = R"GLSL(
#version 140

in vec3 aPosition;
in vec3 aNormal;

uniform mat4 uViewProj;

out vec3 vNormal;

void main() {
    gl_Position = uViewProj * vec4(aPosition, 1.0);
    vNormal = normalize(aNormal);
}
)GLSL";

const char* kFragmentShaderSource = R"GLSL(
#version 140

in vec3 vNormal;
out vec4 fragColor;

void main() {
    vec3 normal_color = 0.4 * normalize(vNormal) + 0.6;
    fragColor = vec4(normal_color, 1.0);
}
)GLSL";
#endif

unsigned int compile_shader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        int log_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        std::string log;
        log.resize(std::max(1, log_len));
        glGetShaderInfoLog(shader, log_len, nullptr, &log[0]);
        std::cerr << "Shader compilation failed: " << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int create_program(const char* vertex_src, const char* fragment_src) {
    unsigned int vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_src);
    if (!vertex_shader)
        return 0;

    unsigned int fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_src);
    if (!fragment_shader) {
        glDeleteShader(vertex_shader);
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        int log_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
        std::string log;
        log.resize(std::max(1, log_len));
        glGetProgramInfoLog(program, log_len, nullptr, &log[0]);
        std::cerr << "Shader link failed: " << log << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}
}  // namespace

bool RasterPreviewRenderer::init(const PreviewMeshData& mesh) {
    shutdown();

    program_ = create_program(kVertexShaderSource, kFragmentShaderSource);
    if (!program_)
        return false;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(PreviewVertex)),
                 mesh.vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                 mesh.indices.data(),
                 GL_STATIC_DRAW);

    int pos_loc = glGetAttribLocation(program_, "aPosition");
    int normal_loc = glGetAttribLocation(program_, "aNormal");
    if (pos_loc < 0 || normal_loc < 0) {
        std::cerr << "RasterPreviewRenderer: shader attributes not found." << std::endl;
        glBindVertexArray(0);
        shutdown();
        return false;
    }

    glEnableVertexAttribArray(static_cast<unsigned int>(pos_loc));
    glVertexAttribPointer(static_cast<unsigned int>(pos_loc),
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(PreviewVertex),
                          reinterpret_cast<void*>(offsetof(PreviewVertex, position)));

    glEnableVertexAttribArray(static_cast<unsigned int>(normal_loc));
    glVertexAttribPointer(static_cast<unsigned int>(normal_loc),
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(PreviewVertex),
                          reinterpret_cast<void*>(offsetof(PreviewVertex, normal)));

    glBindVertexArray(0);

    index_count_ = static_cast<int>(mesh.indices.size());
    return true;
}

void RasterPreviewRenderer::draw(const Camera& camera) {
    if (!program_ || !vao_ || index_count_ <= 0)
        return;

    glm::mat4 view = camera.view_matrix();
    glm::mat4 projection = camera.projection_matrix();
    glm::mat4 view_proj = projection * view;

    glUseProgram(program_);
    int view_proj_loc = glGetUniformLocation(program_, "uViewProj");
    if (view_proj_loc >= 0)
        glUniformMatrix4fv(view_proj_loc, 1, GL_FALSE, glm::value_ptr(view_proj));

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glUseProgram(0);
}

void RasterPreviewRenderer::shutdown() {
    if (ebo_) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (program_) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    index_count_ = 0;
}
