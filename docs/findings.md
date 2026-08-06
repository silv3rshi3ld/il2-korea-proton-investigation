# Findings and evidence ledger

## Verified findings

1. Steam AppID 247970 currently launches the 64-bit PE executable
   `bin/game/IL2Series.exe` from the `IL2Series` install directory.
2. `IL2Series.exe` statically imports `dxBackend12.dll`; that game backend
   imports `d3d12.dll` and `dxgi.dll` and exposes numerous D3D12 resource,
   barrier, command-list, descriptor, and texture operations.
3. No game-local D3D12, DXGI, D3D11, DXVK, or VKD3D DLL overrides were found.
4. The prefix D3D12 DLL hashes match VKD3D-Proton in the selected Proton
   Experimental build. Prefix DXGI and D3D11 hashes match that build's DXVK.
5. The selected VKD3D-Proton commit is
   `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`; it already contains many older
   game-specific synchronization, memory, aliasing, compression, and shader
   workarounds. A generic upgrade to those old fixes is therefore not an IL-2
   solution.
6. RADV exposes descriptor buffers, sparse resources, host-visible/ReBAR-capable
   memory topology, and multiple queue families on this host. Runtime logs
   confirm descriptor-buffer, host-visible upload, and multiple queue paths;
   they do not expose whether the affected terrain uses sparse resources.
7. A 16 GiB PCI BAR is mapped for the RX 7800 XT, strongly indicating ReBAR is
   enabled.
8. Linux reports one NUMA node containing CPUs 0-15. Wine/Windows API topology
   as seen by the shipped OpenMP runtime has not yet been captured.
9. VKD3D-Proton issue #3134 independently records the same tile-shaped terrain
   loss and menu squares on an RX 9070 XT with Mesa 26.1.3. Its log confirms the
   D3D12/VKD3D path, host-visible upload heaps, descriptor buffers, sparse
   capability, and multiple queue paths, but does not isolate one as defective.
10. Controlled local runs E00-r1 and E00-r2 reproduce the defect with no VKD3D diagnostic
    option. Runtime module loading confirms `dxBackend12.dll`, DXVK DXGI, and
    VKD3D-Proton D3D12/D3D12Core, with no D3D11 module load.
11. Both E00 runs used host-visible device-local upload heaps, ReBAR budgeting,
    descriptor buffers, and three VKD3D queue workers. This confirms all three
    planned controls address active baseline paths, but does not select a cause.
12. Neither E00 run produced Vulkan device loss, GPU hang/reset, or
    out-of-memory errors. Split `END_ONLY` warnings occur throughout both runs
    but remain non-causal without resource correlation.
13. `VKD3D_CONFIG=no_upload_hvv` is confirmed active in both E01 runs and
    changes the upload heap from device-local host-coherent memory to forced
    host-coherent memory. Both runs show denser vegetation, but both were
    captured near 1,350-1,500 m. Baseline and E02 screenshots show substantially
    less content near 5,000 m and more near 1,245-2,500 m. The E01 visual result
    is therefore altitude-confounded and classified inconclusive.
14. `VKD3D_CONFIG=single_queue` is confirmed active in both E02 runs. Logged
    staggered submissions use queue family 0, but menu and terrain corruption
    remain unchanged. Async compute/transfer queue selection is therefore
    unlikely to be the primary cause; single-queue execution does not rule out
    synchronization or lifetime defects within the remaining queue path.
15. An unmodified x64/x86 VKD3D-Proton development build now succeeds from the
    exact installed source commit using the official retained-build method.
    Its architecture and exports are valid. Runtime parity with the packaged
    Proton DLLs is the next control and has not yet been claimed.

## Observations not yet promoted to findings

- `KMP_AFFINITY=disabled` plus `OMP_NUM_THREADS=16` allows startup. The exact
  responsible variable and caller are not isolated.
- Split `END_ONLY` barrier warnings span most of E00-r1 and issue #3134, but
  their temporal/resource relationship to corruption is unknown.
- The game creates `BlocksCache` CPU threads on mission entry. The ordinary log
  does not connect them to a D3D12 resource or the visible missing pages.
- Screenshot evidence shows strong altitude/distance dependence across several
  configurations. The user reports that additional low-fidelity assets begin
  loading below roughly 1,500 m, while captures near 5,000 m show substantially
  greater loss. The ordinary logs contain no altitude, mip, tile-mapping,
  sparse-resource, or residency telemetry, so the mechanism is unproven.
- Rendering symptoms are consistent with several failure classes, including
  missing synchronization, upload visibility, descriptor misuse, sparse
  residency, mip/LOD selection, aliasing, compression, shader translation, and
  a driver bug. Visual appearance cannot select among them.

## External-report and prior-art assessment

Issue #3134 is a direct report of this defect and will be the VKD3D-Proton
handoff target. It has no maintainer diagnosis or resolution yet. Among
resolved *other-game* examples, Wuthering Waves had flickering broken textures
fixed by a shader-specific barrier quirk after identifying a missing UAV
dependency. Satisfactory also has a targeted missing-barrier workaround. Arma
Reforger's unusual asset loading motivated a `no_upload_hvv` workaround. These
cases increase the diagnostic value of E01, E02, and later resource/shader
tracing; they do not justify copying an override.

## Root-cause assessment

| Track | Assessment | Confidence |
|---|---|---|
| Startup | Affinity discovery in the shipped OpenMP path is bypassed by the current environment variables; the responsible Wine API/topology/runtime behavior is unproven. | Low |
| Graphics | The game definitely uses D3D12 through VKD3D-Proton and DXVK's DXGI on this setup. The defective layer and mechanism are not isolated. | High for path, low for cause |
| Queue selection | Two `single_queue` runs leave the defects unchanged, making ordinary asynchronous compute/transfer queue selection unlikely to be the primary trigger. | Medium |
| Upload allocation | `no_upload_hvv` changes the allocation path, but its apparent improvement is inseparable from lower capture altitude. | Low/inconclusive |
| Streaming/residency/LOD | The cross-configuration altitude threshold and rectangular terrain pages make this the leading hypothesis class, but current logs contain no resource-level evidence. | Medium for relevance, low for mechanism |

No upstream patch is justified yet. This is an evidence-based stopping point,
not a claim that no fix is possible.

## Source-level investigation gate

Launch-option testing ended after E02 because two baseline runs, two
upload-path runs, and two single-queue runs all retained the core defect. E03
and E04 were not run and must not be described as unchanged. The investigation
has resumed at the development-build stage.

1. Install and run the unmodified local build once to confirm parity with E00.
2. Determine with focused API telemetry whether the title uses D3D12 reserved
   resources and tile mappings for the affected terrain.
3. If it does, correlate tile map/unmap operations, mips, queue submissions,
   and destruction for stable resource cookies. If it does not, instrument the
   game's texture atlas, mip-range SRVs, upload copies, descriptors, and
   lifetime instead.
4. Use descriptor QA only after the resource path is known, or earlier if the
   focused trace reports suspicious descriptor reuse/destruction.
5. Investigate the NUMA caller separately with focused API tracing.
