################################################################################
# ENCOM-64 N64 ROM Makefile
# Nintendo 64 port of ENCOM Dungeon Explorer
################################################################################

# Project settings
PROJECT_NAME = encom-64
ROM_NAME = $(PROJECT_NAME).z64
ELF_NAME = $(PROJECT_NAME).elf

# libdragon toolchain paths
N64_INST ?= $(PWD)/tools/libdragon
LIBDRAGON_SRC = $(PWD)/tools/libdragon-src
TOOLCHAIN_DIR = $(N64_INST)
N64_CC = $(TOOLCHAIN_DIR)/bin/mips64-elf-gcc
N64_LD = $(TOOLCHAIN_DIR)/bin/mips64-elf-ld
N64_OBJCOPY = $(TOOLCHAIN_DIR)/bin/mips64-elf-objcopy
# CHKSUM64 tool will be handled differently

# Source directories
SRC_DIR = src
CORE_DIR = $(SRC_DIR)/core
GEN_DIR = $(SRC_DIR)/generated
BUILD_DIR = build

# Source files
C_SOURCES = $(wildcard $(CORE_DIR)/*.c)
C_OBJECTS = $(C_SOURCES:$(CORE_DIR)/%.c=$(BUILD_DIR)/%.o)

# Include paths
INCLUDES = -I$(SRC_DIR) \
           -I$(GEN_DIR) \
           -I$(LIBDRAGON_SRC)/include \
           -I$(TOOLCHAIN_DIR)/mips64-elf/include

# Compiler flags for N64
CFLAGS = -march=vr4300 \
         -mtune=vr4300 \
         -mabi=o64 \
         -flto \
         -ffat-lto-objects \
         -ffunction-sections \
         -fdata-sections \
         -G0 \
         -Wall \
         -Wextra \
         -O2 \
         -std=gnu99 \
         $(INCLUDES)

# Linker flags
LDFLAGS = -T$(LIBDRAGON_SRC)/n64.ld \
          -L$(LIBDRAGON_SRC) \
          -L$(TOOLCHAIN_DIR)/mips64-elf/lib \
          -ldragon \
          -lc \
          -lm \
          -ldragonsys \
          -mabi=o64 \
          -Wl,--gc-sections

# Build targets
.PHONY: all clean test help

all: $(ROM_NAME)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Check for generated map data
$(GEN_DIR)/map_data.h:
	@echo "ERROR: Map data not generated. Run 'python3 scripts/map_converter.py <json> $(GEN_DIR)/map_data.h'"
	@exit 1

# Compile C source files
$(BUILD_DIR)/%.o: $(CORE_DIR)/%.c $(GEN_DIR)/map_data.h | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(N64_CC) $(CFLAGS) -c $< -o $@

# Link ELF binary
$(ELF_NAME): $(C_OBJECTS)
	@echo "Linking ELF binary..."
	$(N64_CC) $(CFLAGS) $(C_OBJECTS) -o $@ $(LDFLAGS)

# Create N64 ROM from ELF
$(ROM_NAME): $(ELF_NAME)
	@echo "Creating N64 ROM..."
	$(LIBDRAGON_SRC)/tools/n64tool -o $@ -t "ENCOM-64" $<
	@echo "ROM created: $@ ($(shell stat -c%s $@) bytes)"

# Test build (just verify files exist for now)
test: $(ROM_NAME)
	@echo "Running tests..."
	@test -f $(ROM_NAME) || (echo "ERROR: ROM not found" && exit 1)
	@test -s $(ROM_NAME) || (echo "ERROR: ROM is empty" && exit 1)
	@echo "Basic tests passed"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ELF_NAME) $(ROM_NAME)

# Clean everything including generated files
distclean: clean
	rm -f $(GEN_DIR)/*.h

# Show help
help:
	@echo "ENCOM-64 Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build ROM (default)"
	@echo "  clean   - Clean build artifacts"  
	@echo "  test    - Run basic tests"
	@echo "  help    - Show this help"
	@echo ""
	@echo "Prerequisites:"
	@echo "  1. Generate map data: python3 scripts/map_converter.py <json> $(GEN_DIR)/map_data.h"
	@echo "  2. Ensure libdragon toolchain is in $(TOOLCHAIN_DIR)"

# Debug info
debug:
	@echo "Build Configuration:"
	@echo "  Project: $(PROJECT_NAME)"
	@echo "  ROM: $(ROM_NAME)"
	@echo "  Toolchain: $(TOOLCHAIN_DIR)"
	@echo "  Sources: $(C_SOURCES)"
	@echo "  Objects: $(C_OBJECTS)"