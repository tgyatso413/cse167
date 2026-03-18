#define GLM_ENABLE_EXPERIMENTAL
#include "MirrorMaterial.h"

#include <glm/glm.hpp>
#include <iostream>
#include <mutex>

using namespace glm;

namespace {
constexpr float kRayEpsilon = 0.001f;
}  // namespace

Ray MirrorMaterial::sample_ray_and_update_radiance(Ray& ray, Intersection& intersection) {
    /**
     * Calculate the next ray after intersection with the model.
     * This will be used for recursive ray tracing.
     */

    vec3 normal = intersection.normal;
    vec3 point = intersection.point;

    // Specular Reflection

    // Step 1: Calculate reflection direction
    /**
     * TODO(Task 6.2):
     * Calculate the perfect mirror reflection direction.
     */
    vec3 reflection_dir = vec3(0.0f);  // TODO: update reflection direction

    // Step 2: Calculate radiance
    /**
     * TODO(Task 6.2):
     * Calculate throughput update for mirror bounce.
     * Note:
     * - C_specular = `this->reflectance`
     */
    vec3 W_specular = vec3(0.0f);  // TODO: throughput multiplier for current bounce

    ray.W_wip = ray.W_wip * W_specular;
    ray.p0 = point + kRayEpsilon * normal;
    ray.dir = reflection_dir;
    ray.allow_emissive_hit = true;  // specular continuation can hit emitters
    ray.n_bounces++;
    return ray;
}

glm::vec3 MirrorMaterial::color_of_last_bounce(Ray& ray, Intersection& intersection, Scene const& scene) {
    // TODO: once mirror transport is implemented, return the intended mirror last-bounce contribution.
    (void)ray;
    (void)scene;
    return 0.4f * normalize(intersection.normal) + vec3(0.6f);
}
