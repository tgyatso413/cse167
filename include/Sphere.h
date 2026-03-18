/**
 * This is a wrapper class which wraps aroud PrimSphere to represent Sphere Primitive
 */
#ifndef __SPHERE_H__
#define __SPHERE_H__

#include "PrimSphere.h"
#include "ModelBase.h"
#include "Ray.h"

class MaterialBase;

class Sphere : public ModelBase {
   public:
    Sphere(float radius, glm::vec3 center, std::shared_ptr<MaterialBase> mat);

    ~Sphere() override;

    glm::vec3 get_surface_point() override;
};

#endif