#ifndef __RASTER_PREVIEW_RENDERER_H__
#define __RASTER_PREVIEW_RENDERER_H__

#include <cstddef>

#include "Camera.h"
#include "ScenePreviewMesh.h"

class RasterPreviewRenderer {
   public:
    bool init(const PreviewMeshData& mesh);
    void draw(const Camera& camera);
    void shutdown();

    ~RasterPreviewRenderer() { shutdown(); }

   private:
    unsigned int program_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
    int index_count_ = 0;
};

#endif
