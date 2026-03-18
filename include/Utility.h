#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <random>

// Scene-authored literal colors are treated as sRGB and converted once to linear
// before any lighting math.
static glm::vec3 SRGBToLinear(glm::vec3 color) {
    glm::vec3 srgb = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
    auto eotf = [](float c) {
        return (c <= 0.04045f)
                   ? (c / 12.92f)
                   : static_cast<float>(std::pow((c + 0.055f) / 1.055f, 2.4f));
    };
    return glm::vec3(eotf(srgb.x), eotf(srgb.y), eotf(srgb.z));
}

// Convert linear light to sRGB for display/output encoding.
static glm::vec3 LinearToSRGB(glm::vec3 color) {
    glm::vec3 linear = glm::max(color, glm::vec3(0.0f));
    auto oetf = [](float c) {
        return (c <= 0.0031308f)
                   ? (12.92f * c)
                   : (1.055f * static_cast<float>(std::pow(c, 1.0f / 2.4f)) - 0.055f);
    };
    return glm::vec3(oetf(linear.x), oetf(linear.y), oetf(linear.z));
}

// Backwards-compatible alias while call sites migrate to SRGBToLinear.
static glm::vec3 RGB_to_Linear(glm::vec3 color) {
    return SRGBToLinear(color);
}

static float degree_to_rad(float degrees) {
    constexpr float kPi = 3.14159265358979323846f;
    return degrees * kPi / 180.0f;
}

static std::mt19937 &thread_rng_engine() {
    static thread_local std::mt19937 engine(0u);
    return engine;
}

static void seed_thread_rng(uint32_t seed) {
    thread_rng_engine().seed(seed);
}

static uint32_t hash_u32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static float rand01() {
    return std::generate_canonical<float, 24>(thread_rng_engine());
}

static float rand_uniform(float min, float max) {
    return min + (max - min) * rand01();
}

static glm::vec3 rand_uniform_vec3(glm::vec3 min, glm::vec3 max) {
    return glm::vec3(rand_uniform(min.x, max.x),
                     rand_uniform(min.y, max.y),
                     rand_uniform(min.z, max.z));
}

static glm::vec3 align_with_normal(const glm::vec3& v, const glm::vec3& normal) {
    using namespace glm;
    // Build an orthonormal basis(tangent, bitangent, normal)
    vec3 up = (abs(normal.y) < 0.999f) ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(up, normal));
    vec3 bitangent = cross(normal, tangent);

    return normalize(v.x * tangent + v.y * normal + v.z * bitangent);
}

#endif
