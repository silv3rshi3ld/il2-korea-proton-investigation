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
| G17 | The game's tiled dynamic-light reference list contains overlapping allocation ranges, and those ranges cause the visible squares and broad flicker. | D20 finds all 50 workgroups independently allocating from zero. D23 reproduces it with the invalid `R16_UINT` view. D44 captures the same 320-entry ceiling under D42. D47 repairs it through an SSBO, while D52 repairs it with the natural typed shader and an R32 alias. | Confirmed visible mechanism. D50 through D52 refine the root cause to the mismatched view and out-of-bounds behavior, not a required descriptor class. |
| G18 | RADV incorrectly executes an otherwise legal device-scope `R32ui` storage-texel-buffer atomic as though it were workgroup-local. | D21 and exact-shader D22 pass on both RADV GPUs through mutable descriptor sets and descriptor buffers. The failure appears only when D23 binds the 32-bit atomic shader through the live `R16_UINT` view. | Rejected. RADV correctly executes the legal form; D23 is an application descriptor/operation mismatch rather than a general driver atomic bug. |
| G19 | The visible blocks arise inside the tiled dynamic-light loop of the two correlated pixel shaders rather than solely in a later write/blend/composition stage. | D26 replaces only the packed `t9` start/count fetch with zero, preserving the rest of both shaders and the scene. Both runtime overrides are verified and the square grid disappears completely. | Confirmed causal boundary. |
| G20 | Genuine record 2 can trigger the grid rather than loop count, common record-1 math, or sentinel skipping alone. | D27 makes every iteration evaluate record 1 and is clean. D31 eliminates all skips while preserving genuine records and remains defective. D32 keeps only records 1 and 2, with no skipped iterations, and the squares remain. D33 proves record 2 is in bounds in the live `t7` view and its captured values are finite. | Confirmed sufficient for the covered scene. The remaining distinction from safe record 1 is record-2-specific spotlight/shadow behavior, not descriptor placement or bounds. |
| G21 | The apparent `t7`/`t8` reversal in raw captured descriptor bytes is a live D3D binding error. | D33 resolves 9,064/9,064 target lookups: `t7` is the 2,048-byte float4 record view, `t8` is the 4096x2048 shadow image, and both are inside the declared `t0`–`t22` range. | Rejected. Raw mutable-descriptor physical encoding was not authoritative for logical D3D heap placement. |
| G22 | Record 2's shadow projection/comparison result, absent for safe record 1, is required for the visible squares. | D34 preserves the D32 record-1/record-2 mixture and all ordinary record-2 lighting but substitutes fully visible `1.0` for the final `t8` comparison/filter result. Both overrides load and the squares remain. | Rejected for the returned visibility value. Do not pursue shadow sampling as the next fix. |
| G23 | The grid comes from screen-tiled `t9`/`t10` membership or iteration multiplicity rather than record 2's per-pixel spotlight calculation alone. | D27 keeps real counts with safe record 1 and is clean; D32 keeps genuine record-2 membership and is defective; D35 removes all list variation, evaluates record 2 once everywhere, and removes the squares. | Confirmed causal boundary. The real tile-dependent placement/evaluation mask for record 2 is required; inspect producer culling and post-D25 list values. |
| G24 | Record 2's producer culling has false-negative tiles, exposing its otherwise smooth per-pixel contribution as square boundaries. | D36 retains the original consumers, every real light, and normal shadows while both producers use `original_membership || light_id == 2`; the blocks return. | Rejected as a sufficient explanation. Record 2's boundary alone is not the complete defect. |
| G25 | The artifact requires tile-dependent placement of a broader nontrivial light class or interaction between genuine records, rather than record 2 alone. | D27 is clean when all real iterations use record 1; D35 is clean with only global record 2; D36 restores genuine record diversity while making record 2 global and the blocks return. D44 later identifies the unstable overwritten allocator list upstream of these consumers. | Superseded as a root-cause lead. It describes how corrupted membership becomes visible, not why the list is corrupted. |
| G26 | Degenerate normalization in the shared producer culling math yields infinite `rsqrt`, then NaN, causing false-negative tile membership on Vulkan where Windows drivers tolerate the game math. | Runtime D37 applies the existing finite-`rsqrt` quirk to both exact producers. Dumps prove all 12 operations per module receive finite clamps and validate, but the original blocks return. | Rejected as sufficient. Do not promote the finite-`rsqrt` quirk for IL-2. |
| G27 | The shared final packed tile-depth mask/min-max gate falsely rejects otherwise geometrically intersecting local lights. | D38 hides the blocks while the list remains malformed; D39 and D40 restore their presentation. Correctly wired D47 retains the complete original depth gate and is clean after repairing the allocator. The fine residual film-grain lighting is confirmed native on Windows. | Rejected as an independent defect or final fix. It controlled presentation of corrupted membership data only. |
| G28 | Correct behavior requires changing the shader to an SSBO in dxil-spirv. | D50 holds the operation and allocation constant and passes with R32, fails with R16, then passes with R32. D51 passes the exact shader with a live-sized R32 alias through both descriptor backends. D52 keeps stock dxil-spirv and retains `R32ui`, `OpImageTexelPointer`, and `OpAtomicIAdd`; two game runs are clean. | Rejected. D47 and D49 were useful repair experiments, but neither SSBO lowering nor a dxil-spirv change is inherently required. |
| G29 | GFX10+ RADV's texel-buffer out-of-bounds selection differs from native AMD and exposes the game's invalid R16/32-bit atomic use. | D50 through D52 isolate the view-format boundary. Mesa MR !43672 changes RADV from `STRUCTURED_WITH_OFFSET` to `STRUCTURED`, matching native AMD D3D12 and pre-GFX10 behavior. | Leading upstream explanation. It agrees with local evidence, but the Mesa MR remains open and has not yet been tested locally. |

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
1,593 covered cycles while the symptom remains. D16 then resolves every
covered `t9`/`t10` lookup to the exact D15 resources with the expected view
types and shapes. Descriptor selection is now closed; computed values,
typed-buffer code generation, or translated Vulkan cache handling remain
focused leads. D17 leaves the artifact unchanged even with RADV full cache
flushes and waits after every draw/dispatch, closing ordinary translated cache
visibility as well. D18 remains defective with DCC disabled; a possible visual
regression is too uncertain to classify but means compression influence is not
fully excluded. D19 remains unchanged with forced ACO waits, using fresh
compilation because Mesa disables its pipeline caches for ACO code-generation
debug flags. The light-aligned appearance and negative controls now select
produced-value capture rather than another broad launch-option test.

D44 through D49 later prove the allocator mechanism, and D50 through D52 close
the implementation ambiguity. The exact typed shader succeeds with an R32
alias and stock dxil-spirv, so compiler lowering is no longer a selected lead.
The current upstream direction is Mesa MR
[!43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672), which
matches RADV's texel-buffer out-of-bounds selection to native AMD and pre-GFX10
behavior. The local evidence supports that explanation, but a patched-Mesa
runtime test remains outstanding. See
[`evidence-d50-d52-r32-alias-result.md`](evidence-d50-d52-r32-alias-result.md).

## Direct public report

[VKD3D-Proton issue #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134)
documents the same game and same tile-shaped missing terrain on an RX 9070 XT
with RADV/Mesa 26.1.3. Its original log confirms D3D12 through VKD3D-Proton and
shows the normal host-visible upload, descriptor-buffer, sparse-resource, and
multi-queue capabilities enabled together. Later maintainer reproduction and
the D50 through D52 view-format isolation now point to Mesa MR !43672 rather
than one of those broad paths. See [`external-evidence.md`](external-evidence.md).

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
