all: encom-64.z64
.PHONY: all

BUILD_DIR = build
N64_INST ?= /opt/libdragon
include $(N64_INST)/include/n64.mk

OBJS = $(BUILD_DIR)/main.o

encom-64.z64: N64_ROM_TITLE = "ENCOM-64"
encom-64.z64: $(BUILD_DIR)/encom-64.dfs

$(BUILD_DIR)/encom-64.dfs: $(wildcard filesystem/*)
$(BUILD_DIR)/encom-64.elf: $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main.o: src/core/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) *.z64 *.elf *.sym *.stripped
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)