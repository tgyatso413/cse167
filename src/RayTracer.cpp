#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL

#include "RayTracer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "./scenes/cornell_box.inl"
#include "./scenes/open_sky.inl"
#include "./scenes/sphere_in_box.inl"
#include "./scenes/teapot_in_box.inl"
#include "Image.h"
#include "ProgressBar.h"
#include "Ray.h"
#include "Scene.h"
#include "Sphere.h"
#include "Utility.h"

using namespace glm;

namespace {
constexpr int kMinTileSize = 8;
constexpr int kMaxTileSize = 64;
constexpr int kTargetTilesPerThread = 8;
constexpr uint32_t kBaseSeed = 0xC0FFEEu;
constexpr std::chrono::milliseconds kSnapshotInterval(500);

int clamp_tile_size(int tile_size) {
    return std::max(kMinTileSize, std::min(tile_size, kMaxTileSize));
}

int snap_tile_size_bucket(int tile_size) {
    static const int kBuckets[] = {8, 12, 16, 24, 32, 48, 64};
    int clamped = clamp_tile_size(tile_size);
    int best = kBuckets[0];
    int best_distance = std::abs(clamped - best);
    for (int bucket : kBuckets) {
        int distance = std::abs(clamped - bucket);
        if (distance < best_distance) {
            best = bucket;
            best_distance = distance;
        }
    }
    return best;
}

int parse_tile_size_override() {
    const char* env_tile_size = std::getenv("RAYTRACER_TILE_SIZE");
    if (!env_tile_size || env_tile_size[0] == '\0')
        return 0;

    char* end = nullptr;
    long parsed = std::strtol(env_tile_size, &end, 10);
    if (end == env_tile_size || *end != '\0' || parsed <= 0) {
        std::cerr << "Ignoring invalid RAYTRACER_TILE_SIZE='" << env_tile_size << "'" << std::endl;
        return 0;
    }

    return clamp_tile_size(static_cast<int>(parsed));
}

int choose_tile_size(int width, int height, unsigned int num_threads) {
    int override_tile_size = parse_tile_size_override();
    if (override_tile_size > 0)
        return override_tile_size;

    // Heuristic: keep enough tiles in flight (~8/thread) while limiting lock/queue overhead.
    double pixel_count = std::max(1.0, static_cast<double>(width) * static_cast<double>(height));
    double target_tile_count =
        std::max(1.0, static_cast<double>(num_threads) * static_cast<double>(kTargetTilesPerThread));
    double ideal_tile_area = pixel_count / target_tile_count;
    double ideal_tile_side = std::sqrt(std::max(1.0, ideal_tile_area));
    return snap_tile_size_bucket(static_cast<int>(std::lround(ideal_tile_side)));
}
}  // namespace

void RayTracer::init(int scene_id) {
    // Initialize the scene
    if (scene_id == 1)
        scene = std::unique_ptr<Scene>(cornell_box());
    else if (scene_id == 2)
        scene = std::unique_ptr<Scene>(cornell_box_mirror());
    else if (scene_id == 3)
        scene = std::unique_ptr<Scene>(teapot_in_box());
    else if (scene_id == 4)
        scene = std::unique_ptr<Scene>(open_sky());
    else {
        std::cout << "Invalid <scene_id> " << scene_id << " given (expected 1..4)" << std::endl;
        exit(1);
    }
}

Ray RayTracer::ray_thru_pixel(int i, int j) {
    /**
     * This function generated a ray passing through camera origin
     * and pixel (i, j)
     */

    Ray ray;
    ray.pixel_x_coordinate = i;
    ray.pixel_y_coordinate = j;

    // p0
    ray.p0 = glm::vec3(camera.eye);

    /**
     * TODO: Task 3
     * Randomly sample x and y inside pixel(i, j) using `rand_uniform(0.0f, 1.0f)`
     */
    float x = 0.5f;
    float y = 0.5f;

    /**
     * TODO: Task 1.1
     * calculate and assign direction to ray which is passoing
     * through current pixel (i, j)
     */
    float alpha = 0.0f;  // TODO: Implement this
    float beta = 0.0f;   // TODO: Implement this

    vec3 u(camera.cameraMatrix[0]);
    vec3 v(camera.cameraMatrix[1]);
    vec3 w(camera.cameraMatrix[2]);

    ray.dir = vec3(-1.0f);  // TODO: Implement this

    return ray;
}

void RayTracer::set_shading_mode(ShadingMode shading_mode) {
    // Update shading mode for both ray tracer and scene
    this->shading_mode = shading_mode;
    this->scene->shading_mode = shading_mode;
}

