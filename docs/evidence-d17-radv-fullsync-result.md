# D17 RADV full-cache synchronization: result

## Result

D17 is visually unchanged. The same tile-shaped translucent and dark squares
remain across the aircraft, floor, reflections, and shadows while RADV waits
after every draw/dispatch and flushes all caches.

This strongly excludes ordinary translated cache visibility or incomplete
inter-draw/dispatch synchronization as the cause. It does not disable DCC image
compression, change shader compilation, or validate the values generated inside
one compute dispatch.

## Run identity

- Run: `D17-radv-fullsync-r1`
- Game build: `24615759`
- Proton: `1786112352 experimental-11.0-20260724c-wine-mr11604-d10`
- VKD3D-Proton: `274f6f8e2d5b785`
- GPU/driver: Radeon RX 7800 XT, RADV 26.1.6
- Kernel: `7.1.6-1-cachyos`
- Resolution: 2560x1080
- Launch options:
  `PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D17-radv-fullsync-r1 RADV_DEBUG=startup,fullsync VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%`
- Source log: 11,172,605 bytes
- No OpenMP/topology override
- D3D12 and DXGI loaded; no D3D11 module loaded
- No device-loss, OOM, GPU-reset, page-fault, or hang signature

The full run and screenshot remain ignored and local.

## Fullsync validation

RADV emits repeated `radv: info: Created an instance` and device-discovery
messages that are selected by the `startup` token in the same `RADV_DEBUG`
value. There is no unknown-option warning. The screenshot reports approximately
10 FPS, compared with the normal 60 FPS cap, which is consistent with the
documented full synchronization behavior.

The screenshot visibly captures the unchanged defect:

| File | Dimensions | SHA-256 | Observation |
|---|---:|---|---|
| `D17-r1-fullsync-shimmering-unchanged.png` | 2560x1080 | `cb8ab4d896e526d554f7033ba52c86bd638f3e3fc3dfa1744b2403c249fd472a` | approximately 32-pixel grid remains across lit/reflective aircraft and floor regions; overlay shows 10 FPS |

## Descriptor control retained

The target events cover approximately 42.3 seconds, from Wine monotonic
timestamp `2788.015` through `2830.339`.

| Pixel shader | Register | Resolved | Mapping |
|---|---:|---:|---|
| `df0bd777fd1bb89d` | `t9` | 2,076 | cookie 4002, Texture3D `80x34x2 R32_UINT` |
| `df0bd777fd1bb89d` | `t10` | 2,076 | cookie 4001, Buffer `R16_UINT`, 43,520 elements |
| `a2d104d5c813322e` | `t9` | 2,076 | cookie 4002, Texture3D `80x34x2 R32_UINT` |
| `a2d104d5c813322e` | `t10` | 2,076 | cookie 4001, Buffer `R16_UINT`, 43,520 elements |

All 8,304 lookups resolve successfully. Root signature, table base, heap
offsets, descriptor serials, resource cookies, formats, shapes, and buffer
ranges exactly match D16. No event reaches the trace cap.

## Decision

Do not add a synchronization barrier, global serialization workaround,
`RADV_DEBUG=fullsync`, or IL-2 application profile. D17 makes the artifact much
slower but does not change its appearance.

The strongest remaining subpaths are:

1. corruption or interpretation of values produced inside the tiled-light
   compute sequence;
2. an ACO/RDNA code-generation issue inside a producer or consumer shader; or
3. image-compression metadata behavior on the frequently switched
   `80x34x2 R32_UINT` storage/sampled image.

The next cheap one-variable discriminator is `RADV_DEBUG=nodcc` on the same D16
tool and descriptor gate. If unchanged, use `ACO_DEBUG=force-waitcnt` before
building invasive value-capture instrumentation. These are diagnostic controls,
not proposed user-facing launch options.
