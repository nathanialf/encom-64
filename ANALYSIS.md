# ENCOM-DUNGEON Analysis for N64 Port

## Color Scheme Implementation

**Source**: `/src/components/effects/TerminalGreen.tsx`

### Hash-to-Color System
- Map seed is hashed to 32-bit integer: `hash = ((hash << 5) - hash) + char`
- Color selection: `colorIndex = (seedValue % 5) * 3`
- 5 terminal color palettes with 3 shades each (dark, medium, bright)

### Color Palettes
```c
// For N64 - use RGB565 or vertex colors
// Green (index 0): #00b300
{ 0x00, 0x1A, 0x00 },    // dark green
{ 0x00, 0x66, 0x1A },    // medium green  
{ 0x00, 0xB3, 0x26 },    // bright green

// Purple (index 1): #8500ad
{ 0x1A, 0x00, 0x1A },    // dark purple
{ 0x4D, 0x00, 0x66 },    // medium purple
{ 0x85, 0x00, 0xAD },    // bright purple

// Teal (index 2): #018c6c
{ 0x00, 0x1A, 0x14 },    // dark teal
{ 0x00, 0x4D, 0x40 },    // medium teal
{ 0x01, 0x8C, 0x6B },    // bright teal

// Red (index 3): #8b0000
{ 0x1A, 0x00, 0x00 },    // dark red
{ 0x4D, 0x00, 0x00 },    // medium red
{ 0x8B, 0x00, 0x00 },    // bright red

// Amber (index 4): #d53600
{ 0x1A, 0x05, 0x00 },    // dark amber
{ 0x80, 0x1F, 0x00 },    // medium amber
{ 0xD5, 0x36, 0x00 }     // bright amber
```

## Hexagon to 3D Conversion Logic

**Source**: `/src/utils/hexUtils.ts`

### Key Constants
```c
#define HEX_SIZE 25
#define HEX_HEIGHT_SCALE 12
```

### Coordinate Conversion (Flat-top hexagons)
```c
// hexToPosition function
float x = HEX_SIZE * (1.5f * q);
float z = HEX_SIZE * (SQRT3_2 * q + SQRT3 * r);
float y = 0; // base level

// Constants needed
#define SQRT3 1.732050808f
#define SQRT3_2 0.866025404f
```

### Neighbor Directions (6 directions)
```c
// Wall directions for flat-top hex
typedef enum {
    SOUTHEAST = 0,  // [+1, 0]
    NORTHEAST = 1,  // [+1, -1] 
    NORTH = 2,      // [0, -1]
    NORTHWEST = 3,  // [-1, 0]
    SOUTHWEST = 4,  // [-1, +1]
    SOUTH = 5       // [0, +1]
} wall_direction_t;
```

## Wall Rendering System

**Source**: `/src/components/HexTile.tsx`

### Geometry Analysis

#### Per Hex Polygon Count:
- **Floor**: CylinderGeometry(radius, radius, 0.1, 6) = **12 triangles** (6 sides + 2 caps)
- **Ceiling**: CylinderGeometry(radius, radius, 0.1, 6) = **12 triangles** (if walkable)
- **Walls** (up to 6): BoxGeometry per wall = **12 triangles each** (6 faces × 2 triangles)
- **Doorway**: 4 meshes per doorway = **48 triangles** (2 wall segments + 2 door frames)

#### Total Per Hex (worst case):
- Floor: 12 triangles
- Ceiling: 12 triangles  
- 6 Walls: 72 triangles
- **Total: ~96 triangles per hex maximum**

#### Optimizations for N64:
- Use simpler hex floor/ceiling: **6 triangles** instead of 12
- Walls as single quads: **2 triangles** per wall instead of 12
- Remove doorframes: Use simple openings
- **Target: 20-30 triangles per hex**

### Wall Positioning (6 walls per hex)
```c
// Wall positions relative to hex center
typedef struct {
    float x, y, z;
    float rotation_y;
} wall_pos_t;

wall_pos_t wall_positions[6] = {
    // North wall
    { 0, height/2, -radius * 0.866f, 0 },
    // South wall  
    { 0, height/2, radius * 0.866f, 0 },
    // Northeast wall
    { radius * 0.75f, height/2, -radius * 0.433f, -PI/3 },
    // Southeast wall
    { radius * 0.75f, height/2, radius * 0.433f, PI/3 },
    // Southwest wall
    { -radius * 0.75f, height/2, radius * 0.433f, -PI/3 },
    // Northwest wall
    { -radius * 0.75f, height/2, -radius * 0.433f, PI/3 }
};
```

