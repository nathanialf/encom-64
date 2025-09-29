#ifndef RENDER_H
#define RENDER_H

#include <libdragon.h>
#include <rdpq_tri.h>
#include "hexagon.h"
#include "../generated/map_data.h"

// External declarations
extern hexagon_t hexagons[MAP_HEX_COUNT];

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

// Wall segment for depth sorting
typedef struct {
    float distance;
    hexagon_t* hex;
    int wall_dir;
} wall_segment_t;

// Function prototypes
screen_pos_t project_vertex(float world_x, float world_y, float world_z, camera_t* cam);
void render_hexagon_floor(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt);
void render_hexagon_ceiling(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt);
void render_hexagon_pillars(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt);
void render_hexagon_walls(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt);
void render_single_wall(hexagon_t* hex, int wall_dir, camera_t* cam, rdpq_trifmt_t* trifmt);

// Collision detection
int check_collision(float new_x, float new_z, float player_radius);
float point_to_line_distance(float px, float pz, float x1, float z1, float x2, float z2);
int check_doorframe_collision(hexagon_t* hex, int wall_dir, int v1_idx, int v2_idx, float new_x, float new_z, float player_radius);

#endif // RENDER_H