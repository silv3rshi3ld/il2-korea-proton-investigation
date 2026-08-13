# D50-D52 texel-buffer format isolation: result

Date: 2026-08-11

## Result

D50 through D52 isolate the decisive variable to the Vulkan texel-buffer view
and its out-of-bounds handling. They also show that changing dxil-spirv's
atomic lowering is unnecessary.

IL-2 creates an 87,040-byte UAV with an `R16_UINT` view while exact shader
`0x7cefa1bc80bb4c70` performs a scalar 32-bit atomic through an `R32ui` storage
texel buffer. On both tested RADV devices, changing only that view from
`R16_UINT` to `R32_UINT` changes the allocation from the reproduced
per-workgroup restart to one correct global allocation. A narrowly scoped
VKD3D-Proton diagnostic then routed the exact shader to an `R32_UINT` sibling
without modifying dxil-spirv. The square blocks were absent in two game runs.

This is causal diagnostic evidence, not a merge-ready VKD3D-Proton fix. After
reproducing the game, Hans-Kristian Arntzen proposed
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672),
which changes RADV's GFX10+ texel-buffer OOB selection from
`STRUCTURED_WITH_OFFSET` to `STRUCTURED`. That matches the native AMD D3D12
driver and pre-GFX10 behavior. Hans also reported that NVIDIA passes the test
with a descriptor heap. The Mesa direction is therefore cleaner and more
general than either the D49 dxil-spirv lowering or the D52 per-game alias. It
is the preferred upstream direction while the Mesa MR is reviewed.

The superseded implementation drafts, dxil-spirv PR #296 and VKD3D-Proton PR
#3207, were closed unmerged. Mesa MR !43672 remains open, and this
investigation has not game-tested its exact revision.

## D50: same buffer, same shader, view-format A/B

D50 closes a control gap in D23. The earlier legal R32 control used a four-byte
counter, while the failing R16 case used the game's full allocation. D50 uses
one 87,040-byte buffer, one minimal coordinate-zero atomic shader, one compute
pipeline, and the same dispatch topology for this sequence:

1. `R32_UINT`;
2. `R16_UINT`;
3. `R32_UINT` again.

The buffer and output are cleared between runs. Only the bound texel-buffer
view changes. On both the RX 7800 XT and the integrated Raphael RADV device:

- both `R32_UINT` runs produce one correct 2,720-value global allocation;
- the `R16_UINT` run reproduces the corrupt per-workgroup restart;
- returning to `R32_UINT` immediately restores correct output.

Vulkan reports storage-texel-buffer atomic support for `R32_UINT`, not
`R16_UINT`. Validation also identifies the `R32ui` shader and `R16_UINT` view
format mismatch. The repeat sequence excludes buffer size, allocation reuse,
pipeline compilation, and dispatch topology as the discriminator.

## D51: exact game shader with a full-size R32 alias

D51 runs the unmodified captured `ComputeLightsFirstRef` SPIR-V against an
87,040-byte `R32_UINT` view. It uses the game's `80x34x2 R32_UINT` output grid
and deterministic per-tile counts. On both RADV devices, and through both the
mutable descriptor-set and `VK_EXT_descriptor_buffer` backends, every run has:

- final counter: 8,160 of 8,160;
- count mismatches: 0;
- out-of-range intervals: 0;
- overlaps: 0;
- missing entries: 0;
- layer-one mismatches: 0;
- workgroups containing zero: 1;
- nonzero bytes after the first 32-bit counter word: 0.

D51 proves that the exact existing texel-buffer shader works with the game's
full backing-buffer size when it receives an `R32_UINT` view. No SSBO lowering
is needed for that result.

## D52: VKD3D-Proton-only runtime discriminator

The original game-tested D52 was an uncommitted local diagnostic based on:

| Component | Identity |
| --- | --- |
| VKD3D-Proton | `84c87c8390d9df75ba41d911496296fe13f0e275` |
| dxil-spirv gitlink, unchanged | `cc75a0c98d34d7bcc03560527c799b52e48b4d1f` |
| Custom Proton tool | `IL2-Korea-D52-R32Alias-84c87c83-r2` |
| Game build | `24615759` |

