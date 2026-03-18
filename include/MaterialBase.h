/*
Class Defining Material Properties for lightings
*/
#ifndef __MATERIA_BASE_H__
#define __MATERIA_BASE_H__

#include <glm/glm.hpp>

#include "Intersection.h"
#include "Ray.h"
#include "Utility.h"

class Scene;

class MaterialBase {
   public:
    // For emissive materials, `intensity` and `emission` represent total RGB power in linear space.
    // Per-area radiance (Le) is derived at shading time as `Le = emission / light_area`.
    glm::vec3 emission = glm::vec3(0.0f);
    glm::vec3 intensity = glm::vec3(10.0f);

    virtual Ray sample_ray_and_update_radiance(Ray& ray, Intersection& intersection) = 0;

    virtual void convert_to_light(glm::vec3 color, glm::vec3 intensity) {
        // `color` is authored in sRGB (chroma), while `intensity` is linear RGB power.
        this->intensity = intensity;
        this->emission = SRGBToLinear(color) * intensity;
    }

    virtual glm::vec3 color_of_last_bounce(Ray& ray, Intersection& intersection, Scene const& scene) = 0;
};

#endif
