# D34 record-2 shadow-visibility discriminator: preparation

## Question

D32 proves record 2 sufficient in a controlled record-1/record-2 mixture. D33
proves its live descriptor, bounds, and captured data are valid. Static SPIR-V
inspection shows that record 2, unlike safe record 1, enters a shadow-enabled
spotlight path and comparison-samples `t8`.

D34 asks whether the comparison result is required for the visible squares.

## Construction

D34 uses the validated D32 modules as inputs, retaining the exact selection:

```c
filtered_id = light_id == 2 ? 2 : 1;
```

It changes one operand in each target module. The final multiplication that
applies computed shadow visibility to record 2's existing light factor uses
constant `1.0` instead of the sampled/filter result. All ordinary record-2
light math and final accumulation remain. Safe record 1 already bypasses this
shadow branch, so its behavior is unchanged.

This is intentionally a causal diagnostic, not a proposed fix. Comparison
sampling is valid D3D12 behavior, and a final compatibility change would need
to reproduce Windows behavior rather than simply remove shadows.

## Interpretation gate

- Squares disappear while record-2 lighting remains: the shadow
  projection/comparison path is causal. Inspect sampler `s5`, exact DXIL versus
  SPIR-V semantics, coordinate/reference values, and Vulkan image state next.
- Squares remain: the ordinary record-2 spotlight contribution is causal; do
  not pursue the shadow sampler as the fix.
- Record-2 lighting disappears or rendering otherwise breaks: reject the run
  as a valid discriminator.


## Build and audit

The local generator is in
`probes/d34-shadow-visibility-override/patch-shadow-visibility.c`. It refuses
inputs that do not contain the exact D32 ID-2 selection, the expected
visibility multiply, the existing float-one constant, and exactly seven depth
comparison samples per target module.

Both outputs have the same byte size and SPIR-V ID bound as their D32 inputs.
Raw-ID disassembly diff shows one changed operand per shader and no other
change:

| Shader | Operand change | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| `df0bd777fd1bb89d` | visibility `%2302` to float-one `%323` | 76,668 | `7ca8ee0113749d2bd825f3df4c6f3bd9679ccefc33c01efbf108d1c81ab24e1f` |
| `a2d104d5c813322e` | visibility `%2300` to float-one `%326` | 76,672 | `8f95c043da06192223f2d8ff178f818bcd59c37602874eb885b68b679bd42c50` |

Both outputs pass `spirv-val --target-env vulkan1.3`. The override directory
is:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d34-shadow-visibility-overrides`

## Exact runtime comparison

Use the same `IL2-Korea-D25-LightAtomicCompat-84c87c83` compatibility-tool
baseline as D32, then set only:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D34-shadow-visibility-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d34-shadow-visibility-overrides %command%
```

Reproduce the same hangar view and report both whether the squares remain and
whether record-2 lighting is still visible. Close the game before the log is
accepted.

The run is complete. See
[`evidence-d34-shadow-visibility-result.md`](evidence-d34-shadow-visibility-result.md).
