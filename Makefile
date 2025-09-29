all: encom-64.z64
.PHONY: all map

BUILD_DIR = build
N64_INST ?= /opt/libdragon
include $(N64_INST)/include/n64.mk

OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/hexagon.o $(BUILD_DIR)/render.o

# Map generation targets
map: src/generated/map_data.h

$(BUILD_DIR)/map_response.json: | $(BUILD_DIR)
	curl -X POST "https://encom-api-dev.riperoni.com/api/v1/map/generate" \
		-H "Content-Type: application/json" \
		-d '{"hexagonCount": 25}' \
		-o $(BUILD_DIR)/map_response.json

src/generated/map_data.h: $(BUILD_DIR)/map_response.json scripts/map_converter.py
	python3 scripts/map_converter.py $(BUILD_DIR)/map_response.json src/generated/map_data.h

encom-64.z64: N64_ROM_TITLE = "ENCOM-64"
encom-64.z64: $(BUILD_DIR)/encom-64.dfs

$(BUILD_DIR)/encom-64.dfs: $(wildcard filesystem/*)
$(BUILD_DIR)/encom-64.elf: $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: src/core/main.c src/generated/map_data.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/hexagon.o: src/core/hexagon.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/render.o: src/core/render.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) *.z64 *.elf *.sym *.stripped src/generated/map_data.h
.PHONY: clean map

-include $(wildcard $(BUILD_DIR)/*.d)