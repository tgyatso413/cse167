/*
Class representing a Ray
*/

#ifndef __RAY_H__
#define __RAY_H__

#include <glm/glm.hpp>

struct Ray {
    bool terminate = false;                      // Path termination flag
    bool allow_emissive_hit = true;              // No-MIS policy gate: false suppresses emissive hits after NEE diffuse bounce
    int pixel_x_coordinate, pixel_y_coordinate;  // Pixel cordinates to keep track of origin pixel for ray
    int n_bounces = 0;                           // Max bounces performed by this ray
    glm::vec3 debug_color = glm::vec3(0.0f);     // Use this to debug

    // Primitive parameters
    glm::vec3 p0;   // basepoint of the ray
    glm::vec3 dir;  // unit direction vector

    // Lighting parameters
    glm::vec3 W_wip = glm::vec3(1.0f);  // wip: Work in progress
    glm::vec3 color = glm::vec3(0.0f);  // Pixel color at end of each bounce
};

#endif
