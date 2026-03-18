/**
 The Screenshot class allows us to save a screenshot for our OpenGL program.
 Specifically, it reads the pixels data from the GL_FRONT buffer and
 writes a .png image file.

 The (private) class members of a Screenshot object are the width, height for the image, and the raw pixel data. The pixel data should be an array of BTYEs. Instead of manually allocating the array, we store the pixel data using std::vector<BYTE>.  Note that for pixels_ of type std::vector<BYTE>, pixels_.data() gives us exactly the desired raw array of BYTEs.  Using std::vector to encapsulate the array, we don't need to worry about writing an appropriate destructor for the class.
 **/

#ifndef __SCREENSHOT_H__
#define __SCREENSHOT_H__
#include <FreeImage.h>

#include <iostream>
#include <vector>

#include "PlatformGL.h"

class Screenshot {
   public:
    Screenshot(int width, int height) : width_(width), height_(height) {
        pixels_.resize(width_ * height_ * 3);
    }
    void save(const char* filename, BOOL topdown) {
        // Captures the already display-transformed framebuffer (post tone-map + sRGB encode).
        glReadBuffer(GL_FRONT);
        GLint previous_pack_alignment = 4;
        glGetIntegerv(GL_PACK_ALIGNMENT, &previous_pack_alignment);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width_, height_, GL_BGR, GL_UNSIGNED_BYTE, pixels_.data());
        glPixelStorei(GL_PACK_ALIGNMENT, previous_pack_alignment);

        FIBITMAP* img = FreeImage_ConvertFromRawBits(pixels_.data(), width_, height_, width_ * 3, 24, 0xFF0000, 0x00FF00, 0x0000FF, topdown);
        if (!img) {
            std::cerr << "Failed to convert screenshot pixels for " << filename << std::endl;
            return;
        }

        std::cout << "Saving screenshot: " << filename << std::endl;

        if (!FreeImage_Save(FIF_PNG, img, filename, 0))
            std::cerr << "Failed to save screenshot: " << filename << std::endl;

        FreeImage_Unload(img);
    }

   private:
    int width_;                 // Window width
    int height_;                // Window height
    std::vector<BYTE> pixels_;  // raw pixel data
};

#endif
