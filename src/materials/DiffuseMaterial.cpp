#define GLM_ENABLE_EXPERIMENTAL
#include "DiffuseMaterial.h"

#include <cmath>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>
#include <mutex>

#include "ModelBase.h"
#include "Scene.h"

using namespace glm;

namespace {
constexpr float kRayEpsilon = 0.001f;
constexpr int kDirectLightSamples = 8;
constexpr float kPi = 3.14159265358979323846f;
}  // namespace

Ray DiffuseMaterial::sample_ray_and_update_radiance(Ray& ray, Intersection& intersection) {
    /**
     * Calculate the next ray after intersection with the model.
     * This will be used for recursive ray tracing.
     */

    vec3 normal = intersection.normal;
    vec3 point = intersection.point;

    // Diffuse reflection

    // Step 1: Sample ray direction
    /**
     * TODO(Task 6.1):
     * Implement cosine-weighted hemisphere sampling.
     */
    float s = rand01();
    float t = rand01();

    // TODO: Update u, v based on Equation (8) in handout.
    float u = 0.0f;
    float v = 0.0f;

    vec3 hemisphere_sample = vec3(0.0f);  // TODO: Create cosine-weighted sampled local direction

    // The sampled direction above is in local coordinates.
    // Align it with the surface normal before updating the ray.
    vec3 new_dir = align_with_normal(hemisphere_sample, normal);

    // Step 2: Calculate radiance
    /**
     * TODO(Task 6.1):
     * Calculate throughput update for diffuse bounce.
     * Note:
     * - C_diffuse = `this->albedo`
     */
    vec3 W_diffuse = vec3(0.0f);  // TODO: throughput multiplier for current bounce

    ray.W_wip = ray.W_wip * W_diffuse;
    ray.p0 = point + kRayEpsilon * normal;
    ray.dir = new_dir;
    ray.allow_emissive_hit = false;  // old `is_diffuse_bounce=true` policy mapping
    ray.n_bounces++;

    return ray;
}

glm::vec3 DiffuseMaterial::get_direct_lighting(Intersection const& intersection, Scene const& scene) const {
    using namespace glm;

    /**
     * Note:
     * - Light sources from scene can be accessed by `scene.light_sources`
     * - Visibility checks should use nearest-hit queries (`scene.intersect_nearest`)
     */

    // Iterate over all light sources.
    vec3 cumulative_direct_light = vec3(0.0f);
    for (unsigned int idx = 0; idx < scene.light_sources.size(); idx++) {
        ModelBase* light_source = scene.light_sources[idx];

        // Intersection could itself be on one light source, so skip self.
        if (light_source == intersection.model)
            continue;

        vec3 summed_samples = vec3(0.0f);
        for (int sample_idx = 0; sample_idx < kDirectLightSamples; sample_idx++) {
            // Get sampled light position on emissive geometry.
            vec3 light_pos = light_source->get_surface_point();
            vec3 to_light = light_pos - intersection.point;
            float dist2 = dot(to_light, to_light);
            if (dist2 <= 0.0f)
                continue;

            float dist = std::sqrt(dist2);
            vec3 light_dir = to_light / dist;

            float c_x = std::max(dot(intersection.normal, light_dir), 0.0f);
            if (c_x <= 0.0f)
                continue;

            // Shoot a shadow ray towards light source.
            Ray shadow_ray;
            shadow_ray.p0 = vec3(0.0f);   // TODO: offset from intersection point by epsilon along normal
            shadow_ray.dir = vec3(0.0f);  // TODO: direction from shadow_ray.p0 to light_pos

            // TODO(Task 4.1): visibility check using nearest hit.
            Intersection shadow_hit;
            bool is_visible = false;  // TODO: replace with intersect_nearest-based visibility logic

            // TODO(Task 4.1): area-light normal term and emitted radiance.
            float c_y = 0.0f;                // TODO: cosine term at light sample
            vec3 emitted_radiance(0.0f);     // TODO: derive from light_source->material->emission
            vec3 direct_light = vec3(0.0f);  // TODO: Lambert contribution (Eq. 3 style)

            if (is_visible)
                summed_samples += direct_light;
        }

        float light_area = light_source->get_surface_area();
        float normalization = (kDirectLightSamples > 0) ? (light_area / static_cast<float>(kDirectLightSamples)) : 0.0f;
        vec3 light_contribution = vec3(0.0f);  // TODO: combine sample sum and estimator scaling
        (void)normalization;
        (void)summed_samples;
        cumulative_direct_light += light_contribution;
    }

    return cumulative_direct_light * albedo / kPi;
}

vec3 DiffuseMaterial::color_of_last_bounce(Ray& ray, Intersection& intersection, Scene const& scene) {
    using namespace glm;
    /**
     * Color after last bounce will be `W_wip * last_bounce_color`.
     * For this assignment scaffold, the intended final result uses direct diffuse lighting.
     */

    // TODO(Task 4.1): replace fallback with finished direct-light shading.
    vec3 direct_light = this->get_direct_lighting(intersection, scene);
    vec3 shaded = ray.W_wip * direct_light;
    (void)shaded;  // TODO (Task 4.1): Return `shaded` once `get_direct_lighting` is implemented.

    // Boilerplate fallback while Task 4.1 is incomplete. (Remove this normal shading once Task 4.1 is implemented)
    return 0.4f * normalize(intersection.normal) + vec3(0.6f);
}
