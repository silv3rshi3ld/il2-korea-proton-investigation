CC ?= cc
SPIRV_VAL ?= spirv-val

BUILD_DIR := build
PROGRAM := $(BUILD_DIR)/patch-isolate-light-id
DUMP_DIR ?= /tmp/il2-D25-light-atomic-compat-r1/shaders
OUTPUT_DIR ?= ../../build/d32-isolate-light-id2-overrides
MSP_INPUT := $(DUMP_DIR)/df0bd777fd1bb89d.spv
MSS_INPUT := $(DUMP_DIR)/a2d104d5c813322e.spv
MSP_OUTPUT := $(OUTPUT_DIR)/df0bd777fd1bb89d.spv
MSS_OUTPUT := $(OUTPUT_DIR)/a2d104d5c813322e.spv
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror

.PHONY: all check clean

all: $(MSP_OUTPUT) $(MSS_OUTPUT) check

$(BUILD_DIR) $(OUTPUT_DIR):
	mkdir -p $@

$(PROGRAM): patch-isolate-light-id.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

# Preserve only genuine ID 2; map every other entry to safe ID 1.
$(MSP_OUTPUT): $(PROGRAM) $(MSP_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(MSP_INPUT) $@ 6 1347 1346 82 209 215 202 327

$(MSS_OUTPUT): $(PROGRAM) $(MSS_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(MSS_INPUT) $@ 6 1345 1344 84 211 217 204 330

check: $(MSP_OUTPUT) $(MSS_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(MSP_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(MSS_OUTPUT)

clean:
	$(RM) $(PROGRAM) $(MSP_OUTPUT) $(MSS_OUTPUT)
	-rmdir $(BUILD_DIR) $(OUTPUT_DIR)
