# D02 ordinary-texture trace

- Run: `D02-r1`
- Game build: `24596901`
- Compatibility tool: `IL2-Korea-D02-Texture-Trace-54797ad3`
- VKD3D source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Diagnostic commit: `54797ad35d0dcd921f2e65a98121f2c6d98754a4`
- Visual result: **unchanged**; rectangular terrain pages remain missing at
  1,385 m
- Instrumentation validity: **valid**

## Visual evidence

The cockpit capture shows ordinary terrain geometry and vegetation around the
aircraft, but numerous rectangular ground-texture pages remain absent or show
isolated low-fidelity content. Magenta page edges are also visible. This is a
useful correction to the earlier altitude observation: altitude makes the
failure more severe, but lowering the aircraft to approximately 1,500 m does
not fix it.

Curated capture:
[`D02-r1-cockpit-missing-terrain-pages-1385m.png`](../captures/curated/d02-ordinary-texture-trace/D02-r1-cockpit-missing-terrain-pages-1385m.png)

```text
986f717b638a476cc260aeb4a14cd3762f8e2b12df50a7f5bc834ddb1d83fdc7
```

The screenshot timestamp corresponds to approximately 35,526.950 seconds
after boot. The copy-event limit was reached at trace timestamp 35,532.815,
about 5.9 seconds later. The bounded trace therefore still covered the visible
failure at the screenshot moment.

## Validity gates

- `IL2TEX enabled` occurs exactly once.
- The selected custom Proton root is recorded in the run.
- All four post-run prefix D3D12/D3D12Core hashes match the D02 custom tool.
- D3D12/D3D12Core and DXVK DXGI load; no D3D11 module loads.
- The trace is gated and diagnostic-only; it changes no rendering behavior.

The exact compressed Proton log has SHA-256:

```text
b60f9b3316d4e175b70e5f26b7c663cf5843f7b688fa60e7e2e00750ce2e2aa4
```

The retained raw source log is 26,447,236 bytes and has SHA-256:

```text
7f35934532ca3ab09e4d9efce41a2e1c5b174dc3cbfd9b06efb1faae0cee869b
```

## Event census

| Event | Count |
|---|---:|
| Texture creations | 3,478 |
| Normalized SRV descriptions | 4,185 |
| `CopyTextureRegion` | 39,978 |
| Texture `CopyResource` | 22 |
| Texture destructions by process exit | 3,478 |
| Copy-cap suppression marker | 1 |
| Split `END_ONLY` barrier warnings | 16,180 |

The 40,000-copy event cap was reached only after the screenshot. Creation,
SRV, and destruction telemetry use separate limits and remained available.

## Positive and negative evidence

The trace identifies an active ordinary texture-streaming path dominated by
placed, block-compressed textures. Correctly treating
`D3D12_RESOURCE_DIMENSION_TEXTURE3D` as one subresource per mip and combining
its separate Z-slice copies gives this result for block-compressed resources
with multiple mips:

| Upload classification | Resources |
|---|---:|
| Geometrically complete buffer-to-image mip chain | 2,355 |
| No logged buffer upload before the copy cap | 405 |
| Created after the copy cap; upload unobservable | 28 |
| Partial mip/subresource coverage | 0 |

This is evidence against a broad failure to upload the expected mips. It does
not prove the bytes in each complete upload are correct or visible when
sampled.

All 4,185 logged SRV descriptions have `ResourceMinLODClamp = 0`. The common
views expose the full mip chain beginning at mip zero. A non-zero SRV minimum
LOD clamp is therefore excluded for this run.

No SRV or copy operation occurs after the corresponding resource's destruction
marker. The runtime log also contains no device-loss, GPU-hang, out-of-memory,
backend upload-buffer-exhaustion, invalid-copy, image-allocation, or
image-binding failure signature. These results make simple use-after-free,
global allocation failure, and obviously incomplete mip upload poor primary
explanations.

## Suspicious resource class

All 405 pre-cap block-compressed resources without a logged buffer upload are placed
BC3 textures, have an SRV, and have no logged incoming texture copy. Their most
common shapes are:

| Resources | Shape |
|---|---|
| 166 | 256x256, 9 mips, BC3 |
| 102 | 512x512, 10 mips, BC3 |
| 102 | 128x128, 8 mips, BC3 |
| 34 | 1024x1024, 11 mips, BC3 |
| 1 | Texture2D array, 2048x2048x16 layers, 12 mips, BC3 |

This is a discriminator, not yet a defect. Creating an SRV does not prove a
shader sampled it. The textures could be intentionally unused, populated
through an aliasing path not covered by D02, or associated with descriptors
that are later replaced. D02 did not record heap identity, placed-buffer
ranges, aliasing barriers, descriptor copies, or draw-time descriptor use.

## Consequences

### Post-run compressed-copy analysis

The original D02 report focused on mip-chain completeness. A later alignment
pass over the same retained trace found a more concrete class: all 24,444
recognized block-compressed buffer-to-image regions were checked against the
destination mip size and 4x4 block granularity. Exactly 432 are invalid internal
regions, all targeting the 2048x2048, one-mip BC3 cache class. Their shapes are
`128x1` (118), `1x128` (112), `64x1` (110), and `1x64` (92).

These copies coincide with the game module's `BakedTerrain` and
`stitchBorders` architecture and current VKD3D forwards the extents unchanged
to `vkCmdCopyBufferToImage2`. This supersedes the no-upload SRV class as the
first behavioral discriminator. See
[`evidence-d02-bc3-border-copies.md`](evidence-d02-bc3-border-copies.md).

- D3D12 sparse/reserved resources remain excluded by D01b.
- A general missing-mip or non-zero SRV-minimum-LOD explanation is not
  supported by D02.
- `NO_STAGGERED_SUBMIT` is not justified: the ordinary failing D02 path does
  not report staggered submission activation, and the E02 `single_queue`
  control enabled that path while leaving the corruption unchanged.
- `AVOID_IMAGE_BUFFER_ALIASING` cannot yet be selected from the visual symptom.
  In this VKD3D-Proton version that flag changes descriptor-heap image/buffer
  placement; it does not directly prove D3D placed-resource memory overlap.
- If the gated BC3 border-normalization test fixes only the seams, the next
  diagnostic must correlate the 2048x2048 baked-cache pool with descriptor
  propagation, copy-to-sample visibility, and actual draw use.

No permanent application override, general VKD3D-Proton patch, or RADV patch is
justified yet. One gated behavior-changing diagnostic is now justified by the
same D02 trace.
