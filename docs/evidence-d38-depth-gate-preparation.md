# D38 conservative tiled-light depth-gate control: preparation

## Question

> Are the large square blocks caused by the common packed tile-depth rejection
> that runs after the producer has already established a valid geometric light
> interval?

D38 is the direct follow-up to the valid D37 negative. It is not another
light-ID bisection or a broad disable-lighting test.

## Exact change

Both `ComputeLightsCount` and `ComputeLightsIndices` merge a near/far interval
after their type-specific cone or sphere intersection. Invalid geometry
produces `[0, 0]`; valid local geometry produces `near < far`; the non-local
path supplies `[0, 65000]`.

The original shader then derives a 31-bit logarithmic depth mask and compares
the interval with the tile's packed scene-depth data before accepting the
light. D38 changes only the final branch condition:

```text
original_membership -> merged_near < merged_far
```

This preserves screen-area light-volume culling and rejects invalid geometry,
while conservatively bypassing only the later depth optimization. The same
change is applied to count and index production, so allocations and writes
remain consistent.

## Static validation

The output directory contains only the two intended producer modules:

| Hash | Bytes | SHA-256 |
| --- | ---: | --- |
| `651194bd0a21772e` | 37,768 | `e0bb862d470ae097592e6c98aaf629e001ef5a3fee9cecf43824f35e4b4be689` |
| `11e32439a86036ba` | 38,400 | `85bdaa57523955c02898397b9f6f899546ab91bc5b25119f65357a1d7b8181d0` |

The raw disassembly diff for each module contains only:

- the SPIR-V bound increasing by one;
- one ordered `merged_near < merged_far` instruction;
- the final conditional branch changing from the original depth-gated
  membership to that new geometric predicate.

Both modules validate for Vulkan 1.3. A negative contract test with the wrong
membership ID is rejected before output.

## Expected discriminator

- Squares gone with real lights and shadows intact: the final packed depth
  gate is causal. Keep narrowing its mask generation/conversion semantics or
  evaluate the conservative gate as a narrow compatibility allowance.
- Squares remain: the false boundary is in the earlier X/Y light-volume
  intersection. Do not repeat depth or ID filters.

The Windows-native sandy film grain is outside the visual criterion.


## Runtime control

Keep `IL2-Korea-D25-LightAtomicCompat-84c87c83` selected, remove D37's
`VKD3D_SHADER_QUIRKS`, and use:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D38-depth-gate-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d38-depth-gate-overrides VKD3D_SHADER_DUMP_PATH=/tmp/il2-D38-depth-gate-r1/shaders %command%
```
