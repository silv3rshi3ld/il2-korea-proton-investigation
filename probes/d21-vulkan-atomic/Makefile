CC ?= cc
GLSLANG ?= glslangValidator
SPIRV_VAL ?= spirv-val
PKG_CONFIG ?= pkg-config

BUILD_DIR := build
PROGRAM := $(BUILD_DIR)/d21-vulkan-atomic
TEXEL_SPV := $(BUILD_DIR)/atomic_texel.comp.spv
SSBO_SPV := $(BUILD_DIR)/atomic_ssbo.comp.spv
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags vulkan)
LDLIBS += $(shell $(PKG_CONFIG) --libs vulkan)
DEVICE ?= 0
VALIDATION ?= --validation
EXACT_SPV ?= ../../captures/runs/D14-shader-dump-r1/shaders/7cefa1bc80bb4c70.spv
EXACT_R16_SSBO_SPV ?= ../../build/renderdoc-1.45-local/7cefa1bc80bb4c70-counter-ssbo.spv

.PHONY: all check-shaders list run run-exact run-all run-offset-zero-ab clean

all: $(PROGRAM) check-shaders

$(BUILD_DIR):
	mkdir -p $@

$(TEXEL_SPV): atomic_texel.comp | $(BUILD_DIR)
	$(GLSLANG) -V --target-env vulkan1.1 -o $@ $<

$(SSBO_SPV): atomic_ssbo.comp | $(BUILD_DIR)
	$(GLSLANG) -V --target-env vulkan1.1 -o $@ $<

$(PROGRAM): d21-vulkan-atomic.c $(TEXEL_SPV) $(SSBO_SPV) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LDLIBS)

check-shaders: $(TEXEL_SPV) $(SSBO_SPV)
	$(SPIRV_VAL) --target-env vulkan1.1 $(TEXEL_SPV)
	$(SPIRV_VAL) --target-env vulkan1.1 $(SSBO_SPV)

list: $(PROGRAM)
	$(PROGRAM) --list

run: all
	$(PROGRAM) $(VALIDATION) --device $(DEVICE) --texel $(TEXEL_SPV) --ssbo $(SSBO_SPV)

run-exact: all
	test -f $(EXACT_SPV)
	$(PROGRAM) $(VALIDATION) --device $(DEVICE) --texel $(TEXEL_SPV) --ssbo $(SSBO_SPV) --exact $(EXACT_SPV)

run-all: all
	test -f $(EXACT_SPV)
	test -f $(EXACT_R16_SSBO_SPV)
	$(PROGRAM) $(VALIDATION) --device $(DEVICE) --texel $(TEXEL_SPV) --ssbo $(SSBO_SPV) \
		--exact $(EXACT_SPV) --exact-r16 $(EXACT_SPV) \
		--exact-r16-ssbo $(EXACT_R16_SSBO_SPV)

run-offset-zero-ab: all
	test -f $(EXACT_SPV)
	test -f $(EXACT_R16_SSBO_SPV)
	$(PROGRAM) $(VALIDATION) --format-ab --device $(DEVICE) \
		--texel $(TEXEL_SPV) --ssbo $(SSBO_SPV) \
		--exact $(EXACT_SPV) --exact-r16 $(EXACT_SPV) \
		--exact-r16-ssbo $(EXACT_R16_SSBO_SPV)

clean:
	$(RM) $(PROGRAM) $(TEXEL_SPV) $(SSBO_SPV)
	-rmdir $(BUILD_DIR)
