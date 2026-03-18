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

    return false;  // Return `true` if intersection happened, otherwise return `false`.
}