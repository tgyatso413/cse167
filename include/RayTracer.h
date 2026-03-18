#ifndef __RAY_TRACER_H__
#define __RAY_TRACER_H__

#include <functional>

#include "Camera.h"
#include "PrimitiveBase.h"
#include "Scene.h"
#include "ShadingMode.h"

class Ray;
class Image;

class RayTracer {
   public:
    // Camera
    Camera camera;

    // Scene
    std::unique_ptr<Scene> scene;

    // Image
    Image &image;

    // number of samples per pixel
    int samples_per_pixel = 1;
    int max_bounces = 1;

    RayTracer(int width, int height, Image &image) : camera(width, height), image(image) {};
    Ray ray_thru_pixel(int i, int j);
    void draw(const std::function<void()> &on_snapshot = {});
    void init(int scene_id);
    void set_shading_mode(ShadingMode shading_mode);

   private:
    // to determine shading mode
    ShadingMode shading_mode = ShadingMode::NORMAL;  // 0 - normal, 1 - ray tracing, 2 - debug
};
#endif