## Movement Mechanics

**Source**: `/src/components/FirstPersonController.tsx`, `/src/utils/collisionUtils.ts`

### Movement Constants
```c
#define MOVE_SPEED 15
#define DAMPING 10
#define PLAYER_RADIUS 1.5f
#define POSITION_THRESHOLD 5      // State update threshold
#define ROTATION_THRESHOLD 0.2f   // Rotation threshold
```

### Control Scheme
- **WASD**: Forward/back, strafe left/right
- **Mouse**: Look around (horizontal/vertical)
- **Touch**: Virtual joystick + touch look

### Collision System
- **Circle vs. Box**: Player (circle) vs. wall boxes
- **Sliding**: Damped collision response prevents sticking
- **Optimization**: Only check nearby hexes for collision

### N64 Control Mapping
```c
// Suggested N64 controller mapping
// Analog stick: Movement (forward/back, strafe)
// C-Buttons: Camera look (up/down/left/right)  
// Start: Debug menu
// Z-trigger: Future use
```

## Performance Analysis for N64

### Current Complexity (10 hexagon map):
- **Triangles**: 10 × 96 = **960 triangles** (worst case)
- **Optimized**: 10 × 25 = **250 triangles** (target)

### N64 Performance Targets:
- **Resolution**: 320×240
- **Frame Rate**: 30 FPS
- **RAM**: 4MB (8MB expansion if needed)
- **Triangle Rate**: ~150,000 triangles/second at 30fps = **5,000 triangles/frame budget**

### Optimization Strategy:
1. **Simplified Geometry**:
   - Hex floors: 6 triangles (fan triangulation)
   - Walls: 2 triangles (single quad)
   - Remove complex doorframes
   
2. **Fixed-Point Math**:
   - Use 16.16 fixed-point for positions
   - Pre-calculated trigonometry tables
   
3. **Display Lists**:
   - Pre-compile static geometry at build time
   - One display list per hex type
   
4. **Culling**:
   - Distance culling (similar to current 300 unit radius)
   - Frustum culling
   - Backface culling

## API Integration

**Current Endpoints**:
- Dev: `https://encom-api-dev.riperoni.com/api/v1/map/generate`
- Prod: `https://encom-api.riperoni.com/api/v1/map/generate`

**Build-time Integration**:
- Fetch map data during ROM build
- Convert JSON to C structures
- Embed in ROM as static data

## Memory Layout Estimate

```c
// 10-hex map data structures
typedef struct {
    uint8_t q, r;           // Hex coordinates (2 bytes)
    uint8_t type;           // ROOM/CORRIDOR (1 byte)  
    uint8_t connections;    // Bitmask of 6 directions (1 byte)
    uint8_t height;         // Height level (1 byte)
    uint8_t wall_mask;      // Which walls exist (1 byte)
    // Total: 6 bytes per hex
} hex_data_t;

// 10 hexes × 6 bytes = 60 bytes map data
// Display lists: ~2KB per hex type
// Textures: Minimal (vertex colors)
// Code: ~100KB estimated
// Total ROM: <1MB
```

## Next Steps for N64 Port

1. **Setup libdragon SDK** 
2. **Create build system** for API fetch → C conversion → ROM
3. **Implement basic hex rendering** with simplified geometry
4. **Port color system** to vertex colors/simple textures
5. **Add movement controls** with N64 controller
6. **Optimize for 30 FPS target**

## Key Simplifications for N64

- **No post-processing effects** (terminal shader)
- **Vertex colors only** (no complex textures)
- **Simplified wall geometry** (quads instead of boxes)
- **No real-time lighting** (pre-baked vertex colors)
- **Static maps** (no dynamic generation)
- **10 hex limit initially** (expand if performance allows)

The current ENCOM-DUNGEON implementation provides an excellent reference, but significant simplification will be needed to meet N64 hardware constraints while maintaining the core visual style and gameplay.