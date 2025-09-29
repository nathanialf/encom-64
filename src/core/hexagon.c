#include "hexagon.h"

// Standard hexagon vertices relative to center (flat-top orientation)
static const float hex_template_x[6] = { 50.0f, 25.0f, -25.0f, -50.0f, -25.0f, 25.0f };
static const float hex_template_z[6] = { 0.0f, 43.0f, 43.0f, 0.0f, -43.0f, -43.0f };

// Initialize hexagon from map data
void hexagon_init(hexagon_t* hex, const hex_t* map_data) {
    // Calculate proper center position from q, r hexagonal grid coordinates  
    // For touching FLAT-TOP hexagons (our template is flat-top)
    float radius = 50.0f;  // Our hexagon radius (center to vertex)
    
    // For flat-top hexagons:
    // - Width (point to point) = 2 * radius = 100
    // - Height (flat to flat) = sqrt(3) * radius = 86.6
    // - Horizontal spacing between centers = 1.5 * radius = 75
    // - Vertical spacing between centers = sqrt(3) * radius = 86.6
    
    float spacing_x = 1.5f * radius;        // 75 units (horizontal center spacing)
    float spacing_z = 1.732f * radius;      // ~86.6 units (vertical center spacing)
    
    // Convert q, r to world coordinates (axial to cartesian for FLAT-TOP)
    // Mirror along Z axis only to fix map orientation
    hex->center_x = spacing_x * map_data->q;
    hex->center_z = -spacing_z * (map_data->r + map_data->q * 0.5f);
    
    hex->connections = map_data->connections;
    hex->type = map_data->type;
    
    // Calculate world vertices
    for(int i = 0; i < 6; i++) {
        hex->vertices_x[i] = hex->center_x + hex_template_x[i];
        hex->vertices_z[i] = hex->center_z + hex_template_z[i];
    }
}