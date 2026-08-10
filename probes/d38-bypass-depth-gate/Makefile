ROOT := ../..
INPUT := $(ROOT)/captures/runs/D14-shader-dump-r1/shaders
OUTPUT := $(ROOT)/build/d38-depth-gate-overrides
PATCHER := build/patch-depth-gate

COUNT_HASH := 651194bd0a21772e
INDEX_HASH := 11e32439a86036ba

.PHONY: all clean test

all: test

$(PATCHER): patch-depth-gate.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c11 -Wall -Wextra -Werror -O2 $< -o $@

$(OUTPUT):
	mkdir -p $@

$(OUTPUT)/$(COUNT_HASH).spv: $(PATCHER) $(INPUT)/$(COUNT_HASH).spv | $(OUTPUT)
	$(PATCHER) $(INPUT)/$(COUNT_HASH).spv $@ 78 10 210 213 316 1376 1375 1376 1 38

$(OUTPUT)/$(INDEX_HASH).spv: $(PATCHER) $(INPUT)/$(INDEX_HASH).spv | $(OUTPUT)
	$(PATCHER) $(INPUT)/$(INDEX_HASH).spv $@ 89 10 220 223 326 1403 1398 1403 1 38

test: $(OUTPUT)/$(COUNT_HASH).spv $(OUTPUT)/$(INDEX_HASH).spv
	test "$$(find $(OUTPUT) -maxdepth 1 -type f -name '*.spv' | wc -l)" -eq 2
	spirv-val --target-env vulkan1.3 $(OUTPUT)/$(COUNT_HASH).spv
	spirv-val --target-env vulkan1.3 $(OUTPUT)/$(INDEX_HASH).spv
	spirv-dis --raw-id $(OUTPUT)/$(COUNT_HASH).spv -o $(OUTPUT)/$(COUNT_HASH).spvasm
	spirv-dis --raw-id $(OUTPUT)/$(INDEX_HASH).spv -o $(OUTPUT)/$(INDEX_HASH).spvasm
	grep -q 'OpFOrdLessThan %78 %210 %213' $(OUTPUT)/$(COUNT_HASH).spvasm
	grep -q 'OpFOrdLessThan %89 %220 %223' $(OUTPUT)/$(INDEX_HASH).spvasm
	@echo D38_DEPTH_GATE_STATIC_CONTRACT_OK

clean:
	rm -rf build
	rm -f $(OUTPUT)/$(COUNT_HASH).spv $(OUTPUT)/$(INDEX_HASH).spv
	rm -f $(OUTPUT)/$(COUNT_HASH).spvasm $(OUTPUT)/$(INDEX_HASH).spvasm
