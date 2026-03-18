/*
Image Class to write image as a texture to the screen
*/
#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <assert.h>

#include <cmath>
#include <glm/glm.hpp>
#include <vector>

#include "PlatformGL.h"
#include "Utility.h"

class Image {
   public:
    unsigned int fbo;  // frame buffer object
    unsigned int tbo;  // texture buffer object

    int width;
    int height;
    std::vector<glm::vec3> pixels;  // Linear HDR radiance values.
    float exposure = 1.0f;

    Image(int width, int height) : width(width), height(height) {
        // Initialize pixels with black color (0, 0, 0)
        pixels.assign(width * height, glm::vec3(0.5f, 0.0f, 1.0f));
    };

    void init() {
        // Generate Buffers
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &tbo);

#ifndef NDEBUG
        validate_color_pipeline_invariants();
#endif
    }

    void updateColor(glm::vec3 color) {
        for (int idx = 0; idx < pixels.size(); idx++) {
            pixels[idx] = glm::vec3(color);
        }
    }

    glm::vec3 ApplyDisplayTransform(glm::vec3 linear_hdr) const {
        glm::vec3 linear = glm::max(linear_hdr, glm::vec3(0.0f));
        glm::vec3 exposed = linear * exposure;
        glm::vec3 clipped = glm::clamp(exposed, glm::vec3(0.0f), glm::vec3(1.0f));
        return LinearToSRGB(clipped);
    }

    void updatePixel(int i, int j, glm::vec3 color) {
        pixels[j * width + i] = color;
    }

    glm::vec3 getPixel(int i, int j) {
        return pixels[j * width + i];
    }

    void copy(Image image) {
        assert(image.pixels.size() == pixels.size());
        for (int idx = 0; idx < pixels.size(); idx++)
            pixels[idx] = glm::vec3(image.pixels[idx]);
    }

    void draw() {
        std::vector<glm::vec3> new_pixels;
        new_pixels.assign(pixels.size(), glm::vec3(0.0f));

        for (int idx = 0; idx < pixels.size(); idx++)
            new_pixels[idx] = ApplyDisplayTransform(pixels[idx]);

        // Load Textures
        glBindTexture(GL_TEXTURE_2D, tbo);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, &new_pixels[0][0]);

        // Attach Texture and Read Frame
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tbo, 0);

#ifdef GL_FRAMEBUFFER_SRGB
        // Keep this disabled: the CPU path already performs linear->sRGB encoding.
        glDisable(GL_FRAMEBUFFER_SRGB);
#endif

        // Copy framebuffer from read to write
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0, 0, width, height,  // source (x0, y0, x1, y1)
            0, height, width, 0,  // destination (x0, y0, x1, y1) flipped vertically
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

   private:
#ifndef NDEBUG
    void validate_color_pipeline_invariants() const {
        auto nearly_equal = [](float a, float b, float eps = 1e-5f) {
            return std::abs(a - b) <= eps;
        };

        glm::vec3 srgb_zero = SRGBToLinear(glm::vec3(0.0f));
        glm::vec3 srgb_one = SRGBToLinear(glm::vec3(1.0f));
        glm::vec3 linear_zero = LinearToSRGB(glm::vec3(0.0f));
        glm::vec3 linear_one = LinearToSRGB(glm::vec3(1.0f));

        assert(nearly_equal(srgb_zero.x, 0.0f) && nearly_equal(srgb_zero.y, 0.0f) && nearly_equal(srgb_zero.z, 0.0f));
        assert(nearly_equal(srgb_one.x, 1.0f) && nearly_equal(srgb_one.y, 1.0f) && nearly_equal(srgb_one.z, 1.0f));
        assert(nearly_equal(linear_zero.x, 0.0f) && nearly_equal(linear_zero.y, 0.0f) && nearly_equal(linear_zero.z, 0.0f));
        assert(nearly_equal(linear_one.x, 1.0f) && nearly_equal(linear_one.y, 1.0f) && nearly_equal(linear_one.z, 1.0f));

        glm::vec3 probe(0.18f, 0.5f, 0.9f);
        glm::vec3 roundtrip = LinearToSRGB(SRGBToLinear(probe));
        glm::vec3 diff = glm::abs(roundtrip - probe);
        assert(diff.x < 1e-4f && diff.y < 1e-4f && diff.z < 1e-4f);

        glm::vec3 stress = ApplyDisplayTransform(glm::vec3(-0.5f, 0.0f, 1000.0f));
        assert(std::isfinite(stress.x) && std::isfinite(stress.y) && std::isfinite(stress.z));
        assert(stress.x >= 0.0f && stress.x <= 1.0f);
        assert(stress.y >= 0.0f && stress.y <= 1.0f);
        assert(stress.z >= 0.0f && stress.z <= 1.0f);
        assert(nearly_equal(stress.x, 0.0f) && nearly_equal(stress.y, 0.0f));
        assert(nearly_equal(stress.z, 1.0f));
    }
#endif
};

#endif
