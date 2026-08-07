# D06 focused baked-cache trace: installed and prepared

## Purpose

D05c corrected every encountered border reinterpret copy without changing the
image. D06 therefore changes no D3D12 or Vulkan behavior. It records only the
2048x2048, one-mip texture class containing the game's baked-terrain cache and
nearby same-sized intermediates.

The trace records:

- committed/placed creation, format, flags, state, layout, and allocation;
- SRV creation and view parameters;
- `CopyTextureRegion`/`CopyResource` operations involving the class;
- legacy and enhanced barrier states, stages, access masks, and layouts;
- resource destruction;
- bounded per-event suppression markers.

The filter does not assume every 2048x2048 texture is terrain. Post-processing
groups resources by format and lifecycle. The diagnostic does not yet claim to
identify the exact draw or shader which samples a descriptor.

## Identity

- Upstream base: `84c87c8390d9df75ba41d911496296fe13f0e275`
- Diagnostic commit: `376652dc46fc323d2d2eb59ae8bd0ebd6cf3d189`
- Retained patch:
  `patches/0007-vkd3d-Add-focused-IL-2-baked-cache-telemetry.patch`
- Build: `build/vkd3d-proton-il2-d06-cache-376652dc/`
- Source Proton: Experimental `experimental-11.0-20260724c`
- Installed tool: `IL2-Korea-D06-CacheTrace-376652dc`

| File | SHA-256 |
|---|---|
| x64 `d3d12.dll` | `42f671a6a5d3961cb8abf3bd9e33ee0c846dc0e692921001bfe171fdc734ac1a` |
| x64 `d3d12core.dll` | `12635bf64ab734cb59b88130bf1d2e4f333b600ad1351183895818b786446b8c` |
| x86 `d3d12.dll` | `25f52c3884edb75c4dfae69c3c0e0557ffc88052cd8d063468b2c5b46f703566` |
| x86 `d3d12core.dll` | `7050a26a5ce54007b67562d8c91f08f696b2d9ef2299809d2bd831a44db5757f` |

Both architectures built successfully with the official development-package
method. Installed hashes match the build artifacts. Proton Experimental and
the game prefix were not modified; the prefix registry timestamps remained
unchanged after installation.

## Runtime test

Run ID: `D06-cache-trace-r1`

Select `IL2-Korea-D06-CacheTrace-376652dc` for AppID 247970 and use exactly:

```bash
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D06-cache-trace-r1 OMP_NUM_THREADS=16 KMP_AFFINITY=disabled VKD3D_IL2_BAKED_CACHE_TRACE=1 %command%
```

Use the same autumn free-flight scenario. Enter the mission, remain in flight
for about 60-90 seconds around 1,300-1,500 m while the broken terrain pages are
visible, then exit. A screenshot is optional because this is trace-only; note
any unexpected visual or stability change.

The Proton log is automatic. After exit, collect and analyze it with:

```bash
./scripts/collect-proton-log.sh collect D06-cache-trace-r1
./scripts/collect-game-logs.sh D06-cache-trace-r1
```

A valid run has one `IL2CACHE enabled:` marker, matching resource creates, and
no event suppression before the reproduced failure. The generated
`baked-cache-analysis.md` will identify BC3 pages with no incoming copy, SRVs
created before population, and lifecycle groups needing the next narrower
trace.

Rollback is selecting Proton Experimental again. Do not delete a compatibility
tool while Steam is running.

