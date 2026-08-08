# Hypotheses and prior art

This table retains both historical leads and their current dispositions. A
symptom match is not proof of a shared cause, and no item below justifies an
IL-2 application override on its own.

## Startup / NUMA track

| ID | Hypothesis | Discriminating evidence | Current confidence |
|---|---|---|---|
| N1 | Wine's `GetNumaNodeProcessorMaskEx` implementation is incomplete for the caller. | Exact Proton 11 Wine source and a no-override probe show the API returns `ERROR_CALL_NOT_IMPLEMENTED`; the exact game OpenMP DLL then aborts at that API. | Confirmed |
| N2 | Wine exposes a topology that Intel OpenMP's affinity discovery rejects. | `RelationNumaNode` already exposes a coherent node/group/mask; returning the same data through the missing API lets the exact runtime initialize. | Rejected for the observed one-node topology |
| N3 | The game's shipped OpenMP runtime makes an unsupported Windows affinity query. | The exact `libiomp5md.dll` imports both NUMA functions and reports Intel OpenMP Error #179 when MaskEx is unimplemented. | Confirmed as trigger; the query itself is a documented Windows API use |
| N4 | `KMP_AFFINITY=disabled` bypasses the defective affinity path and `OMP_NUM_THREADS=16` is incidental. | Isolated matrix: affinity-disabled alone succeeds; thread-count-only still fails. | Confirmed for OpenMP initialization |
| N5 | Existing upstream Wine MR !11604 is sufficient when backported to Proton 11. | Exact six-commit D10 build passes the component probe and full Steam startup with launch options empty; the live game maps the patched modules and has no OpenMP/topology override. | Confirmed on the reporting host; cross-topology review remains |

The host itself has a simple one-node, 16-logical-CPU Linux topology. The
candidate does not encode that layout: it queries Wine's existing
`RelationNumaNode` data for the current host on every call. Wine's separate,
pre-existing incomplete processor-group support above 64 logical processors is
outside this focused API implementation.

## Graphics track

