CC ?= cc
SPIRV_VAL ?= spirv-val

BUILD_DIR := build
PROGRAM := $(BUILD_DIR)/patch-global-record2-producer
INPUT_DIR ?= ../../captures/runs/D14-shader-dump-r1/shaders
OUTPUT_DIR ?= ../../build/d36-global-record2-producer-overrides
COUNT_INPUT := $(INPUT_DIR)/651194bd0a21772e.spv
INDICES_INPUT := $(INPUT_DIR)/11e32439a86036ba.spv
COUNT_OUTPUT := $(OUTPUT_DIR)/651194bd0a21772e.spv
INDICES_OUTPUT := $(OUTPUT_DIR)/11e32439a86036ba.spv
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror

.PHONY: all check clean

all: $(COUNT_OUTPUT) $(INDICES_OUTPUT) check

$(BUILD_DIR) $(OUTPUT_DIR):
	mkdir -p $@

$(PROGRAM): patch-global-record2-producer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Preserve each shader's original culling result, but make light ID 2 a member
# of every valid screen tile. Both passes must use the identical predicate.
$(COUNT_OUTPUT): $(PROGRAM) $(COUNT_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(COUNT_INPUT) $@ 6 78 57 64 37 316 1376 1375 1376 1

$(INDICES_OUTPUT): $(PROGRAM) $(INDICES_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(INDICES_INPUT) $@ 6 89 68 75 48 326 1403 1398 1403 1

check: $(COUNT_OUTPUT) $(INDICES_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(COUNT_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(INDICES_OUTPUT)

clean:
	$(RM) $(PROGRAM) $(COUNT_OUTPUT) $(INDICES_OUTPUT)
	-rmdir $(BUILD_DIR) $(OUTPUT_DIR)
