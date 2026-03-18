/*
Triangle Primitive
*/

#ifndef __PRIM_TRIANGLE_H__
#define __PRIM_TRIANGLE_H__

#include <glm/glm.hpp>
#include <vector>

#include "PrimitiveBase.h"
#include "Ray.h"

class MaterialBase;

class Ray;

class PrimTriangle : public PrimitiveBase {
   public:
    glm::vec3 vertices[3];
    glm::vec3 normals[3];

    PrimTriangle(std::vector<glm::vec3> &vertices, std::vector<glm::vec3> &normals);
    bool intersect(const Ray &ray, Intersection &out_hit) const override;
};

#endif
