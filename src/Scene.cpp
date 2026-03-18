#include <iostream>
#include <limits>
#include <stack>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "ModelBase.h"
#include "Ray.h"
#include "Scene.h"

Scene::Scene(std::unique_ptr<Node> root_node) {
    /**
     * `rootNode` is a root node of transformation tree defining how primitives as
     * coverted to the desired scene. We need to process these tree to convert
     * Intial primitives into world view coordinates.
     */

    // Stacks for DFS
    // Each element in stack represents a node and cumulative trasformation matrix defined
    // by all nodes above current node in the tree
    std::stack<std::pair<Node*, glm::mat4>>
        node_stack;

    // start with root node
    node_stack.push(std::make_pair(root_node.get(), glm::mat4(1.0f)));

    while (!node_stack.empty()) {
        // get top node
        Node* curr_node = node_stack.top().first;
        glm::mat4 curr_mat = node_stack.top().second;
        node_stack.pop();

        // check if current node is leaf node
        if (curr_node->model.get() != nullptr) {
            // add to light sources if emmission is non zero
            if (curr_node->model->is_light_source()) {
                light_sources.push_back(curr_node->model.get());
            }

            // update transformation matrix of model
            curr_node->model->transformation_matrix = glm::mat4(curr_mat);
            curr_node->model->inverse_transform_matrix = glm::inverse(glm::mat4(curr_mat));
            models.push_back(std::move(curr_node->model));

            continue;
        }

        // iterate child nodes and update stack
        for (unsigned int idx = 0; idx < curr_node->childnodes.size(); idx++) {
            // calculate cummulative transformation matrix
            glm::mat4 cumm_mat = curr_mat * curr_node->childtransforms[idx];

            // update stack
            node_stack.push(std::make_pair(curr_node->childnodes[idx].get(), cumm_mat));
        }
    }
}

bool Scene::intersect_nearest(const Ray& ray, Intersection& out_hit, float t_max) const {
    /**
     * Tests the given ray against all objects in the scene and returns whether an intersection was found,
     * storing the closest hit (nearest to the ray origin) in out_hit up to a maximum ray parameter of t_max.
     * If no intersection occurs within the range, the function returns false and out_hit remains unmodified.
     */
    bool has_hit = false;
    float closest_t = t_max;

    for (unsigned int idx = 0; idx < models.size(); idx++) {
        Intersection model_hit;
        if (!models[idx]->intersect_nearest(ray, model_hit, closest_t))
            continue;

        has_hit = true;
        closest_t = model_hit.t;
        out_hit = model_hit;
    }

    return has_hit;
}

Ray Scene::intersect(Ray& ray) const {
    using namespace glm;
    /**
     * For each model in the scene, we will check if the ray intersects
     * the model. If it does, we will update the ray with the intersection
     * details.
     */

    Intersection intersection;
    bool has_intersection = this->intersect_nearest(ray, intersection);

    // update color based on intersection
    if (has_intersection) {
        if (intersection.model->is_light_source()) {
            bool front_face_hit = dot(intersection.normal, -ray.dir) > 0.0f;
            if (!front_face_hit || !ray.allow_emissive_hit)
                // No-MIS policy: suppress BSDF-hit-light terms where NEE already sampled direct light.
                ray.color = vec3(0.0f);
            else {
                float light_area = intersection.model->get_surface_area();
                if (light_area > 0.0f) {
                    // Light stores total power; rays that see the light return emitted radiance.
                    vec3 emitted_radiance = intersection.model->material->emission / light_area;
                    ray.color = ray.W_wip * emitted_radiance;
                } else {
                    ray.color = vec3(0.0f);
                }
            }

            ray.terminate = true;  // no further processing
            return ray;
        }

        // update color for nth last bounce
        ray.color = intersection.model->material->color_of_last_bounce(ray, intersection, *this);

        // This function will give out next ray and
        // update wip color params as well
        ray = intersection.model->material->sample_ray_and_update_radiance(ray, intersection);

        return ray;
    }

    // if no intersection, update color with sky color`
    ray.W_wip = ray.W_wip * this->get_sky_color(ray);  // sky color
    ray.terminate = true;                              // no further processing
    ray.color = ray.W_wip;
    return ray;
}

glm::vec3 Scene::get_sky_color(Ray& ray) const {
    (void)ray;
    // Assignment boilerplate: no environment light until students implement sky/IBL.
    return glm::vec3(0.0f);
}
