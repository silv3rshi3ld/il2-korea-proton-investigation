# D21-D25 tiled-light atomic investigation: result

> [!IMPORTANT]
> Historical interim result. D25 emitted a StorageBuffer atomic but still
> selected the typed texel-buffer descriptor, so it did not execute the complete
> compatibility translation. D44-D47 later identified that missing descriptor
> half and established the allocator as the visual root cause. D50-D52 then
> proved that an R32 texel-buffer view repairs the exact path without changing
> dxil-spirv. The still-open Mesa MR !43672 became the preferred upstream
> direction, but was not locally game-tested here. See
> [`evidence-d45-correct-ssbo-binding-result.md`](evidence-d45-correct-ssbo-binding-result.md),
> [`evidence-d47-allocator-only-wired-result.md`](evidence-d47-allocator-only-wired-result.md),
> [`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md),
> and [`final-report.md`](final-report.md). The text below is preserved as the
> accurate conclusion from the evidence available at D25.

## Result

The D20 captures exposed a real invalid descriptor/atomic combination, but
D25 proves that correcting it does **not** remove the visible square artifact.
The atomic path must therefore not be presented as the visual root cause or as
an upstream fix for the shimmering.

The tests establish four narrower facts:

1. RADV correctly executes ordinary and exact translated `R32_UINT` global
   atomics across all workgroups (D21 and D22).
2. IL-2 binds the allocation UAV as `R16_UINT` even though
   `ComputeLightsFirstRef` performs a 32-bit interlocked add through it. This is
   an invalid or at least non-portable typed-UAV combination.
3. Reproducing that live `R16_UINT` view produces the same per-workgroup restart
   pattern on both tested RADV GPUs, while translating the counter access as a
   storage-buffer atomic produces correct standalone output (D23 and D24).
4. The same storage-buffer translation was activated for the exact shader in
   IL-2, yet the user still saw the same illuminated square blocks (D25).

Consequently, D20's malformed prior-frame values are correlated evidence, not
a demonstrated causal mechanism for the pixels. They may belong to a benign,
dead, overwritten, or differently interpreted part of the resource history.
The visual investigation remains open and moves to the tiled-light consumer
shaders and later render passes.

## D21 and D22: valid Vulkan paths pass

The headless probe was run with Vulkan validation on both RADV devices:

- AMD Radeon RX 7800 XT, RADV NAVI32, driver 26.1.6;
- AMD Ryzen 7 7800X3D integrated device, RADV RAPHAEL_MENDOCINO, driver 26.1.6.

For D21, both the storage-texel-buffer and SSBO variants returned exactly one
copy of every allocation value from 0 through 2,719. The final counter was
2,720, only one workgroup contained zero, and validation reported zero errors.

D22 ran the exact captured `ComputeLightsFirstRef` SPIR-V with a legal
`R32_UINT` counter through both the mutable-descriptor-set and
`VK_EXT_descriptor_buffer` backends. All four GPU/backend combinations passed:

- counter: 8,160 of expected 8,160;
- overlaps: 0;
- missing entries: 0;
- workgroups containing zero: 1;
- validation errors: 0.

This rejects a general RADV storage-texel-buffer atomic failure and an error in
the descriptor-buffer backend for the legal descriptor shape.

## D23: reproduce the live descriptor mismatch

D16 resolved the game's live allocation/index UAV as an 87,040-byte buffer
viewed as 43,520 `R16_UINT` elements. The captured compute shader performs a
32-bit atomic through that typed UAV. D23 therefore runs the exact original
SPIR-V while deliberately binding the counter descriptor as the game's
`R16_UINT` view.

Both GPUs and both descriptor backends produce the same deterministic result:

- final interpreted counter: 0 instead of 8,160;
- overlapping intervals: 7,966;
- missing entries: 7,966;
- workgroups containing zero: 50;
- validation errors: 0.

The absence of a validation message does not make the D3D usage correct; the
Vulkan descriptor is internally valid, while the mismatch is between the
32-bit shader operation and the 16-bit D3D typed view that led to it.

## D24: standalone compatibility translation

D24 preserves the game's `R16_UINT` storage for its ordinary index accesses
but translates the allocation counter as a raw 32-bit storage-buffer atomic.
Both GPUs and both descriptor backends then pass with counter 8,160, zero
overlaps, zero missing entries, one workgroup containing zero, and zero
validation errors.

This proves that a narrowly compatible translation can repair the standalone
allocation pattern. It does not prove that the allocation pattern causes the
rendered squares.

## D25: authoritative in-game negative

D25 implemented the D24 translation in a local VKD3D-Proton build for only:

- application name `IL2Series.exe`;
- shader hash `7cefa1bc80bb4c70` (`ComputeLightsFirstRef`).

Run identity:

- compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`;
- VKD3D-Proton base: `84c87c8390d9df7` plus the local diagnostic change;
- run directory: `/tmp/il2-D25-light-atomic-compat-r1`;
- original DXIL SHA-256:
  `3e249c5bc6d596371907aa7a4c653f13a0e92e62d094222aa65932a2772df236`;
- translated SPIR-V SHA-256:
  `81d64d3aac62a95d3334e3f1048b5dfc859806a66d58661a8528c83f025cf0f8`.

The log records the exact program name, detects the game, applies its shader
quirks, and reports the intended local build. The dumped target shader differs
from the D14 translation exactly as required: set 1 binding 1 is a
`StorageBuffer` with a 32-bit array stride and the allocation uses
`OpAccessChain` plus `OpAtomicIAdd`, rather than `OpImageTexelPointer`.
`spirv-val --target-env vulkan1.3` accepts it.

Despite that verified activation, the user visually observed the square
artifact. Screenshot evidence:

- source: `/home/USER/Pictures/Screenshots/Screenshot_20260808_122417.png`;
- dimensions: 1769x817;
- SHA-256:
  `7313a7cd81db0bf6764726b60d8ce2005277ac3cfb78df454579802df3ab0bd8`;
- classification: translucent square blocks remain across the lit aircraft
  and floor.

There was no device loss, validation failure, or GPU fault that would
invalidate the comparison.

## Ownership and compatibility conclusion

The game should principally bind resources consistently with the width and
operations declared by its shader. In practice, compatibility projects also
need to run shipped software that native Windows drivers accept, so a narrow
compatibility allowance can be appropriate when it restores observable native
behavior without harming conformant applications.

D25 fails that essential causal test: the allowance changes the questionable
operation but does not fix the reported pixels. It should therefore remain
local diagnostic evidence, not be proposed as an IL-2 quirk or shimmering PR.
If later evidence identifies a different Windows-tolerated renderer behavior,
the same principle applies: prefer correct application usage, but implement a
surgical compatibility rule when that is the practical, demonstrated route to
native-equivalent output.

## Next discriminator

The next test should operate on the actual pixel-producing dependency rather
than infer causality from a captured prior-frame buffer. The two known consumer
pixel shaders are:

- `df0bd777fd1bb89d` (`PixOutLight_msp`);
- `a2d104d5c813322e` (`PixOutLight_mss`).

A local diagnostic should selectively suppress or substitute their tiled-light
list input while preserving the rest of the draw. If the squares disappear,
the defect is still in the light-list consumer chain but is downstream of the
D25 counter. If they remain, the investigation should move to the reflection
target write/blend or a later composition pass. This is a diagnostic gate, not
a proposed game modification or final workaround.

## D50-D52 final refinement

The D25 visual negative was later explained by its mismatched operation and
descriptor selection. D50 removed a remaining control gap by using the same
minimal shader, pipeline, dispatch, and 87,040-byte buffer for an
`R32_UINT`, `R16_UINT`, `R32_UINT` sequence. On both tested RADV devices, both
R32 runs were globally correct and only R16 reproduced the workgroup restart.

D51 used the exact captured shader with the full-size R32 view. All four
device/backend combinations completed 8,160 allocations with zero overlaps,
missing entries, out-of-range intervals, or writes beyond the first counter
word. D52 carried only that descriptor choice into VKD3D-Proton, left
dxil-spirv unchanged at `cc75a0c9`, and removed the square blocks in two runs.

This establishes the allocator as the visual root cause and the texel-buffer
view/OOB behavior as the decisive implementation boundary. The SSBO path was a
useful diagnostic but is not necessary. Mesa MR !43672 now addresses that
behavior at the RADV driver level and supersedes both the SSBO and R32-alias
quirks as upstream proposals.
