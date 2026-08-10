# D26 tiled-light consumer causal gate: preparation

## Question

D25 proves that correcting the invalid light-list allocation atomic does not
remove the visible squares. D26 asks the next direct question: are those pixels
created by the two known pixel shaders' tiled dynamic-light contribution, or
do they survive in a later reflection/write/blend/composition path?

This is a temporary VKD3D-Proton developer diagnostic. It does not modify any
IL-2 file and is not a proposed user-facing workaround.

## Exact change

The target shaders are:

| Hash | Entry point | Original operation | D26 operation |
|---|---|---|---|
| `df0bd777fd1bb89d` | `PixOutLight_msp` | Fetch packed start/count from the `t9` 3D uint grid | Return `uint4(0)` for that one fetch |
| `a2d104d5c813322e` | `PixOutLight_mss` | Fetch packed start/count from the `t9` 3D uint grid | Return `uint4(0)` for that one fetch |

The low ten bits are the light count. A zero value therefore takes each
shader's existing no-tiled-lights branch, initializes the accumulated dynamic
light contribution to zero, skips its `t10` light-index loop, and continues the
rest of the shader. It does not discard the pixel shader, remove the aircraft,
or suppress its three render-target writes.

The generator in `probes/d26-tiled-light-override/` verifies the complete exact
instruction signature and requires one match. It replaces seven SPIR-V words
with another seven-word instruction, leaving the header, IDs, and all other
binary words untouched. Each output differs from its input in only five bytes.

Validation and hashes:

| Shader | Original SHA-256 | D26 SHA-256 | `spirv-val` |
|---|---|---|---|
| `df0bd777fd1bb89d` | `c474df14547fba6a65ed1bce3616f0fcb131b5afe8710c527c4afaf626275bfb` | `0f50b3c8b62bc793c8818d022aafb2dea31b0e3344e6de6581838606089042bc` | pass, Vulkan 1.3 |
| `a2d104d5c813322e` | `c84a5fb7202f47216f975eb5dbc3f45d1209a7ef759e645c1ec1522a48436701` | `c06520d01db761fa3977ae566cdf6e7a3dd41052ab3a81af6d1973ed8a322060` | pass, Vulkan 1.3 |

The ignored override directory is:

`/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d26-zero-tiled-light-overrides`

No captured application shader is tracked or eligible for upload.

## Runtime configuration

Keep the already selected local tool
`IL2-Korea-D25-LightAtomicCompat-84c87c83`. Its atomic compatibility change is
known to be visually neutral from D25, and it retains the validated Wine NUMA
startup implementation. Add only the developer override directory:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D26-zero-tiled-light-r1 VKD3D_SHADER_OVERRIDE=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/build/d26-zero-tiled-light-overrides VKD3D_SHADER_DUMP_PATH=/tmp/il2-D26-zero-tiled-light-r1/shaders %command%
```

The run is valid only if the Proton log contains one
`Overriding shader hash ...` line for each target hash and contains no device
loss or shader-module creation failure.

## Visual classification

Use the same main-menu aircraft and lighting angle. Classify only after the log
validates activation:

- **squares gone while the aircraft/menu remain:** the visual cause is in the
  tiled dynamic-light consumer chain, downstream of the D25 counter;
- **squares remain:** the two shaders' tiled-light contribution is not required,
  so move to their reflection-target write/blend or a later composition pass;
- **scene too broken or shaders not overridden:** inconclusive; refine or rerun
  rather than call it fixed.
