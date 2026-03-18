#ifndef __MIRROR_MATERIAL_H__
#define __MIRROR_MATERIAL_H__

#include <glm/glm.hpp>

#include "MaterialBase.h"

class MirrorMaterial : public MaterialBase {
   public:
    glm::vec3 reflectance;

    explicit MirrorMaterial(glm::vec3 reflectance)
        : reflectance(SRGBToLinear(glm::clamp(reflectance, glm::vec3(0.0f), glm::vec3(1.0f)))) {}

    Ray sample_ray_and_update_radiance(Ray &ray, Intersection &intersection) override;
    glm::vec3 color_of_last_bounce(Ray &ray, Intersection &intersection, Scene const &scene) override;
};

#endif
