#include "ScenePreviewMesh.h"

#include <glm/glm.hpp>

#include <cmath>
#include <iostream>

#include "PrimSphere.h"
#include "PrimTriangle.h"
#include "ModelBase.h"
#include "Scene.h"

namespace {
constexpr int kSphereLonSegments = 24;
constexpr int kSphereLatSegments = 12;
constexpr float kPi = 3.14159265358979323846f;

void append_triangle(const PrimTriangle& tri,
                     const glm::mat4& model_matrix,
                     const glm::mat4& normal_matrix,
                     PreviewMeshData& mesh) {
    uint32_t base_index = static_cast<uint32_t>(mesh.vertices.size());
    for (int i = 0; i < 3; ++i) {
        glm::vec3 world_pos = glm::vec3(model_matrix * glm::vec4(tri.vertices[i], 1.0f));
        glm::vec3 world_normal = glm::normalize(glm::vec3(normal_matrix * glm::vec4(tri.normals[i], 0.0f)));
        mesh.vertices.push_back({world_pos, world_normal});
    }

    mesh.indices.push_back(base_index + 0);
    mesh.indices.push_back(base_index + 1);
    mesh.indices.push_back(base_index + 2);
}

void append_sphere(const PrimSphere& sphere,
                   const glm::mat4& model_matrix,
                   const glm::mat4& normal_matrix,
                   PreviewMeshData& mesh,
                   int lon_segments,
                   int lat_segments) {
    uint32_t start_vertex = static_cast<uint32_t>(mesh.vertices.size());
    int row_stride = lon_segments + 1;

    for (int lat = 0; lat <= lat_segments; ++lat) {
        float theta = kPi * static_cast<float>(lat) / static_cast<float>(lat_segments);
        float sin_theta = std::sin(theta);
        float cos_theta = std::cos(theta);

        for (int lon = 0; lon <= lon_segments; ++lon) {
            float phi = 2.0f * kPi * static_cast<float>(lon) / static_cast<float>(lon_segments);
            float sin_phi = std::sin(phi);
            float cos_phi = std::cos(phi);

            glm::vec3 local_normal = glm::vec3(sin_theta * cos_phi, cos_theta, sin_theta * sin_phi);
            glm::vec3 local_pos = sphere.center + sphere.radius * local_normal;

            glm::vec3 world_pos = glm::vec3(model_matrix * glm::vec4(local_pos, 1.0f));
            glm::vec3 world_normal = glm::normalize(glm::vec3(normal_matrix * glm::vec4(local_normal, 0.0f)));
            mesh.vertices.push_back({world_pos, world_normal});
        }
    }

    for (int lat = 0; lat < lat_segments; ++lat) {
        for (int lon = 0; lon < lon_segments; ++lon) {
            uint32_t i0 = start_vertex + static_cast<uint32_t>(lat * row_stride + lon);
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + static_cast<uint32_t>(row_stride);
            uint32_t i3 = i2 + 1;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);

            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }
}
}  // namespace

PreviewMeshData build_preview_mesh(const Scene& scene) {
    PreviewMeshData mesh;
    bool warned_unsupported = false;

    for (const auto& model_ptr : scene.models) {
        if (!model_ptr)
            continue;

        const ModelBase& model = *model_ptr;
        glm::mat4 model_matrix = model.transformation_matrix;
        glm::mat4 normal_matrix = glm::transpose(model.inverse_transform_matrix);

        for (const auto& primitive_ptr : model.primitives) {
            if (!primitive_ptr)
                continue;

            if (const auto* tri = dynamic_cast<const PrimTriangle*>(primitive_ptr.get())) {
                append_triangle(*tri, model_matrix, normal_matrix, mesh);
                continue;
            }

            if (const auto* sphere = dynamic_cast<const PrimSphere*>(primitive_ptr.get())) {
                append_sphere(*sphere,
                              model_matrix,
                              normal_matrix,
                              mesh,
                              kSphereLonSegments,
                              kSphereLatSegments);
                continue;
            }

            if (!warned_unsupported) {
                std::cerr << "Warning: raster preview skipped unsupported primitive types." << std::endl;
                warned_unsupported = true;
            }
        }
    }

    return mesh;
}
