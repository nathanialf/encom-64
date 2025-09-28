#include "render.h"
#include <math.h>
#include <rdpq.h>

// 3D to 2D projection function
screen_pos_t project_vertex(float world_x, float world_y, float world_z, camera_t* cam) {
    screen_pos_t result = {0};
    
    // Translate relative to camera position
    float rel_x = world_x - cam->x;
    float rel_y = world_y - cam->y;
    float rel_z = world_z - cam->z;
    
    // Rotate the view around the camera (inverse rotation)
    float view_x = rel_x * cosf(-cam->yaw_rad) - rel_z * sinf(-cam->yaw_rad);
    float view_z = rel_x * sinf(-cam->yaw_rad) + rel_z * cosf(-cam->yaw_rad);
    
    // Small Z offset to ensure positive depth for projection stability
    view_z += 10.0f;
    
    // 3D to 2D projection
    if(view_z > 0.001f) {
        result.x = 160.0f + (view_x * cam->focal_length) / view_z;
        result.y = 120.0f - (rel_y * cam->focal_length) / view_z;
        result.valid = 1;
        
        // Clamp to reasonable offscreen bounds to prevent RDP issues
        if(result.x < -500.0f) result.x = -500.0f;
        if(result.x > 820.0f) result.x = 820.0f;
        if(result.y < -500.0f) result.y = -500.0f;
        if(result.y > 740.0f) result.y = 740.0f;
    }
    
    return result;
}

// Render hexagon floor
void render_hexagon_floor(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt) {
    // Project hexagon vertices to screen coordinates
    screen_pos_t screen_pos[6];
    for(int i = 0; i < 6; i++) {
        screen_pos[i] = project_vertex(hex->vertices_x[i], 0.0f, hex->vertices_z[i], cam);
    }
    
    // Set floor color (gray)
    rdpq_set_prim_color(RGBA32(128, 128, 128, 255));
    
    // Draw triangles forming flat hexagon plane (use better triangulation)
    // Triangle 1: vertices 0, 1, 2
    if(screen_pos[0].valid && screen_pos[1].valid && screen_pos[2].valid) {
        float v1[2] = { screen_pos[0].x, screen_pos[0].y };
        float v2[2] = { screen_pos[1].x, screen_pos[1].y };
        float v3[2] = { screen_pos[2].x, screen_pos[2].y };
        rdpq_triangle(trifmt, v1, v2, v3);
    }
    
    // Triangle 2: vertices 0, 2, 3  
    if(screen_pos[0].valid && screen_pos[2].valid && screen_pos[3].valid) {
        float v4[2] = { screen_pos[0].x, screen_pos[0].y };
        float v5[2] = { screen_pos[2].x, screen_pos[2].y };
        float v6[2] = { screen_pos[3].x, screen_pos[3].y };
        rdpq_triangle(trifmt, v4, v5, v6);
    }
    
    // Triangle 3: vertices 0, 3, 4
    if(screen_pos[0].valid && screen_pos[3].valid && screen_pos[4].valid) {
        float v7[2] = { screen_pos[0].x, screen_pos[0].y };
        float v8[2] = { screen_pos[3].x, screen_pos[3].y };
        float v9[2] = { screen_pos[4].x, screen_pos[4].y };
        rdpq_triangle(trifmt, v7, v8, v9);
    }
    
    // Triangle 4: vertices 0, 4, 5
    if(screen_pos[0].valid && screen_pos[4].valid && screen_pos[5].valid) {
        float v10[2] = { screen_pos[0].x, screen_pos[0].y };
        float v11[2] = { screen_pos[4].x, screen_pos[4].y };
        float v12[2] = { screen_pos[5].x, screen_pos[5].y };
        rdpq_triangle(trifmt, v10, v11, v12);
    }
}

// Render pillars at hexagon vertices
void render_hexagon_pillars(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt) {
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));  // White pillars
    
    for(int i = 0; i < 6; i++) {
        // Create small square pillar at each vertex (2x2 units)
        float pillar_center_x = hex->vertices_x[i];
        float pillar_center_z = hex->vertices_z[i];
        float pillar_size = 1.0f;  // Half-size for square
        
        // Four corners of the pillar base
        float corners_x[4] = {
            pillar_center_x - pillar_size,  // Bottom-left
            pillar_center_x + pillar_size,  // Bottom-right
            pillar_center_x + pillar_size,  // Top-right
            pillar_center_x - pillar_size   // Top-left
        };
        float corners_z[4] = {
            pillar_center_z - pillar_size,  // Bottom-left
            pillar_center_z - pillar_size,  // Bottom-right
            pillar_center_z + pillar_size,  // Top-right
            pillar_center_z + pillar_size   // Top-left
        };
        
        // Project each corner for bottom (Y=0) and top (Y=20)
        screen_pos_t bottom_screen[4], top_screen[4];
        
        for(int j = 0; j < 4; j++) {
            bottom_screen[j] = project_vertex(corners_x[j], 0.0f, corners_z[j], cam);
            top_screen[j] = project_vertex(corners_x[j], 20.0f, corners_z[j], cam);
        }
        
        // Draw pillar faces as triangles (simplified - just front face)
        if(bottom_screen[0].valid && bottom_screen[1].valid && top_screen[0].valid && top_screen[1].valid) {
            // Triangle 1: bottom-left, bottom-right, top-left
            float v1[2] = { bottom_screen[0].x, bottom_screen[0].y };
            float v2[2] = { bottom_screen[1].x, bottom_screen[1].y };
            float v3[2] = { top_screen[0].x, top_screen[0].y };
            rdpq_triangle(trifmt, v1, v2, v3);
            
            // Triangle 2: bottom-right, top-right, top-left
            float v4[2] = { bottom_screen[1].x, bottom_screen[1].y };
            float v5[2] = { top_screen[1].x, top_screen[1].y };
            float v6[2] = { top_screen[0].x, top_screen[0].y };
            rdpq_triangle(trifmt, v4, v5, v6);
        }
    }
}

// Render walls for a hexagon (simple version - just north wall for now)
void render_hexagon_walls(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt) {
    // Draw true north wall (between vertices 1 and 2)
    rdpq_set_prim_color(RGBA32(0, 255, 0, 255));  // Green wall
    
    screen_pos_t wall_bottom[2], wall_top[2];
    wall_bottom[0] = project_vertex(hex->vertices_x[1], 0.0f, hex->vertices_z[1], cam);
    wall_bottom[1] = project_vertex(hex->vertices_x[2], 0.0f, hex->vertices_z[2], cam);
    wall_top[0] = project_vertex(hex->vertices_x[1], 20.0f, hex->vertices_z[1], cam);
    wall_top[1] = project_vertex(hex->vertices_x[2], 20.0f, hex->vertices_z[2], cam);
    
    // Draw north wall as 2 triangles if all vertices are valid
    if(wall_bottom[0].valid && wall_bottom[1].valid && wall_top[0].valid && wall_top[1].valid) {
        // Triangle 1: bottom-left, bottom-right, top-left
        float n1[2] = { wall_bottom[0].x, wall_bottom[0].y };
        float n2[2] = { wall_bottom[1].x, wall_bottom[1].y };
        float n3[2] = { wall_top[0].x, wall_top[0].y };
        rdpq_triangle(trifmt, n1, n2, n3);
        
        // Triangle 2: bottom-right, top-right, top-left
        float n4[2] = { wall_bottom[1].x, wall_bottom[1].y };
        float n5[2] = { wall_top[1].x, wall_top[1].y };
        float n6[2] = { wall_top[0].x, wall_top[0].y };
        rdpq_triangle(trifmt, n4, n5, n6);
    }
}