# D18 RADV DCC-disable control: preparation record

## Purpose

D17 leaves the tiled square artifact unchanged even when RADV synchronizes
after every draw/dispatch and flushes all caches. `fullsync` does not disable
Delta Color Compression or its image metadata. The principal tiled-light image
is a frequently switched `80x34x2 R32_UINT` storage/sampled resource, so D18
isolates image compression before moving to shader-compiler or GPU-value
instrumentation.

Mesa documents `RADV_DEBUG=nodcc` as disabling DCC. The `startup` token remains
only to emit evidence that RADV parsed the debug environment. The same D16
descriptor trace remains enabled as a control.

This is a diagnostic driver option, not a proposed launch option, Proton
workaround, Mesa workaround, or fix.

## Controlled comparison

- Game build: `24615759`
- Custom Proton tool: `IL2-Korea-D16-DescriptorTrace-274f6f8e`
- VKD3D-Proton: `274f6f8e2d5b785`
- Baseline descriptor run: `D16-descriptor-trace-r1`
- Full-cache control: `D17-radv-fullsync-r1`
- D16 descriptor gate retained: `VKD3D_IL2_DESCRIPTOR_TRACE=1`
- Added behavior: `RADV_DEBUG=startup,nodcc`
- `fullsync` removed; D18 tests only DCC disable relative to normal rendering
- OpenMP/topology overrides: none

## Prepared run

- Run ID: `D18-radv-nodcc-r1`
- Launch options:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D18-radv-nodcc-r1 RADV_DEBUG=startup,nodcc VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%
```

Protocol:

1. Keep `IL2-Korea-D16-DescriptorTrace-274f6f8e` selected.
2. Replace the D17 launch options with the exact D18 line above.
3. Start the game and observe the affected main-menu aircraft for about ten
   seconds. Do not change graphics settings.
4. Exit normally and classify the squares as unchanged, improved, or fixed.

## Decision rule

- Fixed/improved selects DCC/image metadata for narrower inspection; never ship
  the global `nodcc` diagnostic.
- Unchanged strongly weakens compression metadata on the light-grid image and
  selects `ACO_DEBUG=force-waitcnt` as the last cheap compiler/hazard control.
- A startup/device failure makes the run inconclusive.
