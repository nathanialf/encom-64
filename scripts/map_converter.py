#!/usr/bin/env python3
"""
ENCOM-64 Map Data Converter
Converts JSON map data from ENCOM API to C header files for N64 ROM.
"""

import json
import sys
import argparse
from typing import Dict, List, Any
import hashlib


def hash_string(s: str) -> int:
    """Hash string to 32-bit integer using same algorithm as ENCOM-DUNGEON"""
    hash_val = 0
    for char in s:
        hash_val = ((hash_val << 5) - hash_val) + ord(char)
        hash_val = hash_val & 0xFFFFFFFF  # Convert to 32-bit
    return hash_val


def get_color_index(seed: str) -> int:
    """Get color palette index (0-4) from seed string"""
    if not seed:
        return 0
    hash_val = hash_string(seed)
    return (hash_val % 5) * 3  # Multiply by 3 for palette offset


def convert_hex_coordinate(hex_data: Dict[str, Any]) -> tuple:
    """Convert hex coordinate to fixed-point position"""
    q = hex_data.get('q', 0)
    r = hex_data.get('r', 0)
    
    # From ENCOM-DUNGEON hexUtils.ts
    HEX_SIZE = 25
    SQRT3_2 = 0.866025404
    SQRT3 = 1.732050808
    
    x = HEX_SIZE * (1.5 * q)
    z = HEX_SIZE * (SQRT3_2 * q + SQRT3 * r)
    
    # Convert to 16.16 fixed-point
    x_fixed = int(x * 65536)
    z_fixed = int(z * 65536)
    
    return x_fixed, z_fixed


def analyze_connections(hexagons: List[Dict[str, Any]]) -> Dict[str, int]:
    """Build connection bitmasks for each hex"""
    hex_map = {hex_data['id']: hex_data for hex_data in hexagons}
    coord_map = {f"{hex_data['q']},{hex_data['r']}": hex_data for hex_data in hexagons}
    
    # Direction offsets for flat-top hex (from ENCOM-DUNGEON)
    directions = [
        (1, 0),   # southeast
        (1, -1),  # northeast  
        (0, -1),  # north
        (-1, 0),  # northwest
        (-1, 1),  # southwest
        (0, 1),   # south
    ]
    
    connection_masks = {}
    
    for hex_data in hexagons:
        mask = 0
        q, r = hex_data['q'], hex_data['r']
        
        for i, (dq, dr) in enumerate(directions):
            neighbor_q = q + dq
            neighbor_r = r + dr
            neighbor_key = f"{neighbor_q},{neighbor_r}"
            
            if neighbor_key in coord_map:
                neighbor = coord_map[neighbor_key]
                if neighbor['id'] in hex_data.get('connections', []):
                    mask |= (1 << i)
        
        connection_masks[hex_data['id']] = mask
    
    return connection_masks


