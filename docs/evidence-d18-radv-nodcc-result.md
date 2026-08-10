# D18 RADV DCC-disable control: result

## Result

The tile-shaped artifact remains with RADV Delta Color Compression disabled.
The user reports that it may have looked more pronounced, while explicitly
noting uncertainty. Without a matched screenshot/video pair, the defensible
classification is **present; possible regression, magnitude inconclusive**.

`nodcc` is not a remedy and DCC cannot be the sole cause. The possible severity
change means D18 does not completely exclude compression/layout behavior from
influencing how already incorrect data appears.

## Run identity

- Run: `D18-radv-nodcc-r1`
- Game build: `24615759`
- Proton: `1786112352 experimental-11.0-20260724c-wine-mr11604-d10`
- VKD3D-Proton: `274f6f8e2d5b785`
- GPU/driver: Radeon RX 7800 XT, RADV 26.1.6
- Kernel: `7.1.6-1-cachyos`
- Resolution: 2560x1080
- Launch options:
  `PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D18-radv-nodcc-r1 RADV_DEBUG=startup,nodcc VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%`
- Source log: 11,189,185 bytes
- No OpenMP/topology override
- D3D12 and DXGI loaded; no D3D11 module loaded
- No device-loss, OOM, GPU-reset, page-fault, or hang signature

The full run remains ignored and local. No D18 screenshot was supplied.

## Runtime validation

RADV emits the instance/device discovery messages selected by `startup` in the
same `RADV_DEBUG` value, with no unknown-option warning. The expected D16
VKD3D build and descriptor trace both load.

The target events cover approximately 39.6 seconds, from Wine monotonic
timestamp `3870.418` through `3910.032`.

| Pixel shader | Register | Resolved | Mapping |
|---|---:|---:|---|
| `df0bd777fd1bb89d` | `t9` | 1,636 | cookie 4002, Texture3D `80x34x2 R32_UINT` |
| `df0bd777fd1bb89d` | `t10` | 1,636 | cookie 4001, Buffer `R16_UINT`, 43,520 elements |
| `a2d104d5c813322e` | `t9` | 1,636 | cookie 4002, Texture3D `80x34x2 R32_UINT` |
| `a2d104d5c813322e` | `t10` | 1,636 | cookie 4001, Buffer `R16_UINT`, 43,520 elements |

All 6,544 lookups resolve successfully. Root signature, table base, heap
offsets, descriptor serials, resources, formats, shapes, and ranges exactly
match D16 and D17.

## Decision

Do not propose `RADV_DEBUG=nodcc`, a global compression disable, or an IL-2
profile. The artifact survives without DCC.

The uncertain severity observation is retained rather than promoted to a
causal finding. A matched repeated A/B can be performed later if value capture
points back to image metadata, but repeating it now would not distinguish bad
producer values from a rendering-layout influence.

The next cheap discriminator is `ACO_DEBUG=force-waitcnt`, with DCC restored
and the D16 descriptor control retained. It forces conservative wait states
inside compiled shaders and tests an intra-shader scheduling/hazard failure
that D17's waits between draws/dispatches do not cover.
