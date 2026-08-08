# D19 ACO forced-wait control: preparation record

## Purpose

D17 excludes ordinary synchronization/cache visibility between draws and
dispatches. D18 shows that disabling DCC does not remove the artifact. Neither
control changes instruction scheduling or wait-state emission inside an
individual compute or pixel shader.

D19 uses Mesa's documented `ACO_DEBUG=force-waitcnt`, which forces wait states
whenever ACO has an outstanding operation to wait for. This is a focused test
for an ACO scheduling/hazard error in the tiled-light producers or consumers.
DCC and RADV full synchronization return to their defaults. The D16 descriptor
trace remains enabled as the stable control.

This is a diagnostic compiler option, not a launch workaround or proposed fix.

## Controlled comparison

- Game build: `24615759`
- Custom Proton tool: `IL2-Korea-D16-DescriptorTrace-274f6f8e`
- VKD3D-Proton: `274f6f8e2d5b785`
- Added behavior: `ACO_DEBUG=force-waitcnt`
- Logging-only RADV token: `RADV_DEBUG=startup`
- D16 descriptor gate retained: `VKD3D_IL2_DESCRIPTOR_TRACE=1`
- `fullsync` absent; DCC restored to default
- OpenMP/topology overrides: none

## Prepared run

- Run ID: `D19-aco-force-waitcnt-r1`
- Launch options:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D19-aco-force-waitcnt-r1 RADV_DEBUG=startup ACO_DEBUG=force-waitcnt VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%
```

Protocol:

1. Keep `IL2-Korea-D16-DescriptorTrace-274f6f8e` selected.
2. Replace the D18 launch options with the exact D19 line above.
3. Start the game and observe the affected main-menu aircraft for about ten
   seconds. Do not change graphics settings.
4. Exit normally and classify the squares as unchanged, improved, or fixed.

## Decision rule

- Fixed/improved selects ACO wait-state/code generation for a shader-specific
  reproducer and Mesa investigation. Do not ship the global debug option.
- Unchanged closes the last cheap driver-hazard control and selects produced
  tiled-grid/index values or a focused shader/compiler bisection.
- Startup/device failure makes the run inconclusive.

Nothing from D19 will be uploaded without separate user confirmation.
