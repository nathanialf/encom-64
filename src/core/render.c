#include "render.h"
#include <math.h>
#include <rdpq.h>
#include "../generated/map_data.h"

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
        
        // Clamp to tighter bounds to prevent visual sliding
        if(result.x < -200.0f) result.x = -200.0f;
        if(result.x > 520.0f) result.x = 520.0f;
        if(result.y < -200.0f) result.y = -200.0f;
        if(result.y > 440.0f) result.y = 440.0f;
    } else {
        result.valid = 0;  // Mark as invalid for walls/pillars to skip them
    }
    
    return result;
}

// Special projection for floor vertices that always returns valid coordinates
screen_pos_t project_vertex_floor(float world_x, float world_y, float world_z, camera_t* cam) {
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
    
    // 3D to 2D projection (always return valid coordinates for floors)
    if(view_z > 0.001f) {
        result.x = 160.0f + (view_x * cam->focal_length) / view_z;
        result.y = 120.0f - (rel_y * cam->focal_length) / view_z;
    } else {
        // For vertices behind camera, project to near plane
        float near_z = 0.5f;
        result.x = 160.0f + (view_x * cam->focal_length) / near_z;
        result.y = 120.0f - (rel_y * cam->focal_length) / near_z;
    }
    
    result.valid = 1;  // Always mark as valid for floors
    
    // Clamp to reasonable offscreen bounds to prevent RDP issues
    if(result.x < -500.0f) result.x = -500.0f;
    if(result.x > 820.0f) result.x = 820.0f;
    if(result.y < -500.0f) result.y = -500.0f;
    if(result.y > 740.0f) result.y = 740.0f;
    
    return result;
}

// Render hexagon floor
void render_hexagon_floor(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt) {
    // Project hexagon vertices to screen coordinates using floor projection
    screen_pos_t screen_pos[6];
    for(int i = 0; i < 6; i++) {
        screen_pos[i] = project_vertex_floor(hex->vertices_x[i], 0.0f, hex->vertices_z[i], cam);
    }
    
    // Set floor color (gray)
    rdpq_set_prim_color(RGBA32(128, 128, 128, 255));
    
    // Draw triangles forming flat hexagon plane (render unconditionally)
    // Triangle 1: vertices 0, 1, 2
    {
        float v1[2] = { screen_pos[0].x, screen_pos[0].y };
        float v2[2] = { screen_pos[1].x, screen_pos[1].y };
        float v3[2] = { screen_pos[2].x, screen_pos[2].y };
        rdpq_triangle(trifmt, v1, v2, v3);
    }
    
    // Triangle 2: vertices 0, 2, 3  
    {
        float v4[2] = { screen_pos[0].x, screen_pos[0].y };
        float v5[2] = { screen_pos[2].x, screen_pos[2].y };
        float v6[2] = { screen_pos[3].x, screen_pos[3].y };
        rdpq_triangle(trifmt, v4, v5, v6);
    }
    
    // Triangle 3: vertices 0, 3, 4
    {
        float v7[2] = { screen_pos[0].x, screen_pos[0].y };
        float v8[2] = { screen_pos[3].x, screen_pos[3].y };
        float v9[2] = { screen_pos[4].x, screen_pos[4].y };
        rdpq_triangle(trifmt, v7, v8, v9);
    }
    
    // Triangle 4: vertices 0, 4, 5
    {
        float v10[2] = { screen_pos[0].x, screen_pos[0].y };
        float v11[2] = { screen_pos[4].x, screen_pos[4].y };
        float v12[2] = { screen_pos[5].x, screen_pos[5].y };
        rdpq_triangle(trifmt, v10, v11, v12);
    }
}

