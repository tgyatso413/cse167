#ifndef __INTERSECTION_H__
#define __INTERSECTION_H__

#include <glm/glm.hpp>

class PrimitiveBase;
class ModelBase;

struct Intersection {
    float t;                 // Distance from the ray origin to the intersection point
    glm::vec3 point;         // The intersection point
    glm::vec3 normal;        // The normal vector of the primitive at the intersection point
    PrimitiveBase *primitive;  // Pointer to the Primitive object where the intersection occurred
    ModelBase *model;        // Pointer to the Model object to which the intersected primitive belongs
};

#endif