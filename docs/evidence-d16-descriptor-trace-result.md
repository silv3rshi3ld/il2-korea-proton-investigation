# D16 tiled-light descriptor resolution: result

## Result

D16 is visually unchanged and resolves every covered `t9` and `t10` lookup.
Both affected pixel shaders consistently receive the same resources written by
the final tiled-light compute sequence:

- `t9` is an SRV of `rtLightRefs25` (cookie 4002), an `80x34x2 R32_UINT`
  three-dimensional texture; and
- `t10` is an SRV of cookie 4001, an 87,040-byte buffer viewed as 43,520
  `R16_UINT` elements.

There are no lookup failures, type changes, resource-cookie changes, view-shape
changes, or table-location changes in 13,236 resolved events. Wrong descriptor
selection, propagation, type, or view shape for these two inputs is therefore
excluded for the covered draws. D16 does not validate the values stored in the
resources or prove that the translated Vulkan dependencies flush every relevant
cache correctly.

## Run identity

- Run: `D16-descriptor-trace-r1`
- Game build: `24615759`
- Proton: `1786112352 experimental-11.0-20260724c-wine-mr11604-d10`
- VKD3D-Proton: `274f6f8e2d5b785`
- GPU/driver: Radeon RX 7800 XT, RADV 26.1.6
- Kernel: `7.1.6-1-cachyos`
- Resolution: 2560x1080
- Launch options:
  `PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D16-descriptor-trace-r1 VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%`
- Source log: 18,996,159 bytes
- Compressed log SHA-256:
  `22c9cb1daf928d09d7ac5e52966e71cebf02e4797812b2833ffd165fc54f8f0d`
- No OpenMP/topology override
- D3D12 and DXGI loaded; no D3D11 module loaded
- No device-loss, OOM, GPU-reset, or hang signature

The full run remains ignored and local.

## Visual classification and setting controls

The user confirmed that the square/shimmering artifact remained visible. During
the same run, the user also disabled anti-aliasing and HDR lighting in the game
UI and observed no change.

That setting comparison is useful but is retained as a reported visual control,
not a configuration-verified exclusion. The `startup.cfg` written at exit still
contains `hdr_enable = 1`, `modeaa = 3`, `msaa = 0`, and `multisampling = 2`.
Those saved values do not prove that the requested UI states remained active.
The descriptor conclusion does not depend on the setting comparison.

## Trace coverage

The target draws span approximately 80.3 seconds, from Wine monotonic timestamp
`1849.721` through `1930.058`.

| Pixel shader | Register | Resolved | Failures |
|---|---:|---:|---:|
| `df0bd777fd1bb89d` (`PixOutLight_msp`) | `t9` | 3,309 | 0 |
| `df0bd777fd1bb89d` (`PixOutLight_msp`) | `t10` | 3,309 | 0 |
| `a2d104d5c813322e` (`PixOutLight_mss`) | `t9` | 3,309 | 0 |
| `a2d104d5c813322e` (`PixOutLight_mss`) | `t10` | 3,309 | 0 |

All events use root signature `ab56fdc47663fc9a`, root parameter 0, table base
13,788, and the same 300,000-entry shader-visible heap. `t9` resolves to heap
offset 13,797 with sidecar serial 1,640; `t10` resolves to offset 13,798 with
serial 1,638. No trace event reached the 20,000-event cap.

## Exact resource and view mapping

| Register | Type | Resource | Resource shape | View | Range |
|---|---|---:|---|---|---|
| `t9` | SRV | cookie 4002, `rtLightRefs25` | `80x34x2`, `R32_UINT` | Texture3D, `R32_UINT` | complete one-mip view |
| `t10` | SRV | cookie 4001 | 87,040-byte buffer | Buffer, `R16_UINT` | first 0, 43,520 elements |

D15 records both cookie 4002 and cookie 4001 receiving two explicit UAV
dependencies and a final transition to shader-read state in each covered final
producer cycle. D16 now proves that the correlated consumers read those exact
resources.

The `R16_UINT` view corrects the earlier size-only interpretation of cookie
4001. Its storage is:

```text
80 * 34 tiles * 16 indices per tile * sizeof(uint16_t) = 87,040 bytes
```

The application shader declares a typed uint buffer. This is compatible with an
`R16_UINT` view: a typed fetch returns the 16-bit element zero-extended to the
shader's 32-bit uint. The translated consumer uses an integer texel-buffer
`OpImageFetch`; the final producer uses an integer storage texel-buffer
`OpImageWrite`. Both modules pass `spirv-val`. D16 therefore exposes no
type/width mismatch by itself.

The run names later `rtLightRefs46`, `rtLightRefs71`, and `rtLightRefs96`
resources after settings activity, while these target draws retain cookie 4002.
That is not evidence that the original reproduction used a stale descriptor:
the artifact and correct cookie-4002 producer/consumer sequence already occur
before those later contexts are created. A combined use/descriptor trace would
be required before assigning meaning to the later names.

## Current-upstream check

After a read-only fetch, VKD3D-Proton `origin/master` remains
`84c87c8390d9df75ba41d911496296fe13f0e275`, the same unmodified baseline tested
in D04. `dxil-spirv` has four commits after the embedded `cc75a0c9`, but all four
only change NVIDIA/SM 6.9 ray-tracing local-root-table constant loads. They do
not touch typed buffers, the affected pixel/compute path, or general control
flow. A blind current-upstream rebuild is not a new discriminator.

## Decision

D16 closes incorrect `t9`/`t10` descriptor selection, propagation, type, and
shape for the covered draws. Do not add a descriptor workaround or game/AppID
override from this result.

Before invasive GPU value capture, the next one-variable control should use
RADV's documented `fullsync` debug mode. It waits after every draw/dispatch and
flushes all caches. A visual change would select translated Vulkan
synchronization/cache handling for inspection despite the correct D3D12
barriers; an unchanged result would move the investigation to produced values,
typed-buffer code generation, or another tiled input. This is a diagnostic
launch option, not a proposed user-facing solution.
