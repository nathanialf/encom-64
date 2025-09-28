#ifndef HEXAGON_H
#define HEXAGON_H

#include <stdint.h>
#include "../generated/map_data.h"

// Hexagon object - pure geometry and data
typedef struct {
    float center_x, center_z;    // World position (converted from fixed-point)
    uint8_t connections;         // Connection bitmask from map data
    uint8_t type;               // Room/corridor type
    float vertices_x[6];        // Calculated world vertices
    float vertices_z[6];        // Calculated world vertices
} hexagon_t;

// Function prototypes
void hexagon_init(hexagon_t* hex, const hex_t* map_data);

#endif // HEXAGON_H