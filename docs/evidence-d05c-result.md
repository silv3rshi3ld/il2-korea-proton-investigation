# D05c exact reinterpret-copy diagnostic result

## Result

D05c executed the intended behavior, but the visible corruption was unchanged.
The user observed the same rectangular missing terrain pages and magenta edges
in the mission. This is a valid negative causal result for the tested thin-
border subset, not another instrumentation failure.

- Run: `D05c-reinterpret-r1`
- Game build: `24596901`
- Diagnostic commit: `5391ec7f427795fe0fc151047422629d849e35be`
- Enable markers: 1
- Target-class candidates: 202
- Adjustments: 202
- Rejections: 0
- Distinct destination resources: 8
- GPU hang or reset reported: no
- Visual classification: **unchanged**

The 202 adjusted regions comprised:

| D3D12 source extent | Emitted BC3 destination extent | Count |
|---|---|---:|
| `1x64x1` | `4x256x1` | 78 |
| `1x128x1` | `4x512x1` | 28 |
| `64x1x1` | `256x4x1` | 70 |
| `128x1x1` | `512x4x1` | 26 |

Every source was a footprint-only `R32G32B32A32_UINT` resource. D05c mapped
each 128-bit source element to one 4x4 BC3 block and recomputed the Vulkan
extent and buffer layout. The lower count than D02/D05b reflects different
run duration and terrain demand; all candidates encountered by this run were
adjusted.

## Interpretation

The buffer-to-image reinterpret geometry differs from the analogous VKD3D
image-to-image conversion, and D05c demonstrated that the proposed conversion
can be applied to this exact Korea cache class. It did **not** fix or materially
improve the visible defect. It therefore excludes correction of the thin
borders alone. D06 later showed that the `64x64` page interiors have the same
likely unit mismatch; D05c did not modify those interiors and cannot exclude a
full-page conversion.

No permanent compatibility behavior or application override is justified from
this result alone. See `evidence-d06-result.md` for the subsequent full-page
geometry and `evidence-d07-preparation.md` for the causal follow-up.
