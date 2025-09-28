#include <stdio.h>
#include <math.h>
#include <libdragon.h>
#include <rdpq.h>
#include <rdpq_tri.h>
#include <rdpq_mode.h>
#include "../generated/map_data.h"

static resolution_t res = RESOLUTION_320x240;
static bitdepth_t bit = DEPTH_32_BPP;

static int camera_yaw = 0;   // Horizontal rotation (integer degrees)
static float player_x = 0.0f;  // Player position in room
static float player_z = 0.0f;


int main(void)
{
    /* Initialize peripherals */
    display_init( res, bit, 2, GAMMA_NONE, FILTERS_RESAMPLE );
    dfs_init( DFS_DEFAULT_LOCATION );
    joypad_init();
    rdpq_init();


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



        /* Render 3D hexagon floor with RDP triangles */
        // Setup RDP for triangle rendering
        rdpq_attach(disp, NULL);
        rdpq_set_mode_fill(RGBA32(128, 0, 0, 255));  // Red background/skybox
        rdpq_fill_rectangle(0, 0, 320, 240);
        
        rdpq_set_mode_standard();
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        
        // 3D hexagon vertices (world coordinates) - room-sized floor
        static const float hex_world_x[6] = { 50.0f, 25.0f, -25.0f, -50.0f, -25.0f, 25.0f };
        static const float hex_world_z[6] = { 0.0f, 43.0f, 43.0f, 0.0f, -43.0f, -43.0f };
        static const float hex_world_y = 0.0f;  // Floor level 
        
        // Camera parameters - following player position
        float camera_x = player_x;  // Camera follows player X
        float camera_y = 10.0f;     // Eye level ABOVE the floor
        float camera_z = player_z;  // Camera follows player Z
        float focal_length = 277.0f;  // 60 degree FOV
        
        // Convert yaw to radians (no pitch - looking straight forward)
        float yaw_rad = (camera_yaw * 3.14159f) / 180.0f;
        
        // Project 3D vertices to 2D screen coordinates
        float screen_x[6], screen_y[6];
        for(int i = 0; i < 6; i++) {
            // Hexagon vertices in world space (fixed)
            float world_x = hex_world_x[i];
            float world_y = hex_world_y;
            float world_z = hex_world_z[i];
            
            // Translate relative to camera position
            float rel_x = world_x - camera_x;
            float rel_y = world_y - camera_y;  // Floor is below camera
            float rel_z = world_z - camera_z;
            
            // Rotate the view around the camera (inverse rotation)
            float view_x = rel_x * cosf(-yaw_rad) - rel_z * sinf(-yaw_rad);
            float view_z = rel_x * sinf(-yaw_rad) + rel_z * cosf(-yaw_rad);
            
            // Small Z offset to ensure positive depth for projection stability
            view_z += 10.0f;
            
            // 3D to 2D projection (no pitch - looking straight forward)
            // Always calculate projection, even if result goes offscreen
            if(view_z > 0.001f) {  // Just prevent division by zero
                screen_x[i] = 160.0f + (view_x * focal_length) / view_z;
                screen_y[i] = 120.0f - (rel_y * focal_length) / view_z;  // Below camera = below horizon
            } else {
                // Very close to zero - use small value to prevent division by zero
                screen_x[i] = 160.0f + (view_x * focal_length) / 0.001f;
                screen_y[i] = 120.0f - (rel_y * focal_length) / 0.001f;
            }
            
            // Clamp to reasonable offscreen bounds to prevent RDP issues
            if(screen_x[i] < -500.0f) screen_x[i] = -500.0f;
            if(screen_x[i] > 820.0f) screen_x[i] = 820.0f;
            if(screen_y[i] < -500.0f) screen_y[i] = -500.0f;
            if(screen_y[i] > 740.0f) screen_y[i] = 740.0f;
        }
        
        
        // Define triangle format for flat shading
        rdpq_trifmt_t trifmt = (rdpq_trifmt_t){
            .pos_offset = 0,
            .shade_offset = -1,  // No per-vertex shading
            .tex_offset = -1,    // No texture
            .z_offset = -1       // No Z-buffer
        };
        
        // Set fill color (gray)
        rdpq_set_prim_color(RGBA32(128, 128, 128, 255));
        
        // Draw triangles forming flat hexagon plane (use better triangulation)
        // Draw as 4 triangles instead of 6 triangles from center
        // This avoids always having lines going to screen center
        
        // Triangle 1: vertices 0, 1, 2
        float v1[2] = { screen_x[0], screen_y[0] };
        float v2[2] = { screen_x[1], screen_y[1] };
        float v3[2] = { screen_x[2], screen_y[2] };
        rdpq_triangle(&trifmt, v1, v2, v3);
        
        // Triangle 2: vertices 0, 2, 3  
        float v4[2] = { screen_x[0], screen_y[0] };
        float v5[2] = { screen_x[2], screen_y[2] };
        float v6[2] = { screen_x[3], screen_y[3] };
        rdpq_triangle(&trifmt, v4, v5, v6);
        
        // Triangle 3: vertices 0, 3, 4
        float v7[2] = { screen_x[0], screen_y[0] };
        float v8[2] = { screen_x[3], screen_y[3] };
        float v9[2] = { screen_x[4], screen_y[4] };
        rdpq_triangle(&trifmt, v7, v8, v9);
        
        // Triangle 4: vertices 0, 4, 5
        float v10[2] = { screen_x[0], screen_y[0] };
        float v11[2] = { screen_x[4], screen_y[4] };
        float v12[2] = { screen_x[5], screen_y[5] };
        rdpq_triangle(&trifmt, v10, v11, v12);
        
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
            display_init( res, bit, 2, GAMMA_NONE, FILTERS_DISABLED );
        }

        if( keys.d_left )
        {
            display_close();

            bit = DEPTH_16_BPP;
            display_init( res, bit, 2, GAMMA_NONE, FILTERS_DISABLED );
        }

        if( keys.d_right )
        {
            display_close();

            bit = DEPTH_32_BPP;
            display_init( res, bit, 2, GAMMA_NONE, FILTERS_DISABLED );
        }
    }
}