| ID | Hypothesis | First discriminator | Escalation if supported |
|---|---|---|---|
| G1 | Host-visible VRAM/ReBAR upload-heap behavior exposes an application or VKD3D lifetime/visibility bug. | `no_upload_hvv` alone; inspect VKD3D memory-topology lines. | Compare ReBAR on/off, allocation path, and upload resource lifecycle. |
| G2 | A missing or insufficient cross-queue dependency affects streamed terrain or menu compute work. | `single_queue` alone; then the planned combination only after G1/G2 individual results. | Queue-specific logging, semaphore/timeline analysis, then narrow instrumentation. |
| G3 | Descriptor lifetime/type/reuse behavior exposes invalid game descriptors or a translation/driver defect. | E03-r1 definitely disables `VK_EXT_descriptor_buffer` and is visually unchanged on the D03-derived build; confirm on stock Proton. This weakens a descriptor-buffer-specific defect but does not test invalid descriptors common to both backends. | GPU-assisted descriptor QA or filtered descriptor telemetry. |
| G4 | The game omits a UAV/resource barrier and native drivers implicitly serialize the sequence. | Look for a repeatable change under single queue or controlled heavy synchronization; correlate a specific compute shader/resource sequence. | Narrow shader/resource instrumentation; only then consider a shader-specific barrier quirk. |
| G5 | Reserved/sparse terrain resources or residency updates are mishandled. | Valid D01b trace records zero reserved-resource and tile-mapping calls during reproduction. | Excluded for the tested path; do not pursue without contradictory new evidence. |
| G6 | Incorrect mip/LOD state causes distant terrain to sample absent mips. | D02 found zero non-zero SRV minimum-LOD clamps and complete geometric uploads for 2,355 compressed mip chains, weakening the broad form of this hypothesis. | Correlate the remaining no-upload SRV class with actual binding and explicit-LOD shader use. |
| G7 | Image/buffer descriptor type reuse exposes invalid game use or a translation/driver defect. | D03 found no placed-resource range overlap for all 585 pre-cap candidates and zero explicit legacy alias barriers, excluding resource-memory aliasing for the covered class. E03 now isolates the descriptor-buffer backend. | Use descriptor QA after E03; select `avoid_image_buffer_aliasing` only if descriptor evidence specifically supports image/buffer type confusion. |
| G8 | The repeated split-barrier `END_ONLY` warnings are incidental because VKD3D conservatively completes them. | D07 renders terrain correctly while retaining 40,408 warnings. | Confirmed incidental for the terrain defect. |
| G9 | RADV is given valid Vulkan but mishandles a specific path. | D07 changes only VKD3D copy geometry and fixes the image on the same RADV build. | Very low for terrain; no Mesa report justified. |
| G10 | Shader translation produces incorrect code for a menu/terrain shader. | Artifact is invariant under memory, queue, and descriptor controls; shader debug/hash points to a stable stage. | Shader replacement/bisection and minimal shader test. |
| G11 | The game texture-provider path fails to resolve, decode, or create required Korea terrain inputs under Wine and substitutes its default white texture. | D07 fixes terrain while the same summer/common fallbacks remain in `tex.log`. | Excluded as the primary rectangular terrain cause; possible secondary missing content only. |
| G12 | The thin baked-terrain border reinterpret geometry alone is the visible cause. | D05c adjusted 202/202 exact thin candidates with zero rejects and visuals remained unchanged. | Excluded as a border-only explanation. |
| G13 | The same missing 1:4 block-unit conversion affects complete `64x64` terrain pages. | D07 finds 178 square `R32G32B32A32_UINT` footprints, converts them to `256x256` BC3 regions, and repairs terrain near 5,500 m; clean general-build D08 repeats the repair. | Confirmed causal for terrain; D08-tested predecessor `cf11ba76` is narrowed without changing the IL-2 branch in current PR commit `64ec55e7`. |
| G14 | The 2048x2048 baked-cache page is otherwise never populated, not made visible, or sampled through the wrong descriptor/page index. | D07 repairs terrain without changing descriptors, synchronization, or shader selection. | Excluded as the primary terrain mechanism. |
| G15 | The menu shimmer is produced by the game's tiled variable-rate-shading path or VKD3D/RADV translation of it. | E05 removes `VK_KHR_fragment_shading_rate`, makes VKD3D advertise no D3D12 VRS support, and leaves the same moving squares visible. | Weakened: VRS is not required for the reproduced artifact; do not pursue without contradictory runtime evidence. |
| G16 | A screen-space or temporal reflection resource contains stale or is sampled/interpreted incorrectly. | D12 records about 2,003 stable render/resolve cycles with explicit flag-zero transitions and stable shaders. D14 proves the correlated pixel shaders are tiled-light consumers as well as reflection-target writers. The artifact remains. | Simple named-target transition failure weakened; reflection alone is too broad. Follow the tiled-light inputs to these passes. |
| G17 | The game's tiled dynamic-light reference list or self-light input is stale, incorrectly synchronized, or mistranslated. | D14 identifies the exact six-stage producer and proves that both pixel shaders read the 3D uint grid plus a separate uint light-index buffer. D15 records 1,593 complete final cycles: both resources receive the required UAV dependencies and shader-read transitions, and producer atomics use Device scope. The artifact remains. | Missing synchronization is excluded for this sequence. Resolve the fixed `t9`/`t10` descriptors next, then inspect produced values or a subtler translation/compiler defect. Do not add a barrier quirk from the current evidence. |

The cross-configuration screenshot set shows substantially worse page loss
near 5,000-6,300 m and more low-fidelity content near 1,250-1,900 m. Valid D01b
instrumentation excludes D3D12 reserved/tiled resources, so the remaining
streaming lead is the game's ordinary texture paging, mip/LOD, descriptors, or
uploads rather than API-level sparse residency.

Read-only inspection of the compiled game files is consistent with that split:
the executable names high-level tiled terrain/LOD classes, while
`dxBackend12.dll` exposes ordinary committed/placed resources, mip SRVs,
subresource updates, and copies. This selects G1/G6 for focused telemetry but
does not establish whether the game, VKD3D-Proton, or RADV mishandles them. D02
shows complete mip uploads for most compressed textures and zero non-zero SRV
minimum-LOD clamps, but also a distinct pre-cap class of 405 SRV-bearing BC3 textures
without an observed incoming upload. That class is not yet proven to be sampled
or defective. D03 excludes placed-resource range aliasing as its population
path during the covered interval. D04 is unchanged on current upstream and
adds direct game-log evidence of failed terrain inputs. Re-analysis of D02 now
originally promoted G12 because it connected the engine's named
`stitchBorders` architecture to 432 suspect copy regions. D05c then executed
the exact border-only mapping without visual change. D06 exposed the
interior-page geometry, and D07 confirmed G13 by adjusting every encountered
interior and border copy and repairing the high-altitude terrain. D08 validates
the general `cf11ba76` implementation without the diagnostic gate. Current PR
commit `64ec55e7` preserves that conversion only for equal-byte formats whose
block dimensions differ.
Package inspection independently confirms the engine's 800 m baked-page
geometry; see `evidence-map-package-inspection.md`.