def generate_header(json_data: Dict[str, Any], output_path: str):
    """Generate C header file from JSON map data"""
    
    hexagons = json_data.get('hexagons', [])
    metadata = json_data.get('metadata', {})
    
    seed = metadata.get('seed', '')
    color_index = get_color_index(seed)
    connection_masks = analyze_connections(hexagons)
    
    header_content = f'''/*
 * ENCOM-64 Generated Map Data
 * Auto-generated from API response - DO NOT EDIT
 */

#ifndef MAP_DATA_H
#define MAP_DATA_H

#include <stdint.h>

// Map metadata
#define MAP_SEED "{seed}"
#define MAP_HEX_COUNT {len(hexagons)}
#define MAP_COLOR_INDEX {color_index}
#define MAP_TOTAL_HEXAGONS {metadata.get('totalHexagons', len(hexagons))}
#define MAP_ROOMS {metadata.get('rooms', 0)}
#define MAP_CORRIDORS {metadata.get('corridors', 0)}

// Hex constants (from ENCOM-DUNGEON analysis)
#define HEX_SIZE 25
#define HEX_HEIGHT_SCALE 12

// Fixed-point math (16.16)
#define FIXED_POINT_SHIFT 16
#define INT_TO_FIXED(x) ((x) << FIXED_POINT_SHIFT)
#define FIXED_TO_INT(x) ((x) >> FIXED_POINT_SHIFT)

// Hex types
typedef enum {{
    HEX_TYPE_ROOM = 0,
    HEX_TYPE_CORRIDOR = 1
}} hex_type_t;

// Connection directions (bitmask)
#define CONN_SOUTHEAST  (1 << 0)
#define CONN_NORTHEAST  (1 << 1) 
#define CONN_NORTH      (1 << 2)
#define CONN_NORTHWEST  (1 << 3)
#define CONN_SOUTHWEST  (1 << 4)
#define CONN_SOUTH      (1 << 5)

// Hex data structure
typedef struct {{
    int8_t q, r;              // Hex coordinates
    int32_t x_fixed, z_fixed; // 16.16 fixed-point world position
    uint8_t type;             // hex_type_t
    uint8_t height;           // Height level (0-255)
    uint8_t connections;      // Connection bitmask
    uint8_t is_walkable;      // 0 or 1
}} hex_t;

// Map data array
static const hex_t map_hexagons[MAP_HEX_COUNT] = {{
'''

    # Generate hex data
    for i, hex_data in enumerate(hexagons):
        q = hex_data.get('q', 0)
        r = hex_data.get('r', 0)
        x_fixed, z_fixed = convert_hex_coordinate(hex_data)
        
        hex_type = 1 if hex_data.get('type') == 'CORRIDOR' else 0
        height = min(255, max(0, int(hex_data.get('height', 1) * 12)))  # Scale to 0-255
        connections = connection_masks.get(hex_data['id'], 0)
        is_walkable = 1 if hex_data.get('isWalkable', True) else 0
        
        header_content += f'''    {{ {q:2d}, {r:2d}, {x_fixed:8d}, {z_fixed:8d}, {hex_type}, {height:3d}, 0x{connections:02X}, {is_walkable} }}'''
        
        if i < len(hexagons) - 1:
            header_content += ','
        header_content += f'  // {hex_data.get("id", f"hex-{i}")}\n'
    
    header_content += '''};

// Color palette data (RGB565 format for N64)
static const uint16_t color_palettes[5][3] = {
    // Green palette
    { 0x0340, 0x0660, 0x05A0 },  // dark, medium, bright green
    // Purple palette  
    { 0x4004, 0x9009, 0xA80D },  // dark, medium, bright purple
    // Teal palette
    { 0x0141, 0x0281, 0x158C },  // dark, medium, bright teal
    // Red palette
    { 0x4000, 0x9000, 0xA000 },  // dark, medium, bright red
    // Amber palette
    { 0x4080, 0x8140, 0xD340 }   // dark, medium, bright amber
};

// Get current palette colors
#define GET_DARK_COLOR()   (color_palettes[MAP_COLOR_INDEX / 3][0])
#define GET_MEDIUM_COLOR() (color_palettes[MAP_COLOR_INDEX / 3][1]) 
#define GET_BRIGHT_COLOR() (color_palettes[MAP_COLOR_INDEX / 3][2])

#endif // MAP_DATA_H
'''
    
    # Write to file
    with open(output_path, 'w') as f:
        f.write(header_content)
    
    print(f"Generated map header: {output_path}")
    print(f"  Hexagons: {len(hexagons)}")
    print(f"  Seed: {seed}")
    print(f"  Color Index: {color_index}")


def main():
    parser = argparse.ArgumentParser(description='Convert ENCOM map JSON to C header')
    parser.add_argument('input_json', help='Input JSON file from ENCOM API')
    parser.add_argument('output_header', help='Output C header file')
    
    args = parser.parse_args()
    
    try:
        with open(args.input_json, 'r') as f:
            json_data = json.load(f)
        
        generate_header(json_data, args.output_header)
        
    except FileNotFoundError:
        print(f"ERROR: Input file '{args.input_json}' not found")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"ERROR: Invalid JSON in '{args.input_json}': {e}")
        sys.exit(1)
    except Exception as e:
        print(f"ERROR: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()