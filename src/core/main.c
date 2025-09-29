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
static hexagon_t hexagons[MAP_HEX_COUNT];


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
            
            // Move forward/backward based on camera direction
            // Forward stick (negative Y) should move in camera facing direction
            // Backward stick (positive Y) should move opposite to camera facing direction
            float movement = (joypad.stick_y / 128.0f) * move_speed;  // Removed negative sign to fix direction
            
            player_x += sinf(-yaw_rad) * movement;  // East/West component  
            player_z += cosf(-yaw_rad) * movement;  // North/South component
            
            // No artificial boundary - walls will stop us later
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
        
        // Calculate distance from camera to each hexagon center
        for(int i = 0; i < MAP_HEX_COUNT; i++) {
            float dx = hexagons[i].center_x - camera.x;
            float dz = hexagons[i].center_z - camera.z;
            hex_distances[i].index = i;
            hex_distances[i].distance = sqrtf(dx*dx + dz*dz);
        }
        
        // Simple bubble sort to sort hexagons by distance (far to near)
        for(int i = 0; i < MAP_HEX_COUNT - 1; i++) {
            for(int j = 0; j < MAP_HEX_COUNT - 1 - i; j++) {
                if(hex_distances[j].distance < hex_distances[j+1].distance) {
                    hexagon_distance_t temp = hex_distances[j];
                    hex_distances[j] = hex_distances[j+1];
                    hex_distances[j+1] = temp;
                }
            }
        }
        
        // Render all hexagons from farthest to nearest (painter's algorithm)
        for(int i = 0; i < MAP_HEX_COUNT; i++) {
            int hex_idx = hex_distances[i].index;
            render_hexagon_floor(&hexagons[hex_idx], &camera, &trifmt);
            render_hexagon_pillars(&hexagons[hex_idx], &camera, &trifmt);
            render_hexagon_walls(&hexagons[hex_idx], &camera, &trifmt);
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