// Render hexagon ceiling
void render_hexagon_ceiling(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt) {
    // Project hexagon vertices to screen coordinates at ceiling height (20.0f)
    screen_pos_t screen_pos[6];
    for(int i = 0; i < 6; i++) {
        screen_pos[i] = project_vertex_floor(hex->vertices_x[i], 20.0f, hex->vertices_z[i], cam);
    }
    
    // Set ceiling color (darker gray than floor)
    rdpq_set_prim_color(RGBA32(64, 64, 64, 255));
    
    // Draw triangles forming flat hexagon plane (render unconditionally)
    // Note: Reverse winding order for ceiling so triangles face downward
    // Triangle 1: vertices 2, 1, 0 (reversed from floor)
    {
        float v1[2] = { screen_pos[2].x, screen_pos[2].y };
        float v2[2] = { screen_pos[1].x, screen_pos[1].y };
        float v3[2] = { screen_pos[0].x, screen_pos[0].y };
        rdpq_triangle(trifmt, v1, v2, v3);
    }
    
    // Triangle 2: vertices 3, 2, 0 (reversed from floor)
    {
        float v4[2] = { screen_pos[3].x, screen_pos[3].y };
        float v5[2] = { screen_pos[2].x, screen_pos[2].y };
        float v6[2] = { screen_pos[0].x, screen_pos[0].y };
        rdpq_triangle(trifmt, v4, v5, v6);
    }
    
    // Triangle 3: vertices 4, 3, 0 (reversed from floor)
    {
        float v7[2] = { screen_pos[4].x, screen_pos[4].y };
        float v8[2] = { screen_pos[3].x, screen_pos[3].y };
        float v9[2] = { screen_pos[0].x, screen_pos[0].y };
        rdpq_triangle(trifmt, v7, v8, v9);
    }
    
    // Triangle 4: vertices 5, 4, 0 (reversed from floor)
    {
        float v10[2] = { screen_pos[5].x, screen_pos[5].y };
        float v11[2] = { screen_pos[4].x, screen_pos[4].y };
        float v12[2] = { screen_pos[0].x, screen_pos[0].y };
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

// Helper function to render a full wall between two vertices
static void render_wall_segment(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt, int v1_idx, int v2_idx) {
    screen_pos_t wall_bottom[2], wall_top[2];
    wall_bottom[0] = project_vertex(hex->vertices_x[v1_idx], 0.0f, hex->vertices_z[v1_idx], cam);
    wall_bottom[1] = project_vertex(hex->vertices_x[v2_idx], 0.0f, hex->vertices_z[v2_idx], cam);
    wall_top[0] = project_vertex(hex->vertices_x[v1_idx], 20.0f, hex->vertices_z[v1_idx], cam);
    wall_top[1] = project_vertex(hex->vertices_x[v2_idx], 20.0f, hex->vertices_z[v2_idx], cam);
    
    // Draw wall as 2 triangles if all vertices are valid
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

// Helper function to render doorway (partial walls from pillars with center gap)
static void render_doorway_segment(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt, int v1_idx, int v2_idx) {
    // Calculate distance for LOD decisions
    float dx = hex->center_x - cam->x;
    float dz = hex->center_z - cam->z;
    float dist_sq = dx*dx + dz*dz;
    int skip_doorframes = (dist_sq > 40000.0f); // Skip doorframes beyond 200 units
    screen_pos_t wall_bottom[2], wall_top[2];
    wall_bottom[0] = project_vertex(hex->vertices_x[v1_idx], 0.0f, hex->vertices_z[v1_idx], cam);
    wall_bottom[1] = project_vertex(hex->vertices_x[v2_idx], 0.0f, hex->vertices_z[v2_idx], cam);
    wall_top[0] = project_vertex(hex->vertices_x[v1_idx], 20.0f, hex->vertices_z[v1_idx], cam);
    wall_top[1] = project_vertex(hex->vertices_x[v2_idx], 20.0f, hex->vertices_z[v2_idx], cam);
    
    if(wall_bottom[0].valid && wall_bottom[1].valid && wall_top[0].valid && wall_top[1].valid) {
        // Calculate doorway dimensions (leave 1/3 gap in center, 1/3 wall on each side)
        float door_gap = 0.33f;
        float wall_portion = (1.0f - door_gap) / 2.0f;
        
        // Left wall segment (interpolate in world space, then project)
        float left_end_world_x = hex->vertices_x[v1_idx] + wall_portion * (hex->vertices_x[v2_idx] - hex->vertices_x[v1_idx]);
        float left_end_world_z = hex->vertices_z[v1_idx] + wall_portion * (hex->vertices_z[v2_idx] - hex->vertices_z[v1_idx]);
        
        screen_pos_t left_end_bottom = project_vertex(left_end_world_x, 0.0f, left_end_world_z, cam);
        screen_pos_t left_end_top = project_vertex(left_end_world_x, 20.0f, left_end_world_z, cam);
        
        if(wall_bottom[0].valid && wall_top[0].valid && left_end_bottom.valid && left_end_top.valid) {
            // Left wall triangles
            float l1[2] = { wall_bottom[0].x, wall_bottom[0].y };
            float l2[2] = { left_end_bottom.x, left_end_bottom.y };
            float l3[2] = { wall_top[0].x, wall_top[0].y };
            rdpq_triangle(trifmt, l1, l2, l3);
            
            float l4[2] = { left_end_bottom.x, left_end_bottom.y };
            float l5[2] = { left_end_top.x, left_end_top.y };
            float l6[2] = { wall_top[0].x, wall_top[0].y };
            rdpq_triangle(trifmt, l4, l5, l6);
        }
        
        // Right wall segment (interpolate in world space, then project)
        float right_wall_start = 1.0f - wall_portion;
        float right_start_world_x = hex->vertices_x[v1_idx] + right_wall_start * (hex->vertices_x[v2_idx] - hex->vertices_x[v1_idx]);
        float right_start_world_z = hex->vertices_z[v1_idx] + right_wall_start * (hex->vertices_z[v2_idx] - hex->vertices_z[v1_idx]);
        
        screen_pos_t right_start_bottom = project_vertex(right_start_world_x, 0.0f, right_start_world_z, cam);
        screen_pos_t right_start_top = project_vertex(right_start_world_x, 20.0f, right_start_world_z, cam);
        
        if(wall_bottom[1].valid && wall_top[1].valid && right_start_bottom.valid && right_start_top.valid) {
            // Right wall triangles
            float r1[2] = { right_start_bottom.x, right_start_bottom.y };
            float r2[2] = { wall_bottom[1].x, wall_bottom[1].y };
            float r3[2] = { right_start_top.x, right_start_top.y };
            rdpq_triangle(trifmt, r1, r2, r3);
            
            float r4[2] = { wall_bottom[1].x, wall_bottom[1].y };
            float r5[2] = { wall_top[1].x, wall_top[1].y };
            float r6[2] = { right_start_top.x, right_start_top.y };
            rdpq_triangle(trifmt, r4, r5, r6);
        }
        
        // Skip doorframes for distant hexagons to save triangles
        if(skip_doorframes) return;
        
        // Set doorframe color to black
        rdpq_set_prim_color(RGBA32(0, 0, 0, 255));
        
        // Use the already calculated door edge positions
        // (left_end_world_x/z and right_start_world_x/z are already calculated above)
        
        // Calculate wall direction and parallel offset for doorframe thickness
        float wall_dx = hex->vertices_x[v2_idx] - hex->vertices_x[v1_idx];
        float wall_dz = hex->vertices_z[v2_idx] - hex->vertices_z[v1_idx];
        float wall_length = sqrtf(wall_dx * wall_dx + wall_dz * wall_dz);
        
        // Doorframe thickness parallel to wall direction (thinner)
        float frame_thickness = 1.5f;
        float frame_dx = (wall_dx / wall_length) * frame_thickness;
        float frame_dz = (wall_dz / wall_length) * frame_thickness;
        
        // Left doorframe (at end of left wall segment)
        // Full height of door segment
        float frame_height_start = 0.0f;
        float frame_height_end = 20.0f;
        
        // Position doorframes directly at wall surface (no offset)
        screen_pos_t left_frame_1 = project_vertex(left_end_world_x, frame_height_start, left_end_world_z, cam);
        screen_pos_t left_frame_2 = project_vertex(left_end_world_x + frame_dx, frame_height_start, left_end_world_z + frame_dz, cam);
        screen_pos_t left_frame_3 = project_vertex(left_end_world_x, frame_height_end, left_end_world_z, cam);
        screen_pos_t left_frame_4 = project_vertex(left_end_world_x + frame_dx, frame_height_end, left_end_world_z + frame_dz, cam);
        
        if(left_frame_1.valid && left_frame_2.valid && left_frame_3.valid && left_frame_4.valid) {
            float lf1[2] = { left_frame_1.x, left_frame_1.y };
            float lf2[2] = { left_frame_2.x, left_frame_2.y };
            float lf3[2] = { left_frame_3.x, left_frame_3.y };
            rdpq_triangle(trifmt, lf1, lf2, lf3);
            
            float lf4[2] = { left_frame_2.x, left_frame_2.y };
            float lf5[2] = { left_frame_4.x, left_frame_4.y };
            float lf6[2] = { left_frame_3.x, left_frame_3.y };
            rdpq_triangle(trifmt, lf4, lf5, lf6);
        }
        
        // Right doorframe (at end of right wall segment)
        screen_pos_t right_frame_1 = project_vertex(right_start_world_x, frame_height_start, right_start_world_z, cam);
        screen_pos_t right_frame_2 = project_vertex(right_start_world_x - frame_dx, frame_height_start, right_start_world_z - frame_dz, cam);
        screen_pos_t right_frame_3 = project_vertex(right_start_world_x, frame_height_end, right_start_world_z, cam);
        screen_pos_t right_frame_4 = project_vertex(right_start_world_x - frame_dx, frame_height_end, right_start_world_z - frame_dz, cam);
        
        if(right_frame_1.valid && right_frame_2.valid && right_frame_3.valid && right_frame_4.valid) {
            float rf1[2] = { right_frame_1.x, right_frame_1.y };
            float rf2[2] = { right_frame_2.x, right_frame_2.y };
            float rf3[2] = { right_frame_3.x, right_frame_3.y };
            rdpq_triangle(trifmt, rf1, rf2, rf3);
            
            float rf4[2] = { right_frame_2.x, right_frame_2.y };
            float rf5[2] = { right_frame_4.x, right_frame_4.y };
            float rf6[2] = { right_frame_3.x, right_frame_3.y };
            rdpq_triangle(trifmt, rf4, rf5, rf6);
        }
        
        // Reset wall color back to green
        rdpq_set_prim_color(RGBA32(0, 255, 0, 255));
    }
}

// Render walls for a hexagon with doorway logic
void render_hexagon_walls(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt) {
    rdpq_set_prim_color(RGBA32(0, 255, 0, 255));  // Green wall
    
    // North wall (vertices 1->2)
    if(!(hex->connections & CONN_NORTH)) {
        // No connection: full wall
        render_wall_segment(hex, cam, trifmt, 1, 2);
    } else if(hex->type == HEX_TYPE_CORRIDOR) {
        // Corridor connection: doorway
        render_doorway_segment(hex, cam, trifmt, 1, 2);
    }
    // Room connection: no wall at all
    
    // South wall (vertices 4->5)
    if(!(hex->connections & CONN_SOUTH)) {
        render_wall_segment(hex, cam, trifmt, 4, 5);
    } else if(hex->type == HEX_TYPE_CORRIDOR) {
        render_doorway_segment(hex, cam, trifmt, 4, 5);
    }
    
    // Southeast wall (vertices 5->0)
    if(!(hex->connections & CONN_SOUTHEAST)) {
        render_wall_segment(hex, cam, trifmt, 5, 0);
    } else if(hex->type == HEX_TYPE_CORRIDOR) {
        render_doorway_segment(hex, cam, trifmt, 5, 0);
    }
    
    // Northeast wall (vertices 0->1)
    if(!(hex->connections & CONN_NORTHEAST)) {
        render_wall_segment(hex, cam, trifmt, 0, 1);
    } else if(hex->type == HEX_TYPE_CORRIDOR) {
        render_doorway_segment(hex, cam, trifmt, 0, 1);
    }
    
    // Northwest wall (vertices 2->3)
    if(!(hex->connections & CONN_NORTHWEST)) {
        render_wall_segment(hex, cam, trifmt, 2, 3);
    } else if(hex->type == HEX_TYPE_CORRIDOR) {
        render_doorway_segment(hex, cam, trifmt, 2, 3);
    }
    
    // Southwest wall (vertices 3->4)
    if(!(hex->connections & CONN_SOUTHWEST)) {
        render_wall_segment(hex, cam, trifmt, 3, 4);
    } else if(hex->type == HEX_TYPE_CORRIDOR) {
        render_doorway_segment(hex, cam, trifmt, 3, 4);
    }
}

// Render a single wall direction for depth sorting
void render_single_wall(hexagon_t* hex, int wall_dir, camera_t* cam, rdpq_trifmt_t* trifmt) {
    rdpq_set_prim_color(RGBA32(0, 255, 0, 255));  // Green wall
    
    switch(wall_dir) {
        case 2: // North wall (vertices 1->2)
            if(!(hex->connections & CONN_NORTH)) {
                render_wall_segment(hex, cam, trifmt, 1, 2);
            } else if(hex->type == HEX_TYPE_CORRIDOR) {
                render_doorway_segment(hex, cam, trifmt, 1, 2);
            }
            break;
        case 5: // South wall (vertices 4->5)
            if(!(hex->connections & CONN_SOUTH)) {
                render_wall_segment(hex, cam, trifmt, 4, 5);
            } else if(hex->type == HEX_TYPE_CORRIDOR) {
                render_doorway_segment(hex, cam, trifmt, 4, 5);
            }
            break;
        case 0: // Southeast wall (vertices 5->0)
            if(!(hex->connections & CONN_SOUTHEAST)) {
                render_wall_segment(hex, cam, trifmt, 5, 0);
            } else if(hex->type == HEX_TYPE_CORRIDOR) {
                render_doorway_segment(hex, cam, trifmt, 5, 0);
            }
            break;
        case 1: // Northeast wall (vertices 0->1)
            if(!(hex->connections & CONN_NORTHEAST)) {
                render_wall_segment(hex, cam, trifmt, 0, 1);
            } else if(hex->type == HEX_TYPE_CORRIDOR) {
                render_doorway_segment(hex, cam, trifmt, 0, 1);
            }
            break;
        case 3: // Northwest wall (vertices 2->3)
            if(!(hex->connections & CONN_NORTHWEST)) {
                render_wall_segment(hex, cam, trifmt, 2, 3);
            } else if(hex->type == HEX_TYPE_CORRIDOR) {
                render_doorway_segment(hex, cam, trifmt, 2, 3);
            }
            break;
        case 4: // Southwest wall (vertices 3->4)
            if(!(hex->connections & CONN_SOUTHWEST)) {
                render_wall_segment(hex, cam, trifmt, 3, 4);
            } else if(hex->type == HEX_TYPE_CORRIDOR) {
                render_doorway_segment(hex, cam, trifmt, 3, 4);
            }
            break;
    }
}

// Check collision with walls - returns 1 if collision detected, 0 if safe
int check_collision(float new_x, float new_z, float player_radius) {
    // Quick optimization: only check nearby hexagons for collision
    // (collision radius + hex size + some margin)
    float max_collision_dist_sq = (player_radius + 50.0f) * (player_radius + 50.0f);
    
    // Check against all hexagons and their walls
    for(int hex_i = 0; hex_i < MAP_HEX_COUNT; hex_i++) {
        hexagon_t* hex = &hexagons[hex_i];
        
        // Skip hexagons too far away for collision
        float dx = hex->center_x - new_x;
        float dz = hex->center_z - new_z;
        if(dx*dx + dz*dz > max_collision_dist_sq) continue;
        
        // Check each wall direction for this hexagon
        for(int wall_dir = 0; wall_dir < 6; wall_dir++) {
            // Only check walls that actually exist (no connections)
            int has_wall = 0;
            switch(wall_dir) {
                case 2: // North
                    has_wall = !(hex->connections & CONN_NORTH);
                    break;
                case 5: // South  
                    has_wall = !(hex->connections & CONN_SOUTH);
                    break;
                case 0: // Southeast
                    has_wall = !(hex->connections & CONN_SOUTHEAST);
                    break;
                case 1: // Northeast
                    has_wall = !(hex->connections & CONN_NORTHEAST);
                    break;
                case 3: // Northwest
                    has_wall = !(hex->connections & CONN_NORTHWEST);
                    break;
                case 4: // Southwest
                    has_wall = !(hex->connections & CONN_SOUTHWEST);
                    break;
            }
            
            // Check for doorway segments (corridors with connections)
            int has_doorway = 0;
            switch(wall_dir) {
                case 2: has_doorway = (hex->connections & CONN_NORTH) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 5: has_doorway = (hex->connections & CONN_SOUTH) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 0: has_doorway = (hex->connections & CONN_SOUTHEAST) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 1: has_doorway = (hex->connections & CONN_NORTHEAST) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 3: has_doorway = (hex->connections & CONN_NORTHWEST) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 4: has_doorway = (hex->connections & CONN_SOUTHWEST) && (hex->type == HEX_TYPE_CORRIDOR); break;
            }
            
            if(has_wall || has_doorway) {
                // Get wall endpoints based on direction
                int start_vert, end_vert;
                switch(wall_dir) {
                    case 2: start_vert = 1; end_vert = 2; break; // North
                    case 5: start_vert = 4; end_vert = 5; break; // South
                    case 0: start_vert = 5; end_vert = 0; break; // Southeast
                    case 1: start_vert = 0; end_vert = 1; break; // Northeast
                    case 3: start_vert = 2; end_vert = 3; break; // Northwest
                    case 4: start_vert = 3; end_vert = 4; break; // Southwest
                }
                
                if(has_wall) {
                    // Full wall collision
                    float wall_x1 = hex->vertices_x[start_vert];
                    float wall_z1 = hex->vertices_z[start_vert];
                    float wall_x2 = hex->vertices_x[end_vert];
                    float wall_z2 = hex->vertices_z[end_vert];
                    
                    if(point_to_line_distance(new_x, new_z, wall_x1, wall_z1, wall_x2, wall_z2) < player_radius) {
                        return 1; // Collision detected
                    }
                } else if(has_doorway) {
                    // Doorway collision - check doorframe segments only
                    if(check_doorframe_collision(hex, wall_dir, start_vert, end_vert, new_x, new_z, player_radius)) {
                        return 1; // Doorframe collision detected
                    }
                }
            }
        }
    }
    
    return 0; // No collision
}

// Advanced collision with wall sliding
int check_collision_with_slide(float old_x, float old_z, float *new_x, float *new_z, float player_radius) {
    // If no collision at target position, allow movement
    if(!check_collision(*new_x, *new_z, player_radius)) {
        return 0; // No collision, movement allowed
    }
    
    // Find the closest wall that's blocking us
    float closest_dist = 1000000.0f;
    float wall_x1, wall_z1, wall_x2, wall_z2;
    int found_wall = 0;
    
    float max_collision_dist_sq = (player_radius + 50.0f) * (player_radius + 50.0f);
    
    for(int hex_i = 0; hex_i < MAP_HEX_COUNT; hex_i++) {
        hexagon_t* hex = &hexagons[hex_i];
        
        // Skip hexagons too far away
        float dx = hex->center_x - *new_x;
        float dz = hex->center_z - *new_z;
        if(dx*dx + dz*dz > max_collision_dist_sq) continue;
        
        for(int wall_dir = 0; wall_dir < 6; wall_dir++) {
            // Check if this wall exists and is blocking
            int has_wall = 0;
            int has_doorway = 0;
            
            switch(wall_dir) {
                case 2: has_wall = !(hex->connections & CONN_NORTH); has_doorway = (hex->connections & CONN_NORTH) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 5: has_wall = !(hex->connections & CONN_SOUTH); has_doorway = (hex->connections & CONN_SOUTH) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 0: has_wall = !(hex->connections & CONN_SOUTHEAST); has_doorway = (hex->connections & CONN_SOUTHEAST) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 1: has_wall = !(hex->connections & CONN_NORTHEAST); has_doorway = (hex->connections & CONN_NORTHEAST) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 3: has_wall = !(hex->connections & CONN_NORTHWEST); has_doorway = (hex->connections & CONN_NORTHWEST) && (hex->type == HEX_TYPE_CORRIDOR); break;
                case 4: has_wall = !(hex->connections & CONN_SOUTHWEST); has_doorway = (hex->connections & CONN_SOUTHWEST) && (hex->type == HEX_TYPE_CORRIDOR); break;
            }
            
            if(has_wall || has_doorway) {
                int start_vert, end_vert;
                switch(wall_dir) {
                    case 2: start_vert = 1; end_vert = 2; break;
                    case 5: start_vert = 4; end_vert = 5; break;
                    case 0: start_vert = 5; end_vert = 0; break;
                    case 1: start_vert = 0; end_vert = 1; break;
                    case 3: start_vert = 2; end_vert = 3; break;
                    case 4: start_vert = 3; end_vert = 4; break;
                }
                
                float w_x1 = hex->vertices_x[start_vert];
                float w_z1 = hex->vertices_z[start_vert];
                float w_x2 = hex->vertices_x[end_vert];
                float w_z2 = hex->vertices_z[end_vert];
                
                // For doorways, check wall segments but NOT doorframes
                if(has_doorway) {
                    // Check left wall segment (first 33% - the actual wall)
                    float door_gap = 0.33f;
                    float wall_portion = (1.0f - door_gap) / 2.0f;
                    float left_end_x = w_x1 + wall_portion * (w_x2 - w_x1);
                    float left_end_z = w_z1 + wall_portion * (w_z2 - w_z1);
                    
                    float dist = point_to_line_distance(*new_x, *new_z, w_x1, w_z1, left_end_x, left_end_z);
                    if(dist < player_radius && dist < closest_dist) {
                        closest_dist = dist;
                        wall_x1 = w_x1; wall_z1 = w_z1; wall_x2 = left_end_x; wall_z2 = left_end_z;
                        found_wall = 1;
                    }
                    
                    // Check right wall segment (last 33% - the actual wall)
                    float right_wall_start = 1.0f - wall_portion;
                    float right_start_x = w_x1 + right_wall_start * (w_x2 - w_x1);
                    float right_start_z = w_z1 + right_wall_start * (w_z2 - w_z1);
                    
                    dist = point_to_line_distance(*new_x, *new_z, right_start_x, right_start_z, w_x2, w_z2);
                    if(dist < player_radius && dist < closest_dist) {
                        closest_dist = dist;
                        wall_x1 = right_start_x; wall_z1 = right_start_z; wall_x2 = w_x2; wall_z2 = w_z2;
                        found_wall = 1;
                    }
                    
                    // Skip the middle doorframe section - no collision there
                } else if(has_wall) {
                    // Check full wall for solid walls
                    float dist = point_to_line_distance(*new_x, *new_z, w_x1, w_z1, w_x2, w_z2);
                    if(dist < player_radius && dist < closest_dist) {
                        closest_dist = dist;
                        wall_x1 = w_x1; wall_z1 = w_z1; wall_x2 = w_x2; wall_z2 = w_z2;
                        found_wall = 1;
                    }
                }
            }
        }
    }
    
    if(!found_wall) {
        return 1; // Collision but no clear wall found, stop movement
    }
    
    // Calculate wall direction vector
    float wall_dx = wall_x2 - wall_x1;
    float wall_dz = wall_z2 - wall_z1;
    float wall_len = sqrtf(wall_dx*wall_dx + wall_dz*wall_dz);
    
    if(wall_len < 0.001f) {
        return 1; // Degenerate wall, stop movement
    }
    
    // Normalize wall direction
    wall_dx /= wall_len;
    wall_dz /= wall_len;
    
    // Movement vector
    float move_dx = *new_x - old_x;
    float move_dz = *new_z - old_z;
    
    // Project movement onto wall direction (slide component)
    float slide_amount = move_dx * wall_dx + move_dz * wall_dz;
    
    // Calculate slide position
    float slide_x = old_x + wall_dx * slide_amount;
    float slide_z = old_z + wall_dz * slide_amount;
    
    // Check if slide position is valid
    if(!check_collision(slide_x, slide_z, player_radius)) {
        *new_x = slide_x;
        *new_z = slide_z;
        return 0; // Sliding movement allowed
    }
    
    return 1; // Can't slide, stop movement
}

// Helper function: distance from point to line segment
float point_to_line_distance(float px, float pz, float x1, float z1, float x2, float z2) {
    // Vector from line start to end
    float line_dx = x2 - x1;
    float line_dz = z2 - z1;
    
    // Vector from line start to point
    float point_dx = px - x1;
    float point_dz = pz - z1;
    
    // Project point onto line
    float line_length_sq = line_dx * line_dx + line_dz * line_dz;
    
    if(line_length_sq < 0.001f) {
        // Line is actually a point, return distance to that point
        return sqrtf(point_dx * point_dx + point_dz * point_dz);
    }
    
    float t = (point_dx * line_dx + point_dz * line_dz) / line_length_sq;
    
    // Clamp t to [0,1] to stay on line segment
    if(t < 0.0f) t = 0.0f;
    if(t > 1.0f) t = 1.0f;
    
    // Find closest point on line segment
    float closest_x = x1 + t * line_dx;
    float closest_z = z1 + t * line_dz;
    
    // Return distance from point to closest point on line
    float dist_x = px - closest_x;
    float dist_z = pz - closest_z;
    return sqrtf(dist_x * dist_x + dist_z * dist_z);
}

// Check collision with doorframe segments
int check_doorframe_collision(hexagon_t* hex, int wall_dir, int v1_idx, int v2_idx, float new_x, float new_z, float player_radius) {
    // Calculate doorway dimensions (same as rendering logic)
    float door_gap = 0.33f;
    float wall_portion = (1.0f - door_gap) / 2.0f;
    
    // Left wall segment endpoints
    float left_end_world_x = hex->vertices_x[v1_idx] + wall_portion * (hex->vertices_x[v2_idx] - hex->vertices_x[v1_idx]);
    float left_end_world_z = hex->vertices_z[v1_idx] + wall_portion * (hex->vertices_z[v2_idx] - hex->vertices_z[v1_idx]);
    
    // Right wall segment endpoints  
    float right_wall_start = 1.0f - wall_portion;
    float right_start_world_x = hex->vertices_x[v1_idx] + right_wall_start * (hex->vertices_x[v2_idx] - hex->vertices_x[v1_idx]);
    float right_start_world_z = hex->vertices_z[v1_idx] + right_wall_start * (hex->vertices_z[v2_idx] - hex->vertices_z[v1_idx]);
    
    // Check collision with left wall segment
    if(point_to_line_distance(new_x, new_z, hex->vertices_x[v1_idx], hex->vertices_z[v1_idx], 
                             left_end_world_x, left_end_world_z) < player_radius) {
        return 1;
    }
    
    // Check collision with right wall segment
    if(point_to_line_distance(new_x, new_z, right_start_world_x, right_start_world_z,
                             hex->vertices_x[v2_idx], hex->vertices_z[v2_idx]) < player_radius) {
        return 1;
    }
    
    // Skip doorframe collision - only check wall segments, not the frame pieces
    
    return 0; // No doorframe collision
}

// Check if hexagon is within camera frustum (field of view)
int is_hexagon_in_frustum(hexagon_t* hex, camera_t* cam) {
    // Simple frustum culling based on angle from camera direction
    float dx = hex->center_x - cam->x;
    float dz = hex->center_z - cam->z;
    float dist_sq = dx*dx + dz*dz;
    
    // Always render very close hexagons (player might be standing on them)
    if(dist_sq < 2500.0f) return 1; // Within 50 units, always render
    
    // Camera forward direction
    float cam_forward_x = sinf(-cam->yaw_rad);
    float cam_forward_z = cosf(-cam->yaw_rad);
    
    // Calculate angle between camera forward and hexagon direction
    float hex_length = sqrtf(dist_sq);
    
    // Normalize hexagon direction
    float hex_dir_x = dx / hex_length;
    float hex_dir_z = dz / hex_length;
    
    // Dot product to get cosine of angle
    float cos_angle = cam_forward_x * hex_dir_x + cam_forward_z * hex_dir_z;
    
    // Very wide FOV to be safe - only cull hexagons clearly behind camera
    return cos_angle > -0.7f; // ~135 degree FOV (very wide)
}

// Combined visibility check: frustum + distance culling
int should_render_hexagon(hexagon_t* hex, camera_t* cam) {
    // Distance culling first (cheaper)
    float dx = hex->center_x - cam->x;
    float dz = hex->center_z - cam->z;
    float dist_sq = dx*dx + dz*dz;
    
    if(dist_sq > 160000.0f) return 0; // Too far (~400 units)
    
    // Frustum culling
    if(!is_hexagon_in_frustum(hex, cam)) return 0;
    
    return 1; // Passed all tests
}

// Get LOD level based on distance (0 = highest detail, 2 = lowest detail)
int get_hexagon_lod_level(hexagon_t* hex, camera_t* cam) {
    float dx = hex->center_x - cam->x;
    float dz = hex->center_z - cam->z;
    float dist_sq = dx*dx + dz*dz;
    
    if(dist_sq < 10000.0f) return 0;      // < 100 units: full detail
    if(dist_sq < 40000.0f) return 1;      // < 200 units: medium detail  
    return 2;                             // > 200 units: low detail
}

// Render floor with level of detail
void render_hexagon_floor_lod(hexagon_t* hex, camera_t* cam, rdpq_trifmt_t* trifmt, int lod_level) {
    // Project hexagon vertices to screen coordinates using floor projection
    screen_pos_t screen_pos[6];
    for(int i = 0; i < 6; i++) {
        screen_pos[i] = project_vertex_floor(hex->vertices_x[i], 0.0f, hex->vertices_z[i], cam);
    }
    
    // Set floor color (gray)
    rdpq_set_prim_color(RGBA32(128, 128, 128, 255));
    
    if(lod_level >= 2) {
        // LOD 2: Single quad (2 triangles) - very distant
        float v1[2] = { screen_pos[0].x, screen_pos[0].y };
        float v2[2] = { screen_pos[2].x, screen_pos[2].y };
        float v3[2] = { screen_pos[4].x, screen_pos[4].y };
        rdpq_triangle(trifmt, v1, v2, v3);
        
        float v4[2] = { screen_pos[0].x, screen_pos[0].y };
        float v5[2] = { screen_pos[3].x, screen_pos[3].y };
        float v6[2] = { screen_pos[4].x, screen_pos[4].y };
        rdpq_triangle(trifmt, v4, v5, v6);
    } else if(lod_level == 1) {
        // LOD 1: Reduced triangles (3 triangles) - medium distance
        float v1[2] = { screen_pos[0].x, screen_pos[0].y };
        float v2[2] = { screen_pos[2].x, screen_pos[2].y };
        float v3[2] = { screen_pos[4].x, screen_pos[4].y };
        rdpq_triangle(trifmt, v1, v2, v3);
        
        float v4[2] = { screen_pos[0].x, screen_pos[0].y };
        float v5[2] = { screen_pos[1].x, screen_pos[1].y };
        float v6[2] = { screen_pos[2].x, screen_pos[2].y };
        rdpq_triangle(trifmt, v4, v5, v6);
        
        float v7[2] = { screen_pos[0].x, screen_pos[0].y };
        float v8[2] = { screen_pos[4].x, screen_pos[4].y };
        float v9[2] = { screen_pos[5].x, screen_pos[5].y };
        rdpq_triangle(trifmt, v7, v8, v9);
    } else {
        // LOD 0: Full detail (4 triangles) - close distance
        // Use the original rendering code
        render_hexagon_floor(hex, cam, trifmt);
        return;
    }
}