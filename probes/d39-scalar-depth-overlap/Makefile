ROOT := ../..
INPUT := $(ROOT)/captures/runs/D14-shader-dump-r1/shaders
OUTPUT := $(ROOT)/build/d39-scalar-depth-overlap-overrides
PATCHER := ./patch-scalar-depth-overlap.py

COUNT_HASH := 651194bd0a21772e
INDEX_HASH := 11e32439a86036ba

.PHONY: all clean test

all: test

$(OUTPUT):
	mkdir -p $@

$(OUTPUT)/$(COUNT_HASH).spv: $(PATCHER) $(INPUT)/$(COUNT_HASH).spv | $(OUTPUT)
	python3 $(PATCHER) $(INPUT)/$(COUNT_HASH).spv $@ 78 10 210 213 114 128 316 1376 1375 1376 1 39

$(OUTPUT)/$(INDEX_HASH).spv: $(PATCHER) $(INPUT)/$(INDEX_HASH).spv | $(OUTPUT)
	python3 $(PATCHER) $(INPUT)/$(INDEX_HASH).spv $@ 89 10 220 223 124 138 326 1403 1398 1403 1 39

test: $(OUTPUT)/$(COUNT_HASH).spv $(OUTPUT)/$(INDEX_HASH).spv
	test "$$(find $(OUTPUT) -maxdepth 1 -type f -name '*.spv' | wc -l)" -eq 2
	spirv-val --target-env vulkan1.3 $(OUTPUT)/$(COUNT_HASH).spv
	spirv-val --target-env vulkan1.3 $(OUTPUT)/$(INDEX_HASH).spv
	spirv-dis --raw-id $(OUTPUT)/$(COUNT_HASH).spv -o $(OUTPUT)/$(COUNT_HASH).spvasm
	spirv-dis --raw-id $(OUTPUT)/$(INDEX_HASH).spv -o $(OUTPUT)/$(INDEX_HASH).spvasm
	grep -q '%1380 = OpFOrdGreaterThanEqual %78 %210 %114' $(OUTPUT)/$(COUNT_HASH).spvasm
	grep -q '%1387 = OpLogicalAnd %78 %1379 %1386' $(OUTPUT)/$(COUNT_HASH).spvasm
	grep -q '%1407 = OpFOrdGreaterThanEqual %89 %220 %124' $(OUTPUT)/$(INDEX_HASH).spvasm
	grep -q '%1414 = OpLogicalAnd %89 %1406 %1413' $(OUTPUT)/$(INDEX_HASH).spvasm
	@echo D39_SCALAR_DEPTH_STATIC_CONTRACT_OK

clean:
	rm -f $(OUTPUT)/$(COUNT_HASH).spv $(OUTPUT)/$(INDEX_HASH).spv
	rm -f $(OUTPUT)/$(COUNT_HASH).spvasm $(OUTPUT)/$(INDEX_HASH).spvasm
