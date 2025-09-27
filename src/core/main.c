/*
 * ENCOM-64 - Nintendo 64 Dungeon Explorer
 * Main entry point and game loop
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <libdragon.h>

// Include generated map data
#include "map_data.h"

// Game constants
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define FPS_TARGET 30

// Game state
typedef struct {
    int32_t player_x, player_z;  // 16.16 fixed-point position
    uint16_t player_rotation;    // 0-65535 (0-360 degrees)
    uint8_t current_hex;         // Index into map_hexagons
    uint8_t debug_mode;          // Show debug info
} game_state_t;

static game_state_t game_state;
static display_context_t disp = 0;

// Function prototypes
void init_game(void);
void update_game(void);
void render_game(void);
void handle_input(void);
void draw_debug_info(void);

/*
 * Initialize the game
 */
void init_game(void) {
    // Initialize display
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTERS_RESAMPLE);
    
    // Initialize controller
    joypad_init();
    
    // Initialize timer for FPS control
    timer_init();
    
    // Initialize game state
    memset(&game_state, 0, sizeof(game_state_t));
    
    // Start player at first hex
    if (MAP_HEX_COUNT > 0) {
        game_state.player_x = map_hexagons[0].x_fixed;
        game_state.player_z = map_hexagons[0].z_fixed;
        game_state.current_hex = 0;
    }
    
    game_state.player_rotation = 0;
    game_state.debug_mode = 1;  // Start with debug on
    
    printf("ENCOM-64 Initialized\n");
    printf("Map: %d hexagons, seed: %s\n", MAP_HEX_COUNT, MAP_SEED);
    printf("Color index: %d\n", MAP_COLOR_INDEX);
}

/*
 * Handle controller input
 */
void handle_input(void) {
    joypad_poll();
    joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
    joypad_buttons_t keys_held = joypad_get_buttons_held(JOYPAD_PORT_1);
    joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
    
    // Debug toggle (Start button)
    if (keys.start) {
        game_state.debug_mode = !game_state.debug_mode;
    }
    
    // Movement with analog stick (if held)
    if (inputs.stick_x != 0 || inputs.stick_y != 0) {
        // Convert stick input to movement
        // stick_x: -80 to +80, stick_y: -80 to +80
        
        // Forward/backward (Y axis)
        int32_t forward_speed = (inputs.stick_y * 15 * 65536) / (80 * FPS_TARGET);
        
        // Strafe left/right (X axis)  
        int32_t strafe_speed = (inputs.stick_x * 15 * 65536) / (80 * FPS_TARGET);
        
        // Apply rotation to movement vector
        // For now, just move in world coordinates (no rotation)
        game_state.player_z += forward_speed;
        game_state.player_x += strafe_speed;
    }
    
    // Look around with C buttons
    if (keys_held.c_right) {
        game_state.player_rotation += 1000;  // Rotate right
    }
    if (keys_held.c_left) {
        game_state.player_rotation -= 1000;  // Rotate left
    }
    if (keys_held.c_up) {
        // Could be used for looking up/down in future
    }
    if (keys_held.c_down) {
        // Could be used for looking up/down in future
    }
}

/*
 * Update game logic
 */
void update_game(void) {
    // Handle input
    handle_input();
    
    // TODO: Add collision detection
    // TODO: Update current hex based on position
    // TODO: Add physics/movement smoothing
}

/*
 * Draw debug information
 */
void draw_debug_info(void) {
    if (!game_state.debug_mode) return;
    
    // Get colors from current palette
    uint16_t bright_color = GET_BRIGHT_COLOR();
    
    // Convert fixed-point positions to display
    int player_x_display = FIXED_TO_INT(game_state.player_x);
    int player_z_display = FIXED_TO_INT(game_state.player_z);
    
    // Draw debug text (white background for visibility)
    graphics_set_color(0xFFFF, 0x0000);  // White text, black background
    
    char text_buffer[64];
    
    graphics_draw_text(disp, 10, 10, "ENCOM-64 DEBUG");
    
    sprintf(text_buffer, "Pos: (%d, %d)", player_x_display, player_z_display);
    graphics_draw_text(disp, 10, 25, text_buffer);
    
    sprintf(text_buffer, "Rot: %d", game_state.player_rotation);
    graphics_draw_text(disp, 10, 40, text_buffer);
    
    sprintf(text_buffer, "Hex: %d/%d", game_state.current_hex, MAP_HEX_COUNT);
    graphics_draw_text(disp, 10, 55, text_buffer);
    
    sprintf(text_buffer, "Seed: %s", MAP_SEED);
    graphics_draw_text(disp, 10, 70, text_buffer);
    
    sprintf(text_buffer, "Color: 0x%04X", bright_color);
    graphics_draw_text(disp, 10, 85, text_buffer);
    
    graphics_draw_text(disp, 10, 210, "START: Toggle Debug");
    graphics_draw_text(disp, 10, 225, "Stick: Move, C: Look");
}

/*
 * Render the game
 */
void render_game(void) {
    // Wait for display
    disp = display_get();
    
    // Clear screen with dark color from current palette
    uint16_t dark_color = GET_DARK_COLOR();
    graphics_fill_screen(disp, dark_color);
    
    // TODO: Render 3D hex world
    // For now, just draw a simple representation
    
    // Get current palette colors
    uint16_t medium_color = GET_MEDIUM_COLOR();
    uint16_t bright_color = GET_BRIGHT_COLOR();
    
    // Draw a simple "hex" in the center to show we're working
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;
    int hex_size = 30;
    
    // Draw hex outline (simplified as circle for now)
    graphics_set_color(bright_color, 0x0000);
    for (int i = 0; i < 6; i++) {
        int angle = i * 60;
        int x1 = center_x + (hex_size * cos(angle * 3.14159 / 180));
        int y1 = center_y + (hex_size * sin(angle * 3.14159 / 180));
        int x2 = center_x + (hex_size * cos((angle + 60) * 3.14159 / 180));
        int y2 = center_y + (hex_size * sin((angle + 60) * 3.14159 / 180));
        
        graphics_draw_line(disp, x1, y1, x2, y2, bright_color);
    }
    
    // Draw debug information
    draw_debug_info();
    
    // Show the display
    display_show(disp);
}

/*
 * Main game loop
 */
int main(void) {
    // Initialize systems
    init_game();
    
    printf("Starting main game loop...\n");
    
    // Main game loop
    while (1) {
        // Target 30 FPS timing
        static uint32_t last_time = 0;
        uint32_t current_time = timer_ticks();
        uint32_t target_ticks = TICKS_PER_SECOND / FPS_TARGET;
        
        if (current_time - last_time >= target_ticks) {
            update_game();
            render_game();
            last_time = current_time;
        }
    }
    
    return 0;
}