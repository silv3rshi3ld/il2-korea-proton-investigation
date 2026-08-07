# D06 focused baked-cache trace result

## Result

D06 is a valid trace-only run. The user observed the same corruption, which is
expected because this build changes no D3D12 or Vulkan command. The trace
records one enable marker, 183 matching resource creations, 1,000 bounded copy
events, 20,000 bounded barrier events, and orderly destruction of the observed
resources.

The important result is the copy geometry of the six active 2048x2048 one-mip
BC3 caches:

| Emitted extent | Copies |
|---|---:|
| `64x64x1` | 292 |
| `64x1x1` | 354 |
| `1x64x1` | 354 |

The `64x64` interiors are placed at destination offsets separated by 256
texels: `0`, `256`, `512`, and so on. Border locations such as `252`, `508`,
and `764` align with the ends of those 256-texel pages. VKD3D-Proton instead
emits the source dimensions unchanged, leaving each interior at only `64x64`.

Keeping the observed offsets but expanding each source element to one 4x4 BC3
block changes exact union coverage as follows:

| Cache pair | Current coverage | Projected 4x coverage |
|---|---:|---:|
| `2886`, `2888` | 6.32% each | 100.00% each |
| `2892`, `2894` | 4.64% each | 73.44% each |
| `2813`, `2815` | 3.46% each | 54.69% each |

This geometry closely matches the visible isolated terrain rectangles and
empty space. D05c changed only the thin borders, so its unchanged result
excluded a border-only repair but did not test the interior pages.

## Limits

D06 logged the source resource as a buffer and therefore printed source format
zero; it did not print `PlacedFootprint.Footprint.Format`. D05b/D05c proved
that the thin members of this copy family use footprint-only
`R32G32B32A32_UINT` sources. The same format for the square interiors is
strongly indicated by the page layout but is not established by D06 alone.

The barrier cap is not useful for terrain ordering in this run. One format
`0x37` resource consumed 19,995 of 20,000 barrier records before most terrain
copies occurred. No absence-of-barrier conclusion is drawn.

## Next discriminator

D07 keeps the D05c physical-block checks and exact cache filter, but extends
the opt-in conversion to observed `64x64` and `128x128` interiors. Its own
candidate log records the placed-footprint format before adjustment. One run
therefore confirms the missing source-format field and directly tests visual
causality without a separate trace-only run.

Detailed machine-generated output is in
`captures/runs/D06-cache-trace-r1/baked-cache-analysis.md` in the local
workspace; the large raw Proton log remains excluded from Git.
