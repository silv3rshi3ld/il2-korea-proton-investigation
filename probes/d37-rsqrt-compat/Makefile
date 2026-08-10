ROOT := ../..
DXIL_SPIRV := $(ROOT)/build/dxil-spirv-tools-395d9747/dxil-spirv
INPUT := $(ROOT)/captures/runs/D14-shader-dump-r1/shaders
OUTPUT := $(ROOT)/build/d37-rsqrt-compat-static

HASHES := 651194bd0a21772e 11e32439a86036ba
SPV := $(addprefix $(OUTPUT)/,$(addsuffix .spv,$(HASHES)))
ASM := $(SPV:.spv=.spvasm)

.PHONY: all clean verify
.SECONDARY: $(SPV)

all: $(SPV) $(ASM) verify

$(OUTPUT):
	mkdir -p $@

$(OUTPUT)/%.spv: $(INPUT)/%.dxil | $(OUTPUT)
	$(DXIL_SPIRV) $< --output $@ --validate --shader-quirk 10

$(OUTPUT)/%.spvasm: $(OUTPUT)/%.spv
	spirv-dis --raw-id $< -o $@

verify: $(SPV) $(ASM)
	@for file in $(ASM); do \
		test "$$(grep -c 'InverseSqrt' $$file)" -eq 12; \
		test "$$(grep -c ' NMin ' $$file)" -eq 27; \
		spirv-val --target-env vulkan1.3 $${file%.spvasm}.spv; \
	done
	@echo D37_RSQRT_STATIC_CONTRACT_OK

clean:
	rm -f $(SPV) $(ASM)