The remaining square artifact is a separate graphics batch. E05 weakens VRS,
and D12 weakens a simple transition failure on the actively rendered SSR
targets. The same run expands the symptom from the menu to the cockpit and a
burning-aircraft scene. D13 then records stable, explicit clear/transition/UAV
cycles for `rtSelfLight` and `rtLightRefs`. D14 proves that the nearby pixel
shaders read the tiled-light grid and its separate index buffer, and discovers
two final producer stages outside D13's logging window. Their translated code
is structurally faithful. D15 then proves that the separate buffer and 3D grid
receive the application-supplied dependencies and final read transitions in
1,593 covered cycles while the symptom remains. Runtime descriptor/view
selection and computed values are now the focused leads; a forced barrier is
not justified.

## Direct public report

[VKD3D-Proton issue #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134)
documents the same game and same tile-shaped missing terrain on an RX 9070 XT
with RADV/Mesa 26.1.3. Its log confirms D3D12 through VKD3D-Proton and shows the
normal host-visible upload, descriptor-buffer, sparse-resource, and multi-queue
capabilities enabled together. It therefore corroborates the defect but does
not isolate one of those paths. See [`external-evidence.md`](external-evidence.md).

## Relevant resolved cases in other games

The installed VKD3D-Proton commit postdates these fixes, so updating blindly to
“a version containing the fix” cannot apply them to IL-2 automatically. Their
value is the mechanism and debugging method.

1. **Wuthering Waves:** broken/flickering textures were narrowed to a missing
   UAV barrier between a custom clear shader and light injection. VKD3D-Proton
   PR [#2617](https://github.com/HansKristian-Work/vkd3d-proton/pull/2617)
   applied `VKD3D_SHADER_QUIRK_FORCE_PRE_COMPUTE_BARRIER` to one shader hash.
   This is strong prior art for G4, but IL-2 needs its own resource/shader
   evidence before any comparable quirk.
2. **Satisfactory:** current VKD3D-Proton carries a shader-specific missing
   barrier workaround reported to fix corrupt rendering, especially on AMD.
   This again supports investigating G4 narrowly rather than treating all split
   barriers as defective.
3. **Arma Reforger:** VKD3D-Proton 2.14.1 release notes record a
   `no_upload_hvv` workaround for unusual asset-loading behavior. This makes
   E01 a high-value existing control for G1, not an assumed fix.
4. **Deathloop and DIRT 5:** historical fixes include missing synchronization,
   host-visible image bugs, and `ResourceMinLODClamp`/black-ground behavior.
   These distinguish G4/G6 from a generic “terrain is black” diagnosis.
5. **Wreckfest 2 and Rise of the Tomb Raider:** current workarounds document
   illegal texture aliasing and render-while-sample/compression behavior. They
   justify inspecting G7 only if the simpler controls fail.
6. **Microsoft Flight Simulator 2024:** two separate failures were fixed in
   `dxil-spirv`, including a grass/near-ground crash caused by a translator bug
   which `spirv-val` did not catch. This justified D04, but D04 is visually
   unchanged and closes the current-upstream version lead.
7. **Microsoft Flight Simulator 2020:** its `host_import_fallback` advice is
   tied to an exceptional 16 GiB `OpenExistingHeapFromAddress` allocation and
   unusable AMD performance. IL-2 has no matching evidence, so this flag is not
   selected without a focused host-import trace.

Primary references:

- [IL-2 Korea issue #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134)
- [VKD3D-Proton source at the installed commit](https://github.com/HansKristian-Work/vkd3d-proton/tree/3dfc6f07d0953b1e8b41705275c2c59cc7374fc5)
- [Wuthering Waves issue #2561](https://github.com/HansKristian-Work/vkd3d-proton/issues/2561)
- [Wuthering Waves fix PR #2617](https://github.com/HansKristian-Work/vkd3d-proton/pull/2617)
- [VKD3D-Proton changelog](https://github.com/HansKristian-Work/vkd3d-proton/blob/master/CHANGELOG.md)
- [VKD3D-Proton releases](https://github.com/HansKristian-Work/vkd3d-proton/releases)
- [MSFS prior-art assessment](prior-art-msfs.md)

## Decision gates

- Do not add any application override after only one run.
- A generic flag must materially improve at least two runs before building a
  one-flag experimental application override.
- A shader barrier quirk requires a stable shader/resource sequence and a
  demonstrated missing dependency, not only a visual resemblance to prior art.
- Driver attribution requires valid Vulkan evidence and a cross-driver or
  cross-Mesa discriminator.
- The NUMA and graphics tracks never share a root-cause conclusion merely
  because the same launch command is used.
