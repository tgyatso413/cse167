#ifndef __MODEL_BASE_H__
#define __MODEL_BASE_H__
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <limits>

#include "PrimitiveBase.h"
#include "MaterialBase.h"
#include "Ray.h"

using namespace glm;

class ModelBase {
   public:
    // List of primitives composing the given model
    std::vector<std::unique_ptr<PrimitiveBase>> primitives;
    std::shared_ptr<MaterialBase> material;
    glm::mat4 transformation_matrix;
    glm::mat4 inverse_transform_matrix;

    ModelBase() = default;

    virtual ~ModelBase() = default;

    virtual bool intersect_nearest(const Ray &ray,
                                   Intersection &out_hit,
                                   float t_max = std::numeric_limits<float>::max()) const {
        bool has_hit = false;
        float closest_t = t_max;

        Ray ray_model;
        ray_model.p0 = vec3(inverse_transform_matrix * vec4(ray.p0, 1.0f));
        ray_model.dir = normalize(vec3(inverse_transform_matrix * vec4(ray.dir, 0.0f)));

        mat4 normal_matrix = transpose(inverse_transform_matrix);

        for (unsigned int idx = 0; idx < primitives.size(); idx++) {
            Intersection model_hit;
            if (!primitives[idx]->intersect(ray_model, model_hit))
                continue;

            vec3 world_point = vec3(transformation_matrix * vec4(model_hit.point, 1.0f));
            float world_t = length(world_point - ray.p0);
            if (world_t <= 0.0f || world_t >= closest_t)
                continue;

            has_hit = true;
            closest_t = world_t;

            out_hit = model_hit;
            out_hit.model = const_cast<ModelBase *>(this);
            out_hit.point = world_point;
            out_hit.normal = normalize(vec3(normal_matrix * vec4(model_hit.normal, 0.0f)));
            out_hit.t = world_t;
        }

        return has_hit;
    }

    virtual glm::vec3 get_surface_point() = 0;

    virtual float get_surface_area() const {
        // Area is only required for emissive-primitive direct lighting.
        // Non-area-light models can keep the default unit area.
        return 1.0f;
    }

    virtual bool is_light_source() {
        return glm::length(material->emission) > 0;
    }
};

#endif
