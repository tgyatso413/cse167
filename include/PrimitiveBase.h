/**
 * Class to represent primitive shapes like Triangle and Sphere
 */

#ifndef __PRIMITIVE_BASE_H__
#define __PRIMITIVE_BASE_H__

#include "Intersection.h"

class Ray;

class PrimitiveBase {
   public:
    virtual ~PrimitiveBase() = default;
    virtual bool intersect(const Ray &ray, Intersection &out_hit) const = 0;
};

#endif