// --------------------- Main Draw Function ---------------------
void RayTracer::draw(const std::function<void()>& on_snapshot) {
    image.updateColor(glm::vec3(0.0f));
    camera.computeMatrices();

    if (this->shading_mode == ShadingMode::NORMAL) {
        std::cerr << "RayTracer::draw skipped in NORMAL mode; raster preview handles normal shading." << std::endl;
        return;
    }

    unsigned int samples_per_pixel_u = static_cast<unsigned int>(std::max(1, samples_per_pixel));
    int max_bounces_i = std::max(1, max_bounces);
    unsigned int total_samples_count =
        static_cast<unsigned int>(samples_per_pixel_u * camera.height * camera.width);
    if (total_samples_count == 0) {
        std::cerr << "RayTracer::draw skipped: no samples to render." << std::endl;
        return;
    }

    std::atomic<unsigned int> processed_sample_count(0);
    std::atomic<unsigned long long> processed_bounce_count(0);
    ProgressBar bar(total_samples_count);

    unsigned int num_threads = std::max(1u, std::thread::hardware_concurrency());
    int tile_size = choose_tile_size(camera.width, camera.height, num_threads);
    int tiles_x = (camera.width + tile_size - 1) / tile_size;
    int tiles_y = (camera.height + tile_size - 1) / tile_size;
    int total_tiles = tiles_x * tiles_y;

    std::atomic<int> next_tile_id(0);
    std::atomic<unsigned int> active_workers(num_threads);
    std::mutex imageMutex;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    auto trace_start = std::chrono::steady_clock::now();
    auto worker_callback = [&]() {
        while (true) {
            int tile_id = next_tile_id.fetch_add(1, std::memory_order_relaxed);
            if (tile_id >= total_tiles)
                break;

            seed_thread_rng(kBaseSeed ^ hash_u32(static_cast<uint32_t>(tile_id + 1)));

            int tile_x = tile_id % tiles_x;
            int tile_y = tile_id / tiles_x;
            int x_start = tile_x * tile_size;
            int y_start = tile_y * tile_size;
            int x_end = std::min(x_start + tile_size, camera.width);
            int y_end = std::min(y_start + tile_size, camera.height);
            int tile_width = x_end - x_start;
            int tile_height = y_end - y_start;

            std::vector<glm::vec3> tile_colors(tile_width * tile_height, glm::vec3(0.0f));
            unsigned int local_processed_samples = 0;
            unsigned long long local_processed_bounces = 0;

            for (int j = y_start; j < y_end; j++) {
                for (int i = x_start; i < x_end; i++) {
                    // Accumulate linear radiance here; display transform is applied in Image::draw().
                    glm::vec3 pixel_color(0.0f);

                    for (unsigned int s = 0; s < samples_per_pixel_u; s++) {
                        Ray ray = ray_thru_pixel(i, j);

                        while (true) {
                            ray = scene->intersect(ray);
                            glm::vec3 ray_color = (this->shading_mode == ShadingMode::DEBUG) ? ray.debug_color : ray.color;
                            pixel_color += ray_color / static_cast<float>(samples_per_pixel_u);  // TODO#: Is this correct? the bounce contribution is just getting added.
                            local_processed_bounces++;

                            if (!(ray.n_bounces < max_bounces_i && !ray.terminate))
                                break;
                        }
                        local_processed_samples++;
                    }

                    tile_colors[(j - y_start) * tile_width + (i - x_start)] = pixel_color;
                }
            }

            {
                std::lock_guard<std::mutex> image_lock(imageMutex);
                for (int j = y_start; j < y_end; j++)
                    for (int i = x_start; i < x_end; i++)
                        image.updatePixel(i, j, tile_colors[(j - y_start) * tile_width + (i - x_start)]);
            }

            processed_sample_count.fetch_add(local_processed_samples, std::memory_order_relaxed);
            processed_bounce_count.fetch_add(local_processed_bounces, std::memory_order_relaxed);
        }

        active_workers.fetch_sub(1, std::memory_order_relaxed);
    };

    for (unsigned int i = 0; i < num_threads; i++)
        threads.emplace_back(worker_callback);

    unsigned int frame_update_count = 0;
    while (active_workers.load(std::memory_order_relaxed) > 0) {
        std::this_thread::sleep_for(kSnapshotInterval);
        if (on_snapshot) {
            std::lock_guard<std::mutex> image_lock(imageMutex);
            on_snapshot();
        }
        if (frame_update_count % 4 == 0)
            bar.update(processed_sample_count.load(std::memory_order_relaxed));
        frame_update_count++;
    }

    for (auto& thread : threads)
        thread.join();
    auto trace_end = std::chrono::steady_clock::now();
    double render_seconds = std::chrono::duration<double>(trace_end - trace_start).count();

    bar.update(processed_sample_count.load(std::memory_order_relaxed));
    if (on_snapshot) {
        std::lock_guard<std::mutex> image_lock(imageMutex);
        on_snapshot();
    }

    std::cout << std::endl
              << "Done! " << processed_sample_count.load(std::memory_order_relaxed) << " samples processed" << std::endl
              << "Bounces traced: " << processed_bounce_count.load(std::memory_order_relaxed) << std::endl
              << "Tile size: " << tile_size << std::endl
              << "Trace time: " << render_seconds << " seconds" << std::endl;
}