On 2026-08-13, the same source change was forward-ported to VKD3D-Proton
parent `238f157e1d64f90e0d90593557c092ab8af6e0a3` and published on the
personal fork as commit
[`8cd28e8f98751afe3b85c3b08519464907aa5143`](https://github.com/silv3rshi3ld/vkd3d-proton/commit/8cd28e8f98751afe3b85c3b08519464907aa5143)
on branch
[`diagnostic-il2-r32-alias-d52`](https://github.com/silv3rshi3ld/vkd3d-proton/tree/diagnostic-il2-r32-alias-d52).
The x86-64 and x86 builds passed. The forward-port was not installed or rerun
in the game, so the two clean game runs and binary identities below apply only
to the original `84c87c83` diagnostic. The source link was added in a
[follow-up comment on closed PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207#issuecomment-5276514345)
for inspection only, with no request to reopen the PR or merge the alias.

The candidate leaves the normal `R16_UINT` descriptor in place. For only
`IL2Series.exe`, shader `0x7cefa1bc80bb4c70`, and the exact full-resource UAV
shape, it creates an `R32_UINT` storage-texel-buffer sibling in the available
raw descriptor slot and remaps only `u1` to that sibling. Unsupported descriptor
layouts retain the normal typed path. This narrow scope makes D52 useful as a
discriminator, but the sibling-slot assumptions and per-game policy are also
why it is not proposed for merging.

Both MinGW architectures and the complete package build passed. The installed
DLL identities are:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `beecadd5579db1decf33ab13db7b6e9138eea78768f861bf77362f54e9dd1cb8` |
| x86-64 | `d3d12core.dll` | `dfecb221202c1dd6c0532fa3fbd5cbe4764e7736efb752290bbb64d24a2be86e` |
| x86 | `d3d12.dll` | `540e32dc651a65ed0d0617ed924393835d4f4bd37f9b917e95a0f6cd57a68af9` |
| x86 | `d3d12core.dll` | `e8d1b8f4994e2ce2c2462eed00cf1507a2a3bfb688a54cbfde63db85d41cee26` |

The diagnostic run records this marker exactly once:

```text
vkd3d_create_buffer_uav_embedded: Creating R32_UINT texel-buffer alias for IL-2 tiled-light allocator UAV.
```

The captured DXIL remains byte-identical to D14:

| Artifact | Baseline SHA-256 | D52 SHA-256 |
| --- | --- | --- |
| DXIL | `3e249c5bc6d596371907aa7a4c653f13a0e92e62d094222aa65932a2772df236` | `3e249c5bc6d596371907aa7a4c653f13a0e92e62d094222aa65932a2772df236` |
| SPIR-V | `dc41155e335ea72ee29485610ebef683a2f7fef88e898c64e66ae7bbea3772a7` | `c68b701b0ae648a31fb975a36a9f736a26ceb70bf95fada30387d5fb2dda49e3` |

The SPIR-V retains `R32ui`, `OpImageTexelPointer`, and `OpAtomicIAdd`. Its
disassembly differs from the D14 baseline only in the affected resource's two
decorations: descriptor set/binding changes from `1/1` to `2/0`. This confirms
that D52 changed descriptor selection, not compiler lowering.

The first game run captured the marker and shader dump. A second run omitted
all VKD3D diagnostic environment. The user reported no square blocks in either
run. Both used
`OMP_NUM_THREADS=16 KMP_AFFINITY=disabled %command%` only to bypass the
independent Wine startup problem. D52 deliberately excludes terrain PR #3202,
so the terrain map was outside this test's acceptance scope.

Sanitized source and runtime artifacts are retained as
[`../patches/0017-vkd3d-proton-Add-diagnostic-R32-texel-alias-for-IL2.patch`](../patches/0017-vkd3d-proton-Add-diagnostic-R32-texel-alias-for-IL2.patch)
and
[`../captures/curated/d52-r32-alias/runtime-proof.txt`](../captures/curated/d52-r32-alias/runtime-proof.txt).
The patch is a reproducibility artifact, not an upstream proposal.

## Root cause and upstream boundary

The evidence now supports this chain:

1. The game relies on a 32-bit atomic through an `R16_UINT` typed view.
2. The legal R32 texel-buffer form works on both tested RADV devices and both
   tested descriptor backends.
3. The R16 view deterministically produces the same malformed allocation seen
   in the game, even at coordinate zero.
4. Replacing only that view with an R32 alias restores the exact shader and the
   game rendering without changing dxil-spirv.
5. Mesa MR !43672 identifies RADV's GFX10+ texel-buffer OOB selector as the
   driver behavior that differs from native AMD D3D12 and pre-GFX10 hardware.

Principally, the game should use a view compatible with its shader operation.
Practically, matching the native driver behavior is appropriate for shipped
software when it can be done generally and without a per-game compiler or
VKD3D-Proton hack. D50-D52 helped isolate that boundary. They do not need to be
merged, and the earlier draft detour caused no lasting change because neither
draft implementation was merged.

Mesa MR !43672 has not yet been independently runtime-tested in this
investigation. Acceptance of the architectural direction is therefore distinct
from local validation of that exact Mesa commit. The decisive final check is
unmodified VKD3D-Proton and dxil-spirv on a Mesa build containing the MR,
followed by upstream review and normal Mesa/Proton delivery.
