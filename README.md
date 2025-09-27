# ENCOM-64

Nintendo 64 port of ENCOM Dungeon Explorer - a first-person hexagonal dungeon crawler that runs as a native N64 ROM.

## Overview

ENCOM-64 brings the ENCOM dungeon exploration experience to the Nintendo 64, featuring:
- **Procedurally generated hex-grid dungeons** from the ENCOM API
- **Terminal color aesthetics** with 5 seed-based color palettes
- **First-person exploration** with N64 controller support
- **Optimized for N64 hardware** (4MB RAM, 30 FPS target)

## Architecture

### Build System
- **Jenkins Pipeline**: Automated toolchain setup, map fetching, and ROM building
- **libdragon SDK**: Modern N64 homebrew development framework
- **Map Integration**: Build-time fetching from ENCOM API and conversion to C data structures

### Technical Specifications
- **Resolution**: 320×240
- **Frame Rate**: 30 FPS target
- **Memory**: 4MB RAM (expandable to 8MB if needed)
- **Map Size**: 10 hexagons initially (scalable based on performance)
- **ROM Format**: Z64 (byte-swapped)

## Development Setup

### Prerequisites
- Debian/Ubuntu Linux environment
- Jenkins with aws-encom-dev credentials
- Build dependencies: `build-essential git libpng-dev libpixman-1-dev pkg-config cmake`

### Jenkins Pipeline Build
The project is designed for Jenkins automation:

1. **Setup Toolchain**: Downloads and builds libdragon SDK (cached between builds)
2. **Fetch Map Data**: Calls ENCOM API with small map request (10 hexagons)
3. **Generate Headers**: Converts JSON to C structures with fixed-point math
4. **Build ROM**: Compiles and links N64 ROM using MIPS toolchain
5. **Run Tests**: Validates ROM integrity and size constraints
6. **Store Artifacts**: Archives ROM with build metadata

### Manual Development
```bash
# Clone project (done by Jenkins)
git clone <repo> encom-64
cd encom-64

# Setup toolchain (automated in Jenkins pipeline)
mkdir -p tools
cd tools
git clone https://github.com/DragonMinded/libdragon.git
cd libdragon && ./build.sh

# Fetch and convert map data
curl -X POST https://encom-api-dev.riperoni.com/api/v1/map/generate \
     -H "Content-Type: application/json" \
     -d '{"hexagonCount": 10}' \
     -o map_response.json

python3 scripts/map_converter.py map_response.json src/generated/map_data.h

# Build ROM
export N64_INST=${PWD}/tools/libdragon
export PATH=${N64_INST}/bin:${PATH}
make
```

## Project Structure

```
encom-64/
├── src/
│   ├── core/                 # Core game code
│   │   └── main.c           # Main entry point and game loop
│   └── generated/           # Generated map data (created by build)
│       └── map_data.h       # Converted map structures
├── scripts/
│   └── map_converter.py     # JSON to C converter
├── tools/                   # libdragon SDK (downloaded by Jenkins)
├── build/                   # Build artifacts
├── Jenkinsfile             # CI/CD pipeline
├── Makefile               # N64 build system
└── ANALYSIS.md           # Technical analysis of ENCOM-DUNGEON
```

## Game Features

### Current Implementation (MVP)
- ✅ **Basic ROM Generation**: Boots on N64 emulator (Project64 tested)
- ✅ **Map Data Integration**: Converts ENCOM API maps to C structures  
- ✅ **Color System**: 5 terminal palettes selected by map seed hash
- ✅ **Debug Display**: Shows position, rotation, current hex, map seed
- ✅ **Controller Input**: Analog stick movement, C-button camera
- ✅ **Proper ROM Header**: Uses n64tool for valid N64 ROM structure
- 🔄 **Simple Rendering**: Basic hex outline display (working)

### Planned Features (Future Phases)
- 🔄 **3D Hex Rendering**: Floor, ceiling, and wall geometry
- 🔄 **Collision Detection**: Wall boundaries and movement constraints
- 🔄 **Performance Optimization**: Display lists and culling
- 🔄 **Enhanced Controls**: Smooth movement and camera rotation

## Technical Details

### Color System
Based on ENCOM-DUNGEON analysis:
- **Hash Algorithm**: `((hash << 5) - hash) + char` per character
- **Palette Selection**: `(hash % 5) * 3` for color index
- **RGB565 Format**: Optimized for N64 hardware
- **5 Palettes**: Green, Purple, Teal, Red, Amber

### Coordinate System
- **Hex Size**: 25 units (from ENCOM-DUNGEON)
- **Fixed-Point**: 16.16 format for precise positioning
- **Flat-top Hexagons**: Standard orientation with 6 directions
- **World Coordinates**: Direct conversion from q,r to x,z

### Memory Layout
```c
// Per-hex storage: 6 bytes
typedef struct {
    int8_t q, r;              // Hex coordinates
    int32_t x_fixed, z_fixed; // 16.16 fixed-point position  
    uint8_t type;             // ROOM/CORRIDOR
    uint8_t height;           // Height level
    uint8_t connections;      // 6-bit direction mask
    uint8_t is_walkable;      // Boolean flag
} hex_t;
```

### Performance Targets
- **Triangle Budget**: ~250 triangles/frame (10 hexes × 25 triangles)
- **Memory Usage**: <1MB ROM, ~100KB RAM for game state
- **Frame Rate**: Stable 30 FPS with room for expansion

## Testing

### Jenkins Validation
- ROM size limits (64MB max, >1KB min)
- JSON parsing and conversion verification
- Build artifact integrity

### Manual Testing
```bash
make test                    # Basic ROM validation
# TODO: Emulator integration tests
# TODO: Performance benchmarking
```

## Emulator Support

### Project64 (Recommended)
- **Version**: 3.x or newer
- **Plugin**: Default settings
- **ROM Format**: Z64 (byte-swapped)

### Other Emulators
- **Mupen64Plus**: Should work with default configuration
- **Real Hardware**: Untested (requires flash cart)

## Build Artifacts

ROMs are named with build metadata:
- **Format**: `encom-64-build-{BUILD_NUMBER}-{TIMESTAMP}.z64`
- **Metadata**: Includes git commit, map seed, ROM size
- **Storage**: Jenkins artifacts (retention policy TBD)

## Development Priorities

### Phase 1: MVP (Completed)
- [x] Build system and toolchain
- [x] Basic ROM generation with proper N64 headers  
- [x] Map data integration from ENCOM API
- [x] Debug display and controller input
- [x] Project64 emulator compatibility

### Phase 2: Core Gameplay
- [ ] 3D hex rendering (simplified geometry)
- [ ] Wall placement and collision
- [ ] Smooth movement and camera
- [ ] Performance optimization

### Phase 3: Polish
- [ ] Enhanced visuals
- [ ] Larger map support
- [ ] Audio integration (future)
- [ ] Real hardware testing

## Contributing

1. **Development**: All changes go through Jenkins pipeline
2. **Testing**: Ensure ROM builds and passes size validation
3. **Map Changes**: Test with different seed values for color variations
4. **Performance**: Monitor triangle counts and frame rate stability

---

**Part of the ENCOM Project**: This N64 port provides retro gaming access to procedurally generated hexagonal dungeons from the ENCOM ecosystem.