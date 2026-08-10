CC ?= cc
SPIRV_VAL ?= spirv-val

BUILD_DIR := build
PROGRAM := $(BUILD_DIR)/patch-global-record2
INPUT_DIR ?= ../../build/d34-shadow-visibility-overrides
OUTPUT_DIR ?= ../../build/d35-global-record2-overrides
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

$(PROGRAM): patch-global-record2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(MSP_OUTPUT): $(PROGRAM) $(MSP_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(MSP_INPUT) $@ 34 1113 156 1115 82 215 6 3347 3346 209 215 10 1676 323 1677 1

$(MSS_OUTPUT): $(PROGRAM) $(MSS_INPUT) | $(OUTPUT_DIR)
	$(PROGRAM) $(MSS_INPUT) $@ 34 1111 158 1113 84 217 6 3345 3344 211 217 10 1674 326 1675 1

check: $(MSP_OUTPUT) $(MSS_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(MSP_OUTPUT)
	$(SPIRV_VAL) --target-env vulkan1.3 $(MSS_OUTPUT)

clean:
	$(RM) $(PROGRAM) $(MSP_OUTPUT) $(MSS_OUTPUT)
	-rmdir $(BUILD_DIR) $(OUTPUT_DIR)
