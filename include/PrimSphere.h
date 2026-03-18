/*
Sphere Primitive
*/

#ifndef __PRIM_SPHERE_H__
#define __PRIM_SPHERE_H__

#include <glm/glm.hpp>

#include "PrimitiveBase.h"
#include "Ray.h"

class MaterialBase;

class Ray;

class PrimSphere : public PrimitiveBase {
   public:
    float radius;
    glm::vec3 center;

    PrimSphere(float radius, glm::vec3 center) : radius(radius), center(center) {};
    bool intersect(const Ray &ray, Intersection &out_hit) const override;
};

#endif
