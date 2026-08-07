# D05b footprint-aware BC3 diagnostic result

## Result

Run `D05b-bc3-r1` is visually unchanged, but it produced a decisive source-
format discriminator rather than a valid normalization test.

- The intended diagnostic DLL loaded and emitted one enable marker.
- It recorded exactly 432 target-class candidates.
- It adjusted zero candidates and explicitly rejected all 432.
- 214 candidates used rejection mask `0x1`; 218 used `0x41`.
- Every candidate was footprint-only (`src_box_present=0`).
- Every source footprint used `DXGI_FORMAT_R32G32B32A32_UINT` (`0x3`).
- Every destination used `DXGI_FORMAT_BC3_UNORM` (`0x4d`).
- The visible terrain corruption remained at 1,407 m.

Because no Vulkan copy changed, the screenshot is not a negative result for
the corrected copy geometry. It confirms only that D05b's BC3-source model was
wrong.

## Reinterpret-copy deduction

D3D12 explicitly permits 128-bit uncompressed resources such as
`R32G32B32A32_UINT` to be reinterpreted as BC2/BC3/BC5. Each uncompressed texel
contains one complete 16-byte compressed block, so the block-compressed width
and height are each four times the uncompressed dimensions.

The observed Korea copies therefore map as follows:

| Source footprint | D3D12 row pitch | Required BC3 Vulkan extent | Required `bufferRowLength` | Required `bufferImageHeight` |
|---|---:|---|---:|---:|
| `128x1x1` RGBA32_UINT | 2,048 | `512x4x1` | 512 | 4 |
| `64x1x1` RGBA32_UINT | 1,024 | `256x4x1` | 256 | 4 |
| `1x128x1` RGBA32_UINT | 256 | `4x512x1` | 64 | 512 |
| `1x64x1` RGBA32_UINT | 256 | `4x256x1` | 64 | 256 |

The existing buffer-to-image helper instead derives the Vulkan buffer layout
and copy extent from the uncompressed source geometry. Representative emitted
pre-D05c values are:

- `128x1`: extent `128x1`, `bufferRowLength=128`,
  `bufferImageHeight=1`;
- `1x128`: extent `1x128`, `bufferRowLength=16`,
  `bufferImageHeight=128`.

When Vulkan interprets those fields using the BC3 destination format, the byte
layout and copied block count no longer represent the D3D12 operation.

## Prior art

VKD3D-Proton commit `e28f36ae18821f58aaa94c68af9efdac590454df`
fixed the same compressed/uncompressed block conversion for image-to-image
copies by doing the geometry in block units. Its regression test commit
`2eccf76a311e83d70b461c978a33975a9fec5340` includes
`R32G32B32A32_UINT` to BC3 `CopyResource` and `CopyTextureRegion` cases.
The buffer-footprint-to-image helper was not changed by that fix.

## Evidence

- Run log SHA-256 (compressed):
  `088bfffbf263303b0c50828e7fc3f2d9a8aab2f83c03bffd7c90dd7ffdf46c01`
- Screenshot:
  `captures/curated/d05b-bc3-normalization/D05b-r1-terrain-unchanged-1407m.png`
- Screenshot SHA-256:
  `88f4f770b4f96bfd506955f24e8e3dfd0ee1050088d16c191552f163b3b70f48`
- Full run directory (intentionally ignored because it contains the large raw
  Proton log): `captures/runs/D05b-bc3-r1/`

## Next discriminator

D05c keeps the opt-in gate and exact Korea resource/shape filter but applies
the documented uncompressed-to-BC3 block mapping to the Vulkan extent,
`bufferRowLength`, and `bufferImageHeight`. It is still a causal experiment,
not an application override or proposed permanent patch.
