#ifndef __SCENE_PREVIEW_MESH_H__
#define __SCENE_PREVIEW_MESH_H__

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

class Scene;

struct PreviewVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct PreviewMeshData {
    std::vector<PreviewVertex> vertices;
    std::vector<uint32_t> indices;
};

PreviewMeshData build_preview_mesh(const Scene& scene);

#endif
