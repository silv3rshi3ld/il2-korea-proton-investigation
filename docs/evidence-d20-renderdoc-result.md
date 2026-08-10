# D20 tiled-light value capture: result

## Result

D20 is a successful diagnostic capture and not a visual fix. The user saw the
same illuminated/reflected square artifacts while the D20 build was active.
CPU-only inspection of two full-frame captures then found a strong value-level
failure in the tiled-light list: all 50 `8x8` compute workgroups reuse the same
small beginning of the global light-index buffer instead of receiving disjoint
ranges from one device-wide allocation.

At the time of capture this was a strong candidate for the visible failure: a
screen tile could apparently read light IDs written for a different tile or
workgroup. D21-D24 later explained how the malformed pattern can arise from an
invalid 32-bit atomic through the game's live `R16_UINT` UAV view.

However, D25 corrected that operation in the running game and the square
artifact remained. The captured values are therefore no longer evidence of
the visual root cause. They remain evidence of a real invalid descriptor/atomic
combination whose observable rendering effect is unproven. See
[`evidence-d21-d25-atomic-result.md`](evidence-d21-d25-atomic-result.md).

## Run and capture identity

- Run: `D20-renderdoc-light-values-r1`
- Game build: `24615759`
- Compatibility tool: `IL2-Korea-D20-RenderDoc-5735f64f`
- VKD3D-Proton source: `5735f64f643236a8cd189297e56e4015bcdf3c55`
- GPU/driver: Radeon RX 7800 XT, RADV 26.1.6
- Launch line contained only the ordinary Proton log variables; no OpenMP,
  topology, rendering, or game-setting workaround was active
- User classification: artifacts/shimmering present

Three local RenderDoc files were produced. They are ignored by Git and must
not be uploaded because they may contain captured game resources:

| Capture | SHA-256 | Use |
|---|---|---|
| `il2-d20_capture.rdc` | `9ac54fb88712b9a5a8bdc862e98581c20b07a0c81ec5fb968b067358323fbab2` | targeted queue submission |
| `il2-d20_frame2794.rdc` | `8b88f6eb37d95eaaded055d6e320bcf4546735c90ccdf45a0a36134a8247082c` | complete frame |
| `il2-d20_frame3335.rdc` | `d9159a2620a24a727f01d7fe9012de7fb2c15734b265dc520ba9ba1a54276ed9` | second complete frame |

The game itself completed the capture run without a GPU hang or reset. A later
RenderDoc GPU replay caused a recoverable RADV device loss, so analysis after
that point was deliberately CPU-only. That replay failure is not classified as
an IL-2 runtime failure and is not used as evidence for the square artifact.

## Resolved resources

The captured consumer inputs agree with D16:

- `t9`: `rtLightRefs25`, `80x34x2 R32_UINT`, RenderDoc
  `ResourceId::32573`;
- `t10`: an 87,040-byte range at offset 86,507,520 in RenderDoc
  `GlobalBuffer (cookie 118)`, viewed as 43,520 `R16_UINT` values.

The targeted submission begins with zeroed initial contents and therefore does
not expose the final live producer values by static extraction alone. The two
full-frame captures contain coherent prior-frame contents for the same
resources and supply the useful discriminator.

## Value-level evidence

The lower ten bits of `t9` layer 0 are the per-tile light count. The remaining
bits are the first `t10` index allocated to that tile. `t9` layer 1 contains the
corresponding final cursor. The consumer pixel shaders unpack that same layout.

For frame 2794:

- the 2,720 tiles request 10,545 total light-index entries;
- individual tile counts are between 1 and 5;
- the largest recorded end offset is only 320;
- only the first 320 entries of the 43,520-element `t10` buffer are used;
- within every one of the 50 dispatch workgroups, tile intervals form a
  gap-free partition starting at zero.

For frame 3335:

- the 2,720 tiles request 13,349 total entries;
- individual tile counts are between 3 and 5;
- the largest recorded end offset is again only 320;
- only the first 320 `t10` entries are used;
- all 50 workgroups again form independent, gap-free partitions from zero.

A correct device-wide allocator would produce non-overlapping ranges spanning
roughly 10,545 and 13,349 entries in these frames. Instead, each workgroup acts
as though it owns a private counter beginning at zero. The repeated 320 ceiling
also matches the maximum of 64 active tiles in an `8x8` group multiplied by
the observed upper count of five.

The two frames contain different, plausible light IDs and tile counts. The
result is therefore not a completely stale or uniformly zero resource. It is
a structured allocation failure at the workgroup boundary.

## DXIL-to-SPIR-V boundary

The original `ComputeLightsFirstRef` DXIL (`7cefa1bc80bb4c70`) performs one
atomic add on UAV `u1`, element zero, for every active screen tile. Its returned
value becomes that tile's start offset. There is no workgroup-indexed counter
in the DXIL.

The preceding `SetLightIndex` DXIL (`ce5553a11c1e3c3d`) writes the same `u1`
element only from global dispatch invocation `(0,0,0)`. It does not reset the
counter once per workgroup.

VKD3D-Proton translates the allocation to an `R32ui` storage texel buffer and:

```text
OpImageTexelPointer ... element 0
OpAtomicIAdd ... Scope=Device MemorySemantics=Relaxed
```

`Scope=Device` is the required cross-workgroup scope. The structural
translation therefore preserves the important D3D operation. This does not
prove that all descriptor and synchronization state reaching the driver is
correct, but it excludes the obvious translation to a workgroup-scoped atomic.

## Evidence boundary

This was a strong candidate, but later testing rejects it as a sufficient
visual cause:

- the extracted full-frame contents are initial state for the captured frame,
  not a readback inserted at the exact consumer event;
- both independent frames nevertheless contain the same impossible allocation
  structure and different live-looking light data;
- D15 proves the final grid/index dependencies, but its bounded trace was not
  designed to identify the small allocation counter and must not be
  retroactively described as doing so;
- the RenderDoc captures should not be GPU-replayed again on this driver while
  the device-loss behavior remains unexplained;
- D21 and D22 show that legal `R32_UINT` atomics work correctly on both RADV
  GPUs and both descriptor backends;
- D23 reproduces the pattern only with the live invalid `R16_UINT` view;
- D24 repairs it in isolation, while D25 activates the same repair in IL-2 and
  leaves the visible squares unchanged.

The next step is a direct discriminator on the consumer pixel shaders or later
composition pass, as specified in
[`shimmering-ownership-plan.md`](shimmering-ownership-plan.md).
