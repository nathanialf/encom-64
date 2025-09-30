#include <stdio.h>
#include <math.h>
#include <libdragon.h>
#include <rdpq.h>
#include <rdpq_tri.h>
#include <rdpq_mode.h>
#include "../generated/map_data.h"
#include "hexagon.h"
#include "render.h"

static resolution_t res = RESOLUTION_320x240;
static bitdepth_t bit = DEPTH_32_BPP;

static int camera_yaw = 0;   // Horizontal rotation (integer degrees)
static float player_x = 0.0f;  // Player position in room
static float player_z = 0.0f;

// Hexagon objects for all map hexagons
hexagon_t hexagons[MAP_HEX_COUNT];


int main(void)
{
    /* Initialize peripherals */
    display_init( res, bit, 2, GAMMA_NONE, FILTERS_RESAMPLE );
    dfs_init( DFS_DEFAULT_LOCATION );
    joypad_init();
    rdpq_init();

    /* Initialize all hexagons from map data */
    for(int i = 0; i < MAP_HEX_COUNT; i++) {
        hexagon_init(&hexagons[i], &map_hexagons[i]);
    }


    /* Main loop test */
    while(1) 
    {
        char tStr[256];
        static display_context_t disp = 0;

        /* Grab a render buffer */
        disp = display_get();
       
        /*Fill the screen */
        graphics_fill_screen( disp, 0 );

        /* Handle analog stick input for camera yaw */
        joypad_poll();
        joypad_inputs_t joypad = joypad_get_inputs(JOYPAD_PORT_1);
        
        // Analog stick X controls yaw (left/right look)
        if(joypad.stick_x > 30 || joypad.stick_x < -30) {
            camera_yaw -= joypad.stick_x / 20;  // Right stick = clockwise (positive yaw)
            while(camera_yaw >= 360) camera_yaw -= 360;
            while(camera_yaw < 0) camera_yaw += 360;
        }
        
        // Analog stick Y controls forward/backward movement
        if(joypad.stick_y > 30 || joypad.stick_y < -30) {
            float move_speed = 1.25f;
            float yaw_rad = (camera_yaw * 3.14159f) / 180.0f;
            
            // Calculate proposed movement
            float movement = (joypad.stick_y / 128.0f) * move_speed;
            float new_x = player_x + sinf(-yaw_rad) * movement;
            float new_z = player_z + cosf(-yaw_rad) * movement;
            
            // Check collision with wall sliding (player radius = 3 units)
            if(!check_collision_with_slide(player_x, player_z, &new_x, &new_z, 3.0f)) {
                player_x = new_x;
                player_z = new_z;
            }
        }



        /* Render 3D hexagons with RDP triangles */
        // Setup RDP for triangle rendering (no Z-buffer for now)
        rdpq_attach(disp, NULL);
        rdpq_set_mode_fill(RGBA32(128, 0, 0, 255));  // Red background/skybox
        rdpq_fill_rectangle(0, 0, 320, 240);
        
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        
        // Camera parameters - following player position
        camera_t camera = {
            .x = player_x,              // Camera follows player X
            .y = 10.0f,                 // Eye level ABOVE the floor
            .z = player_z,              // Camera follows player Z
            .yaw_rad = (camera_yaw * 3.14159f) / 180.0f,  // Convert to radians
            .focal_length = 277.0f      // 60 degree FOV
        };
        
        // Define triangle format for flat shading (no Z-buffer)
        rdpq_trifmt_t trifmt = (rdpq_trifmt_t){
            .pos_offset = 0,
            .shade_offset = -1,  // No per-vertex shading
            .tex_offset = -1,    // No texture
            .z_offset = -1       // No Z-buffer
        };
        
        // Create array of hexagon indices with distances for depth sorting
        typedef struct {
            int index;
            float distance;
        } hexagon_distance_t;
        
        hexagon_distance_t hex_distances[MAP_HEX_COUNT];
        
        // Calculate squared distance from camera to each hexagon center
        // Skip very distant hexagons early to reduce sorting overhead
        int visible_hex_count = 0;
        for(int i = 0; i < MAP_HEX_COUNT; i++) {
            float dx = hexagons[i].center_x - camera.x;
            float dz = hexagons[i].center_z - camera.z;
            float dist_sq = dx*dx + dz*dz;
            
            // Only include hexagons within maximum render distance
            if(dist_sq <= 160000.0f) {
                hex_distances[visible_hex_count].index = i;
                hex_distances[visible_hex_count].distance = dist_sq;
                visible_hex_count++;
            }
        }
        
        // Simple bubble sort to sort hexagons by distance (far to near)
        // Only sort the visible hexagons
        for(int i = 0; i < visible_hex_count - 1; i++) {
            for(int j = 0; j < visible_hex_count - 1 - i; j++) {
                if(hex_distances[j].distance < hex_distances[j+1].distance) {
                    hexagon_distance_t temp = hex_distances[j];
                    hex_distances[j] = hex_distances[j+1];
                    hex_distances[j+1] = temp;
                }
            }
        }
        
        // Render ceilings first (back to front, farthest geometry)
        for(int i = 0; i < visible_hex_count; i++) {
            int hex_idx = hex_distances[i].index;
            // Combined visibility culling
            if(should_render_hexagon(&hexagons[hex_idx], &camera)) {
                render_hexagon_ceiling(&hexagons[hex_idx], &camera, &trifmt);
            }
        }
        
        // Render floors (back to front) with LOD
        for(int i = 0; i < visible_hex_count; i++) {
            int hex_idx = hex_distances[i].index;
            // Combined visibility culling  
            if(should_render_hexagon(&hexagons[hex_idx], &camera)) {
                int lod = get_hexagon_lod_level(&hexagons[hex_idx], &camera);
                render_hexagon_floor_lod(&hexagons[hex_idx], &camera, &trifmt, lod);
            }
        }
        
        // Collect all wall segments for depth sorting
        // Limit max wall segments to reduce sorting overhead
        #define MAX_WALL_SEGMENTS 100
        wall_segment_t wall_segments[MAX_WALL_SEGMENTS];
        int wall_count = 0;
        
        for(int hex_i = 0; hex_i < MAP_HEX_COUNT; hex_i++) {
            hexagon_t* hex = &hexagons[hex_i];
            
            // Skip hexagons that are not visible
            if(!should_render_hexagon(hex, &camera)) continue;
            
            // Calculate wall segment distances for each direction
            for(int wall_dir = 0; wall_dir < 6; wall_dir++) {
                // Only add walls that will actually render
                int should_render = 0;
                switch(wall_dir) {
                    case 2: // North
                        should_render = (!(hex->connections & CONN_NORTH)) || (hex->type == HEX_TYPE_CORRIDOR);
                        break;
                    case 5: // South
                        should_render = (!(hex->connections & CONN_SOUTH)) || (hex->type == HEX_TYPE_CORRIDOR);
                        break;
                    case 0: // Southeast
                        should_render = (!(hex->connections & CONN_SOUTHEAST)) || (hex->type == HEX_TYPE_CORRIDOR);
                        break;
                    case 1: // Northeast
                        should_render = (!(hex->connections & CONN_NORTHEAST)) || (hex->type == HEX_TYPE_CORRIDOR);
                        break;
                    case 3: // Northwest
                        should_render = (!(hex->connections & CONN_NORTHWEST)) || (hex->type == HEX_TYPE_CORRIDOR);
                        break;
                    case 4: // Southwest
                        should_render = (!(hex->connections & CONN_SOUTHWEST)) || (hex->type == HEX_TYPE_CORRIDOR);
                        break;
                }
                
                if(should_render) {
                    // Calculate squared distance (avoid expensive sqrt)
                    float wall_center_x = hex->center_x;
                    float wall_center_z = hex->center_z;
                    float dx = wall_center_x - camera.x;
                    float dz = wall_center_z - camera.z;
                    float dist_sq = dx*dx + dz*dz;
                    
                    // Skip walls that are too far away (distance culling)
                    if(dist_sq > 160000.0f) continue;  // ~400 unit cutoff
                    
                    // Only add if we have room (prioritize closer walls)
                    if(wall_count < MAX_WALL_SEGMENTS) {
                        wall_segments[wall_count].distance = dist_sq;
                        wall_segments[wall_count].hex = hex;
                        wall_segments[wall_count].wall_dir = wall_dir;
                        wall_count++;
                    }
                }
            }
        }
        
        // Sort wall segments by distance (far to near) using insertion sort
        for(int i = 1; i < wall_count; i++) {
            wall_segment_t key = wall_segments[i];
            int j = i - 1;
            
            // Move elements that are closer than key to one position ahead
            while(j >= 0 && wall_segments[j].distance < key.distance) {
                wall_segments[j + 1] = wall_segments[j];
                j--;
            }
            wall_segments[j + 1] = key;
        }
        
        // Render wall segments in depth order
        for(int i = 0; i < wall_count; i++) {
            render_single_wall(wall_segments[i].hex, wall_segments[i].wall_dir, &camera, &trifmt);
        }
        
        rdpq_detach();

        // Draw debug text after RDP operations
        sprintf(tStr, "Map: %s (%d hexes)\n", MAP_SEED, MAP_HEX_COUNT);
        graphics_draw_text( disp, 20, 20, tStr );
        sprintf(tStr, "Yaw: %d, Pos: %.1f,%.1f\n", camera_yaw, player_x, player_z);
        graphics_draw_text( disp, 20, 30, tStr );
        sprintf(tStr, "Stick X: %d, Y: %d\n", joypad.stick_x, joypad.stick_y);
        graphics_draw_text( disp, 20, 40, tStr );

        display_show(disp);

        /* Do we need to switch video displays? */
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        if( keys.d_up )
        {
            display_close();

            res = RESOLUTION_640x480;
            display_init( res, bit, 2, GAMMA_NONE, FILTERS_DISABLED );
        }

        if( keys.d_down )
        {
            display_close();

            res = RESOLUTION_320x240;
            display_init( res, bit, 2, GAMMA_NONE, FILTERS_RESAMPLE );
        }

        if( keys.d_left )
        {
            display_close();

            bit = DEPTH_16_BPP;
            // Use FILTERS_RESAMPLE for 320x240, FILTERS_DISABLED for higher res
            if(res.width <= 320) {
                display_init( res, bit, 2, GAMMA_NONE, FILTERS_RESAMPLE );
            } else {
                display_init( res, bit, 2, GAMMA_NONE, FILTERS_DISABLED );
            }
        }

        if( keys.d_right )
        {
            display_close();

            bit = DEPTH_32_BPP;
            // Use FILTERS_RESAMPLE for 320x240, FILTERS_DISABLED for higher res
            if(res.width <= 320) {
                display_init( res, bit, 2, GAMMA_NONE, FILTERS_RESAMPLE );
            } else {
                display_init( res, bit, 2, GAMMA_NONE, FILTERS_DISABLED );
            }
        }
    }
}
