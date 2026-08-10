CC ?= cc
SPIRV_VAL ?= spirv-val

BUILD_DIR := build
PROGRAM := $(BUILD_DIR)/patch-shadow-visibility
INPUT_DIR ?= ../../build/d32-isolate-light-id2-overrides
OUTPUT_DIR ?= ../../build/d34-shadow-visibility-overrides
MSP_INPUT := $(INPUT_DIR)/df0bd777fd1bb89d.spv
MSS_INPUT := $(INPUT_DIR)/a2d104d5c813322e.spv
MSP_OUTPUT := $(OUTPUT_DIR)/df0bd777fd1bb89d.spv
MSS_OUTPUT := $(OUTPUT_DIR)/a2d104d5c813322e.spv
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror

.PHONY: all check clean

all: $(MSP_OUTPUT) $(MSS_OUTPUT) check

$(BUILD_DIR) $(OUTPUT_DIR):
	mkdir -p $@

$(PROGRAM): patch-shadow-visibility.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# D32 maps every non-2 ID to safe ID 1. Preserve that control and replace only
# the shadow-visibility operand in the ID-2 path with float 1.0.
$(MSP_OUTPUT): $(PROGRAM) $(MSP_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(MSP_INPUT) $@ 10 327 6 1347 209 215 3346 3347 323 1676 2302 1677

$(MSS_OUTPUT): $(PROGRAM) $(MSS_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(MSS_INPUT) $@ 10 330 6 1345 211 217 3344 3345 326 1674 2300 1675

check: $(MSP_OUTPUT) $(MSS_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(MSP_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(MSS_OUTPUT)

clean:
	$(RM) $(PROGRAM) $(MSP_OUTPUT) $(MSS_OUTPUT)
	-rmdir $(BUILD_DIR) $(OUTPUT_DIR)
