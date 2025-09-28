#ifndef RENDER_H
#define RENDER_H

#include <libdragon.h>
#include <rdpq_tri.h>
#include "hexagon.h"

// Camera parameters structure
typedef struct {
    float x, y, z;           // Camera position
    float yaw_rad;           // Camera rotation in radians
    float focal_length;      // FOV focal length
} camera_t;

// Screen coordinates
typedef struct {
    float x, y;
    int valid;
} screen_pos_t;

// Function prototypes
screen_pos_t project_vertex(float world_x, float world_y, float world_z, camera_t* cam);
void render_hexagon_floor(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt);
void render_hexagon_pillars(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt);
void render_hexagon_walls(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt);

#endif // RENDER_H