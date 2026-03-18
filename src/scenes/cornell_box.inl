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

namespace {
Scene* build_cornell_box(bool mirror_sphere) {
    // Materials
    std::shared_ptr<DiffuseMaterial> diffuse_material_white_wall = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.725f, 0.71f, 0.68f)));
    std::shared_ptr<DiffuseMaterial> diffuse_material_red_wall = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.63f, 0.065f, 0.05f)));
    std::shared_ptr<DiffuseMaterial> diffuse_material_green_wall = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.14f, 0.45f, 0.091f)));
    std::shared_ptr<DiffuseMaterial> diffuse_material_cuboid = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.85f, 0.75f, 0.5f)));
    std::shared_ptr<DiffuseMaterial> diffuse_material_sphere = std::make_shared<DiffuseMaterial>(LinearToSRGB(vec3(0.85f, 0.75f, 0.5f)));
    std::shared_ptr<MirrorMaterial> mirror_material_sphere = std::make_shared<MirrorMaterial>(vec3(0.95f));
    std::shared_ptr<DiffuseMaterial> light_material = std::make_shared<DiffuseMaterial>(vec3(0.0f));
    light_material->convert_to_light(vec3(1.0f), vec3(30.0f));  // color, total power

    // Create scene tree
    std::unique_ptr<Node> root_node = std::make_unique<Node>();

    std::unique_ptr<Node> floor_square = std::make_unique<Node>();
    floor_square->model = std::make_unique<Square>(vec3(0.0f), 4.0f, vec3(0.0f, 1.0f, 0.0f), diffuse_material_white_wall);
    root_node->childnodes.push_back(std::move(floor_square));
    root_node->childtransforms.push_back(translate(vec3(0.0f, -2.0f, 0.0f)));

    std::unique_ptr<Node> ceiling_square = std::make_unique<Node>();
    ceiling_square->model = std::make_unique<Square>(vec3(0.0f), 4.0f, vec3(0.0f, -1.0f, 0.0f), diffuse_material_white_wall);
    root_node->childnodes.push_back(std::move(ceiling_square));
    root_node->childtransforms.push_back(translate(vec3(0.0f, 2.0f, 0.0f)));

    std::unique_ptr<Node> left_square = std::make_unique<Node>();
    left_square->model = std::make_unique<Square>(vec3(0.0f), 4.0f, vec3(1.0f, 0.0f, 0.0f), diffuse_material_red_wall);
    root_node->childnodes.push_back(std::move(left_square));
    root_node->childtransforms.push_back(translate(vec3(-2.0f, 0.0f, 0.0f)));

    std::unique_ptr<Node> right_square = std::make_unique<Node>();
    right_square->model = std::make_unique<Square>(vec3(0.0f), 4.0f, vec3(-1.0f, 0.0f, 0.0f), diffuse_material_green_wall);
    root_node->childnodes.push_back(std::move(right_square));
    root_node->childtransforms.push_back(translate(vec3(2.0f, 0.0f, 0.0f)));

    std::unique_ptr<Node> back_square = std::make_unique<Node>();
    back_square->model = std::make_unique<Square>(vec3(0.0f), 4.0f, vec3(0.0f, 0.0f, 1.0f), diffuse_material_white_wall);
    root_node->childnodes.push_back(std::move(back_square));
    root_node->childtransforms.push_back(translate(vec3(0.0f, 0.0f, -2.0f)));

    std::unique_ptr<Node> square_light = std::make_unique<Node>();
    square_light->model = std::make_unique<Square>(vec3(0.0f), 1.0f, vec3(0.0f, -1.0f, 0.0f), light_material);
    root_node->childnodes.push_back(std::move(square_light));
    root_node->childtransforms.push_back(translate(vec3(0.0f, 2.0f, 0.0f)));

    // Diffuse cuboid composed from six square faces in local model space.
    std::unique_ptr<Node> cuboid_node = std::make_unique<Node>();

    std::unique_ptr<Node> cuboid_pos_x_face = std::make_unique<Node>();
    cuboid_pos_x_face->model = std::make_unique<Square>(vec3(0.0f), 1.0f, vec3(1.0f, 0.0f, 0.0f), diffuse_material_cuboid);
    cuboid_node->childnodes.push_back(std::move(cuboid_pos_x_face));
    cuboid_node->childtransforms.push_back(translate(vec3(0.5f, 0.0f, 0.0f)));

    std::unique_ptr<Node> cuboid_neg_x_face = std::make_unique<Node>();
    cuboid_neg_x_face->model = std::make_unique<Square>(vec3(0.0f), 1.0f, vec3(-1.0f, 0.0f, 0.0f), diffuse_material_cuboid);
    cuboid_node->childnodes.push_back(std::move(cuboid_neg_x_face));
    cuboid_node->childtransforms.push_back(translate(vec3(-0.5f, 0.0f, 0.0f)));

    std::unique_ptr<Node> cuboid_pos_y_face = std::make_unique<Node>();
    cuboid_pos_y_face->model = std::make_unique<Square>(vec3(0.0f), 1.0f, vec3(0.0f, 1.0f, 0.0f), diffuse_material_cuboid);
    cuboid_node->childnodes.push_back(std::move(cuboid_pos_y_face));
    cuboid_node->childtransforms.push_back(translate(vec3(0.0f, 0.5f, 0.0f)));

    std::unique_ptr<Node> cuboid_neg_y_face = std::make_unique<Node>();
    cuboid_neg_y_face->model = std::make_unique<Square>(vec3(0.0f), 1.0f, vec3(0.0f, -1.0f, 0.0f), diffuse_material_cuboid);
    cuboid_node->childnodes.push_back(std::move(cuboid_neg_y_face));
    cuboid_node->childtransforms.push_back(translate(vec3(0.0f, -0.5f, 0.0f)));

    std::unique_ptr<Node> cuboid_pos_z_face = std::make_unique<Node>();
    cuboid_pos_z_face->model = std::make_unique<Square>(vec3(0.0f), 1.0f, vec3(0.0f, 0.0f, 1.0f), diffuse_material_cuboid);
    cuboid_node->childnodes.push_back(std::move(cuboid_pos_z_face));
    cuboid_node->childtransforms.push_back(translate(vec3(0.0f, 0.0f, 0.5f)));

    std::unique_ptr<Node> cuboid_neg_z_face = std::make_unique<Node>();
    cuboid_neg_z_face->model = std::make_unique<Square>(vec3(0.0f), 1.0f, vec3(0.0f, 0.0f, -1.0f), diffuse_material_cuboid);
    cuboid_node->childnodes.push_back(std::move(cuboid_neg_z_face));
    cuboid_node->childtransforms.push_back(translate(vec3(0.0f, 0.0f, -0.5f)));

    root_node->childnodes.push_back(std::move(cuboid_node));
    root_node->childtransforms.push_back(
        translate(vec3(-0.65f, -1.0f, -0.45f)) *
        rotate(degree_to_rad(18.0f), vec3(0.0f, 1.0f, 0.0f)) *
        scale(vec3(1.0f, 2.0f, 1.0f)));

    std::shared_ptr<MaterialBase> sphere_material = mirror_sphere
                                                         ? std::static_pointer_cast<MaterialBase>(mirror_material_sphere)
                                                         : std::static_pointer_cast<MaterialBase>(diffuse_material_sphere);
    std::unique_ptr<Node> sphere_node = std::make_unique<Node>();
    sphere_node->model = std::make_unique<Sphere>(0.6f, vec3(0.0f), sphere_material);
    root_node->childnodes.push_back(std::move(sphere_node));
    root_node->childtransforms.push_back(translate(vec3(0.75f, -1.40f, 0.55f)));

    // Initialize the scene
    return new Scene(std::move(root_node));
}
}  // namespace

Scene* cornell_box() {
    return build_cornell_box(false);
}

Scene* cornell_box_mirror() {
    return build_cornell_box(true);
}
