# Hypotheses and prior art

These are investigation leads, not conclusions. A symptom match is not proof
of a shared cause, and no item below justifies an IL-2 application override on
its own.

## Startup / NUMA track

| ID | Hypothesis | Discriminating evidence | Current confidence |
|---|---|---|---|
| N1 | Wine's `GetNumaNodeProcessorMaskEx` implementation or return semantics are incomplete for the caller. | Identify caller module and arguments/return via focused Wine trace or debugger; compare with current Wine source and Windows. | Low |
| N2 | Wine exposes a topology that Intel OpenMP's affinity discovery rejects even though Linux has one NUMA node and CPUs 0-15. | Capture all NUMA API queries made by the runtime; compare masks, group numbers, node count, and last error. | Low-medium |
| N3 | The game's shipped OpenMP runtime makes a Windows-specific affinity assumption. | Identify the runtime DLL/version; run the same binary on Windows and compare topology queries. | Low |
| N4 | `KMP_AFFINITY=disabled` merely bypasses the defective affinity path; `OMP_NUM_THREADS=16` is incidental or independently required. | Test the two variables separately only after a safe baseline is archived. | Medium that the current pair is mitigation only |

The host itself has a simple one-node, 16-logical-CPU Linux topology. That does
not establish what Wine reports through Win32 processor groups and NUMA APIs.

## Graphics track

| ID | Hypothesis | First discriminator | Escalation if supported |
|---|---|---|---|
| G1 | Host-visible VRAM/ReBAR upload-heap behavior exposes an application or VKD3D lifetime/visibility bug. | `no_upload_hvv` alone; inspect VKD3D memory-topology lines. | Compare ReBAR on/off, allocation path, and upload resource lifecycle. |
| G2 | A missing or insufficient cross-queue dependency affects streamed terrain or menu compute work. | `single_queue` alone; then the planned combination only after G1/G2 individual results. | Queue-specific logging, semaphore/timeline analysis, then narrow instrumentation. |
| G3 | Descriptor-buffer lifetime/type/reuse behavior exposes invalid game descriptors or a translation/driver defect. | E03 disables only `VK_EXT_descriptor_buffer`; D03 excluded placed-resource range aliasing but did not test descriptor-heap image/buffer type reuse. | Descriptor QA or filtered descriptor telemetry. |
| G4 | The game omits a UAV/resource barrier and native drivers implicitly serialize the sequence. | Look for a repeatable change under single queue or controlled heavy synchronization; correlate a specific compute shader/resource sequence. | Narrow shader/resource instrumentation; only then consider a shader-specific barrier quirk. |
| G5 | Reserved/sparse terrain resources or residency updates are mishandled. | Valid D01b trace records zero reserved-resource and tile-mapping calls during reproduction. | Excluded for the tested path; do not pursue without contradictory new evidence. |
| G6 | Incorrect mip/LOD state causes distant terrain to sample absent mips. | D02 found zero non-zero SRV minimum-LOD clamps and complete geometric uploads for 2,355 compressed mip chains, weakening the broad form of this hypothesis. | Correlate the remaining no-upload SRV class with actual binding and explicit-LOD shader use. |
| G7 | Image/buffer descriptor type reuse exposes invalid game use or a translation/driver defect. | D03 found no placed-resource range overlap for all 585 pre-cap candidates and zero explicit legacy alias barriers, excluding resource-memory aliasing for the covered class. E03 now isolates the descriptor-buffer backend. | Use descriptor QA after E03; select `avoid_image_buffer_aliasing` only if descriptor evidence specifically supports image/buffer type confusion. |
| G8 | The repeated split-barrier `END_ONLY` warnings are incidental because VKD3D conservatively completes them. | Count and correlate warning timestamps with resources and visible failures; compare behavior, not warning count alone. | Instrument a suspicious resource's before/after states and layouts. |
| G9 | RADV is given valid Vulkan but mishandles a specific path. | Reproduce with another AMD Vulkan driver or Mesa version, one variable at a time. | Vulkan validation, RenderDoc/trace if lawful and practical, then minimal Vulkan reproducer. |
| G10 | Shader translation produces incorrect code for a menu/terrain shader. | Artifact is invariant under memory, queue, and descriptor controls; shader debug/hash points to a stable stage. | Shader replacement/bisection and minimal shader test. |

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
path during the covered interval, leaving descriptor propagation/use as the
next discriminator.

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

Primary references:

- [IL-2 Korea issue #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134)
- [VKD3D-Proton source at the installed commit](https://github.com/HansKristian-Work/vkd3d-proton/tree/3dfc6f07d0953b1e8b41705275c2c59cc7374fc5)
- [Wuthering Waves issue #2561](https://github.com/HansKristian-Work/vkd3d-proton/issues/2561)
- [Wuthering Waves fix PR #2617](https://github.com/HansKristian-Work/vkd3d-proton/pull/2617)
- [VKD3D-Proton changelog](https://github.com/HansKristian-Work/vkd3d-proton/blob/master/CHANGELOG.md)
- [VKD3D-Proton releases](https://github.com/HansKristian-Work/vkd3d-proton/releases)

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
