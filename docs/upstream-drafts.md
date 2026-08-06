# Upstream drafts (review only; do not post automatically)

These drafts describe the completed E00-E02 investigation accurately. They do
not claim a root cause or fix. Attach only selected, reviewed screenshots and
filtered logs; do not upload the game, prefix, credentials, or unfiltered large
artifacts.

## ValveSoftware/Proton issue #9906 update

```text
Controlled test update for Korea. IL-2 Series (247970)

Environment
- CachyOS, kernel 7.1.6-1-cachyos, Wayland
- Ryzen 7 7800X3D (8 cores/16 threads, one Linux NUMA node)
- Radeon RX 7800 XT; 16 GiB PCI BAR mapped
- Mesa/RADV 26.1.6 (Mesa 26.1.6-arch3.1)
- Proton Experimental experimental-11.0-20260724c; prefix version 11.0-100
- VKD3D-Proton 3dfc6f07d0953b1e8b41705275c2c59cc7374fc5
- DXVK 1a5919b7edd111887648d1e8bf0c32733e2e00d3
- Game build ID 24596901

Runtime path
bin/game/IL2Series.exe loads the game's dxBackend12.dll, Proton's
VKD3D-Proton d3d12.dll/d3d12core.dll, and DXVK dxgi.dll. No completed
controlled log with module evidence loads d3d11.dll.

Startup
The current mitigation is:
  OMP_NUM_THREADS=16 KMP_AFFINITY=disabled %command%

The shipped libiomp5md.dll loads in these runs. Wine logs
GetNumaHighestNodeNumber as a semi-stub under the mitigation, but these tests
did not capture the original GetNumaNodeProcessorMaskEx call or isolate which
environment variable is necessary. This remains a mitigation, not a NUMA fix.

Rendering reproduction
1. Start with the mitigation above.
2. Observe the rotating aircraft in the main menu: moving square/block
   corruption appears over the aircraft and effects.
3. Start the same flight and use an external aircraft camera.
4. The terrain is mostly dark/absent, with isolated rectangular texture pages
   and magenta edges. The loss becomes more severe with altitude. Below roughly
   1,500 m, additional low-fidelity vegetation/terrain assets begin to appear;
   near 5,000 m, most ground is absent.

Controlled results (two runs each)
- E00, no VKD3D diagnostic option: reproducible defect in both runs.
- E01, VKD3D_CONFIG=no_upload_hvv: the upload allocation path changed as
  expected. More vegetation was visible, but both screenshots were near
  1,350-1,500 m; the result is altitude-confounded and inconclusive.
- E02, VKD3D_CONFIG=single_queue: menu and terrain defects unchanged in both
  runs. Ordinary async compute/transfer queue selection is unlikely to be the
  primary trigger.
- E03, descriptor buffer disabled: not run.
- E04, no_upload_hvv plus single_queue: not run.
- D01b, dedicated custom Proton with gated resource telemetry: the trace build
  is verified active and records zero CreateReservedResource,
  GetResourceTiling, UpdateTileMappings, or CopyTileMappings calls while the
  corruption is visible. D3D12 sparse/tiled resources are excluded from this
  failing path.

No run reported device loss, GPU hang/reset, or out-of-memory errors. Repeated
split END_ONLY barrier warnings remain uncorrelated with an affected resource
and are not treated as causal.

Assessment
The altitude dependence and rectangular pages make ordinary texture streaming,
uploads, descriptor/mip selection, or LOD behavior the leading hypothesis
class. D01b excludes API-level sparse residency, but current completed output
contains no affected ordinary-texture IDs, mip ranges, or altitude, so the
defective layer (game, VKD3D-Proton, or RADV) is not isolated. A gated D02
ordinary-texture trace is prepared; no override or behavior-changing source
patch is proposed.

Attachments proposed after review
- environment.md
- experiment-matrix.md
- findings.md
- one E00 low-altitude and one E00 high-altitude screenshot
- one E02 high-altitude screenshot
- filtered log comparison and checksums
```

## VKD3D-Proton issue #3134 update

Use this as a comment on the existing issue rather than opening a duplicate.

```text
I reproduced the issue on a separate AMD generation and completed six
controlled runs.

Environment: RX 7800 XT (Navi 32), Mesa/RADV 26.1.6, Proton Experimental
experimental-11.0-20260724c, VKD3D-Proton commit
3dfc6f07d0953b1e8b41705275c2c59cc7374fc5, game build 24596901.

The runtime path is IL2Series.exe -> dxBackend12.dll -> VKD3D-Proton
D3D12/D3D12Core plus DXVK DXGI. d3d11.dll is not loaded in any controlled run.
Startup still requires OMP_NUM_THREADS=16 KMP_AFFINITY=disabled on this host.

Results, two runs per completed configuration:
- Baseline: repeated menu block artifacts and severe rectangular terrain-page
  loss with magenta edges.
- VKD3D_CONFIG=no_upload_hvv: allocation-path change confirmed, but visual
  comparison is inconclusive because both captures were at lower altitude.
- VKD3D_CONFIG=single_queue: unchanged twice.
- Descriptor-buffer disable and the combined option were not run.
- A valid dedicated-custom-Proton trace recorded zero D3D12 reserved-resource,
  tiling-query, tile-update, or tile-copy calls during reproduction. This
  excludes API-level sparse/tiled resources from the failing path.

There is a strong altitude/distance dependency across configurations. The user
observes some low-fidelity assets loading below roughly 1,500 m, while captures
near 5,000 m show almost entirely absent ground. Ordinary PROTON_LOG output
does not expose altitude, affected ordinary-texture IDs, or mip ranges, so this
does not yet distinguish upload, descriptor/mip, barrier, application, or
driver behavior.

No device loss, GPU reset/hang, or OOM was found. Split END_ONLY barrier
warnings are frequent but have no resource correlation, and the current
VKD3D-Proton code already handles END_ONLY as a conservative full transition.

Current conclusion: no game override or general patch is justified. The next
prepared step is bounded telemetry for ordinary texture creation/lifetime,
normalized SRV mip ranges, and upload copies by stable resource cookie. That
should select a resource class before any barrier, descriptor, or driver
instrumentation is broadened.
```

## Wine or Proton startup report outline

Do not file this yet. The original failing call, caller, arguments, return
value, and Windows comparison have not been captured. A useful report needs:

- a clean failure log without the OpenMP mitigation;
- module attribution for `GetNumaNodeProcessorMaskEx`;
- the returned Wine processor-group/NUMA topology;
- isolated tests of `KMP_AFFINITY=disabled` and `OMP_NUM_THREADS=16`;
- comparison with the same `libiomp5md.dll` behavior on Windows.

## Mesa/RADV report gate

No Mesa report is justified from the current evidence. Prepare one only if a
minimal Vulkan reproduction or driver comparison shows that VKD3D-Proton emits
valid Vulkan and RADV produces the wrong result. Include exact Mesa commit,
kernel and firmware, validation classification, trace/reproducer checksum,
alternative-driver result, and good/bad commit range where available.

## Pull-request gate

There is no PR draft because there is no candidate change. A future PR must
name the isolated mechanism, explain D3D12 versus Vulkan semantics, include a
focused regression test where practical, quantify performance impact, and
limit any `IL2Series.exe` override specifically to Steam AppID 247970.
