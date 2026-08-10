# D28 ID-4 light-record filter

This local diagnostic preserves the real `t9` tile start/count, loop bounds,
`t10` fetch, and captured light IDs 1, 2, and 3 in both correlated consumer
pixel shaders. It applies `light_id & 3` before the existing sentinel test and
light-record lookup. For the observed ID range 0–4, this changes only ID 4 to
sentinel 0; the shader's existing branch skips that record.

The patcher inserts one `OpBitwiseAnd`, rewires exactly one zero comparison and
one light-record index calculation, updates the SPIR-V ID bound, and rejects an
unexpected input signature. `make` validates both outputs for Vulkan 1.3.

Interpretation:

- artifact gone: light record/type 4 is required;
- artifact remains: continue separating IDs 2 and 3 or their interaction;
- materially broken lighting: do not classify the visual result.

This is a diagnostic shader override, not a game modification or a proposed
Proton/VKD3D-Proton fix.
