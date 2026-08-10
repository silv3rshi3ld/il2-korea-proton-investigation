ROOT := ../..
INPUT := $(ROOT)/captures/runs/D14-shader-dump-r1/shaders
OUTPUT := $(ROOT)/build/d43-conservative-depth-overrides
PATCHER := ./patch-conservative-depth.py
HASH := e41c75bf472dc42b

.PHONY: all clean test

all: test

$(OUTPUT):
	mkdir -p $@

$(OUTPUT)/$(HASH).spv: $(PATCHER) $(INPUT)/$(HASH).spv | $(OUTPUT)
	python3 $(PATCHER) $(INPUT)/$(HASH).spv $@

test: $(OUTPUT)/$(HASH).spv
	test "$$(find $(OUTPUT) -maxdepth 1 -type f -name '*.spv' | wc -l)" -eq 1
	spirv-val --target-env vulkan1.3 $(OUTPUT)/$(HASH).spv
	spirv-dis --raw-id $(OUTPUT)/$(HASH).spv -o $(OUTPUT)/$(HASH).spvasm
	grep -q '%260 = OpINotEqual %70 %154 %34' $(OUTPUT)/$(HASH).spvasm
	grep -q '%262 = OpSelect %6 %260 %261 %160' $(OUTPUT)/$(HASH).spvasm
	grep -q '%265 = OpLogicalAnd %70 %263 %264' $(OUTPUT)/$(HASH).spvasm
	grep -q '%267 = OpSelect %6 %265 %266 %228' $(OUTPUT)/$(HASH).spvasm
	grep -q '%271 = OpBitwiseOr %6 %270 %269' $(OUTPUT)/$(HASH).spvasm
	grep -q '%229 = OpShiftLeftLogical %6 %267 %230' $(OUTPUT)/$(HASH).spvasm
	grep -q '%231 = OpBitwiseOr %6 %262 %229' $(OUTPUT)/$(HASH).spvasm
	grep -q '%235 = OpCompositeConstruct %234 %231 %271 %231 %231' $(OUTPUT)/$(HASH).spvasm
	@echo D43_CONSERVATIVE_DEPTH_STATIC_CONTRACT_OK

clean:
	rm -f $(OUTPUT)/$(HASH).spv $(OUTPUT)/$(HASH).spvasm
