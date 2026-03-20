#include "PrimSphere.h"

#include <iostream>
#include <utility>

#include "Intersection.h"
#include "Ray.h"

bool PrimSphere::intersect(const Ray& ray, Intersection& out_hit) const {
    /**
     * NOTE: Ray is already transformed to the Model coordinate space.
     */
    using namespace glm;
    constexpr float kEpsilon = 1e-6f;

    /**
     * TODO(Task 2.1):
     * Implement ray-sphere intersection and write the nearest valid hit to `out_hit`.
     *
     * Suggested steps:
     * 1) Build quadratic coefficients for sphere equation.
     * 2) Compute discriminant and reject invalid cases.
     * 3) Pick nearest positive t (front-facing hit only).
     * 4) Compute hit point and unit normal.
     *
     * Once you compute a valid intersection, populate `out_hit` as:
     * out_hit = {t, point, normal, const_cast<PrimSphere *>(this), nullptr};
     *
     * Note:
     * - Model pointer should remain nullptr here; ModelBase assigns it later.
     * - Only accept intersections with t > kEpsilon.
     */
    // 1. build quadratic coefficients for sphere equation
    // 1. build quadratic coefficients for sphere equation
    vec3 p0_minus_center = ray.p0 - center;
    float b = dot(ray.dir, p0_minus_center);
    float b_squared = b * b;
    float rmc_squared = dot(p0_minus_center, p0_minus_center);

    float discriminant = b_squared - rmc_squared + (radius * radius);

    if (discriminant < 0.0f) {
        return false;
    }

    float t = -1.0f;
    float sqrtD = sqrt(discriminant);

    if (discriminant < 0.0f) {
        return false;
    }
    float t1 = -b - sqrtD;
    float t2 = -b + sqrtD;

    if (t1 > kEpsilon) {
        t = t1;
    } else if (t2 > kEpsilon) {
        t = t2;
    } else {
        return false;
    }

    vec3 point = ray.p0 + t * ray.dir;
    vec3 normal = normalize(point - center);

    out_hit = {t, point, normal, const_cast<PrimSphere *>(this), nullptr};
    return true;
}