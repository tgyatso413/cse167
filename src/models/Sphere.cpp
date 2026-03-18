#include "Sphere.h"

#include <iostream>

using namespace glm;

Sphere::Sphere(float radius, vec3 center, std::shared_ptr<MaterialBase> material) {
    // Material Object can be shared between multiple primitives
    this->material = material;

    // This will only have one sphere as primitive
    primitives.push_back(std::make_unique<PrimSphere>(radius, center));
}

Sphere::~Sphere() {}

glm::vec3 Sphere::get_surface_point() {
    /**
     * If this model is a light source, return the position of the light source
     * which is then assumed as a origin of a point light source.
     */

    PrimSphere *prim = static_cast<PrimSphere *>(primitives[0].get());
    vec3 center = prim->center;

    // get random direction for next ray
    // sample it on nomal distribution
    vec3 random_dir(0.0f);
    do {
        random_dir = rand_uniform_vec3(vec3(-1.0f), vec3(1.0f));
    } while (dot(random_dir, random_dir) <= 1e-8f);
    random_dir = normalize(random_dir);

    // return point light position on surface
    vec3 position = center + prim->radius * random_dir;

    // transform to world space
    return vec3(transformation_matrix * vec4(position, 1.0f));
}
