#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "DiffuseMaterial.h"
#include "MirrorMaterial.h"
#include "RayTracer.h"
#include "Sphere.h"
#include "Square.h"

using namespace glm;

/**
 * NOTE: This scene has large mirror-like reflective surfaces, which makes it very noisy for low spp value
 */

Scene* open_sky() {
    std::shared_ptr<DiffuseMaterial> diffuse_material_white = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.725f, 0.71f, 0.68f)));
    std::shared_ptr<DiffuseMaterial> diffuse_material_red = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.63f, 0.065f, 0.05f)));
    std::shared_ptr<DiffuseMaterial> diffuse_material_green = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.14f, 0.45f, 0.091f)));
    std::shared_ptr<MirrorMaterial> mirror_material = std::make_shared<MirrorMaterial>(vec3(0.95f));
    std::shared_ptr<DiffuseMaterial> light_material = std::make_shared<DiffuseMaterial>(vec3(0.0f));
    // For power-based lights, a 10x10 panel (area 100) needs 100x total power
    // to match prior apparent brightness that used radiance-like tuning values.
    light_material->convert_to_light(vec3(1.0f), vec3(500.0f));  // color, total power

    // Create scene tree
    std::unique_ptr<Node>   
        root_node = std::make_unique<Node>();

    std::unique_ptr<Node> ground = std::make_unique<Node>();
    ground->model = std::make_unique<Sphere>(1e3, vec3(0.0f, 0.0f, 0.0f), diffuse_material_white);
    root_node->childnodes.push_back(std::move(ground));
    root_node->childtransforms.push_back(translate(vec3(0.0f, -1 - 1e3, 0.0f)));

    std::unique_ptr<Node> red_sphere = std::make_unique<Node>();
    red_sphere->model = std::make_unique<Sphere>(0.5, vec3(0.0f, 0.0f, 0.0f), diffuse_material_red);
    root_node->childnodes.push_back(std::move(red_sphere));
    root_node->childtransforms.push_back(translate(vec3(1.0f, -0.5f, 5.0f)));

    std::unique_ptr<Node> green_sphere = std::make_unique<Node>();
    green_sphere->model = std::make_unique<Sphere>(0.5, vec3(0.0f, 0.0f, 0.0f), diffuse_material_green);
    root_node->childnodes.push_back(std::move(green_sphere));
    root_node->childtransforms.push_back(translate(vec3(-1.0f, -0.5f, 5.0f)));

    std::unique_ptr<Node> back_square = std::make_unique<Node>();
    back_square->model = std::make_unique<Square>(vec3(0.0f, 0.0f, 0.0f), 5.0f, vec3(0.0f, 0.0f, 1.0f), mirror_material);
    root_node->childnodes.push_back(std::move(back_square));
    root_node->childtransforms.push_back(translate(vec3(0.0f, 0.0f, 4.0f)) * rotate(degree_to_rad(0.0), vec3(0.0f, 1.0f, 0.0f)));

    std::unique_ptr<Node> other_square = std::make_unique<Node>();
    other_square->model = std::make_unique<Square>(vec3(0.0f, 0.0f, 0.0f), 5.0f, vec3(0.0f, 0.0f, -1.0f), mirror_material);
    root_node->childnodes.push_back(std::move(other_square));
    root_node->childtransforms.push_back(translate(vec3(0.0f, 0.0f, 10.0f)) * rotate(degree_to_rad(0.0), vec3(0.0f, 1.0f, 0.0f)));

    std::unique_ptr<Node> square2 = std::make_unique<Node>();
    square2->model = std::make_unique<Square>(vec3(0.0f, 0.0f, 0.0f), 10.0f, vec3(0.0f, -1.0f, 0.0f), light_material);
    root_node->childnodes.push_back(std::move(square2));
    root_node->childtransforms.push_back(translate(vec3(0.0f, 10.0f, 0.0f)));

    // Initialize the scene
    return new Scene(std::move(root_node));
}
