# D03 placed-resource alias trace

- Run: `D03-r1`
- Game build: `24596901`
- Compatibility tool: `IL2-Korea-D03-Alias-Trace-cfca234e`
- VKD3D source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Diagnostic commit: `cfca234ebaff261e5fc1aa1df2a9f5520fef5e96`
- Visual result: **unchanged**, reported by the user; no D03 screenshot was saved
- Instrumentation validity: **valid**

## Question and result

D03 tested whether the placed, multi-mip block-compressed textures that receive
an SRV without a logged incoming copy are populated through D3D12 placed-resource
memory aliasing. It correlated the texture class with every traced placed buffer
and texture range and with explicit legacy D3D12 alias barriers.

The result is negative. Of 585 candidate textures created and first exposed by
an SRV before copy-event suppression:

- all 585 have a matching placed-resource record;
- zero overlap any traced placed buffer or texture range, even outside an
  overlapping lifetime;
- zero overlap a live buffer;
- zero overlap a live texture; and
- zero are named by an explicit legacy alias barrier.

The full run records zero explicit legacy alias barriers. This excludes
placed-resource range aliasing as the population path for the covered candidate
class with high confidence. It does not test descriptor-heap image/buffer type
reuse, which is a different mechanism despite the similar terminology in
`AVOID_IMAGE_BUFFER_ALIASING`.

## Validity and coverage

- `IL2TEX enabled` and `IL2ALIAS enabled` each occur exactly once.
- All four runtime D3D12/D3D12Core hashes match the D03 custom tool.
- `dxBackend12.dll`, VKD3D-Proton D3D12/D3D12Core, and DXVK DXGI load; no
  D3D11 module loads.
- The trace changes no allocation, resource, descriptor, barrier, queue, or
  synchronization behavior.
- The copy-event cap occurs at log line 153,221. The alias-create cap occurs
  later, so every pre-copy-cap candidate has a corresponding alias record.
- Another 952 broad candidates were created or first exposed by an SRV after
  copy suppression and were excluded rather than mislabeled as missing uploads.
- A later incoming copy after suppression cannot be ruled out by this bounded
  run; the range-overlap and zero-alias-barrier results do not depend on copy
  telemetry after the cap.

## Event census

| Event | Count |
|---|---:|
| Placed-resource creates | 30,000 |
| Placed-resource destroys | 30,000 |
| Placed buffers | 25,702 |
| Placed textures | 4,298 |
| Distinct traced heaps | 55 |
| Explicit legacy alias barriers | 0 |
| Texture creates | 4,371 |
| SRV descriptions | 5,107 |
| `CopyTextureRegion` | 39,988 |
| Texture `CopyResource` | 12 |
| Split `END_ONLY` warnings | 28,608 |

The raw source log is 50,783,549 bytes with SHA-256:

```text
63bd8b57abf95f4b7940f353947577816ad92be21561998840fa9213ee3623c9
```

The compressed retained log has SHA-256:

```text
702d89ebb68065f9878647d3b433fa871689234ff2a997c2826352f96004ace3
```

The unredacted generated reports remain under ignored `captures/runs/D03-r1/`.
Heap pointers in those reports are process-local diagnostic identifiers and
must be redacted before publication.

## Consequences

- No application override or aliasing workaround is justified.
- `AVOID_IMAGE_BUFFER_ALIASING` is not selected by D03 because it controls
  descriptor-heap image/buffer placement, not D3D12 placed-resource ranges.
- The next controlled discriminator is E03: disable only
  `VK_EXT_descriptor_buffer`. This directly tests the active descriptor-buffer
  backend without combining it with upload or queue changes.
- If E03 is unchanged, a descriptor-QA build is the next source-level step;
  it should test descriptor type, lifetime, propagation, and binding rather
  than repeat broader resource-allocation logging.

D03 is a negative but useful result: it removes a plausible mechanism and
prevents a speculative aliasing application override.
