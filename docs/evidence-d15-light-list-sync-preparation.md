# D15 final light-list synchronization trace: preparation record

## Purpose

D14 identifies two final light-list stages that D13's four-dispatch budget did
not capture: `ComputeLightsFirstRef` and `ComputeLightsIndices`. The latter
writes a separate uint light-index buffer that the correlated pixel shaders
consume together with the 3D `R32_UINT` tile grid.

D15 passively records the four dispatches after `ComputeLightsCount` and the
next sixteen legacy resource barriers of every resource. This resolves whether
the separate buffer receives a transition or UAV dependency before it is read.
It does not insert a barrier, replace a shader, or change rendering.

## Source and build identity

- VKD3D-Proton diagnostic commit: `9c6a4338a2eff9f`
- Parent D13 telemetry commit: `395d974767f56f1`
- Terrain-fix base: `cf11ba76a1cdbee`
- Wine base: exact MR !11604 D10 package
- Custom Proton tool: `IL2-Korea-D15-LightListSync-9c6a4338`
- Trace gate: `VKD3D_IL2_LIGHT_TRACE=1`
- OpenMP/topology overrides: none

The build is gated and inert for other games unless its private diagnostic
environment variable is explicitly enabled. Both architectures were compiled
from the correct `src/vkd3d-proton-menu-trace` worktree and embed build identity
`0x9c6a4338a2eff9f`.

| Packaged file | SHA-256 |
|---|---|
| x64 `d3d12.dll` | `1642a053e5d9dbbf17adef2c383303a11062479f65e467bfff75ff1916adf369` |
| x64 `d3d12core.dll` | `85978ac3831d90902bf8ea72c85e8c3072bcbba54fa4034f5d643780ce21569b` |
| x86 `d3d12.dll` | `471d9050a29f66154ffdf75f1ec2d07c9f03b0307da08216d17c67091ed8e4fc` |
| x86 `d3d12core.dll` | `52f3b9118fbf3007dfa4bef468bed43fbcbe3003863bbf1d117772b08650a2a2` |

The installed custom-tool hashes match the build outputs exactly. The source
D13 tool and the game prefix were not modified.

## Prepared run

- Run ID: `D15-light-list-sync-r1`
- Launch options:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D15-light-list-sync-r1 VKD3D_IL2_LIGHT_TRACE=1 %command%
```

Protocol:

1. Fully exit Steam before selecting/installing a new compatibility tool.
2. Start Steam and select `IL2-Korea-D15-LightListSync-9c6a4338` for AppID
   247970.
3. Apply the exact launch options above.
4. Start the game, leave the affected menu aircraft visible for about ten
   seconds, and note whether the squares remain.
5. Exit the game normally. No mission or screenshot is needed for this passive
   trace.

## Decision rule

- If the separate light-index buffer has no dependency after the final writer,
  build one gated, shader-specific forced-barrier A/B.
- If a sufficient buffer dependency is present, do not add a barrier quirk;
  move to descriptor/view resolution or value capture.
- Visual persistence is expected and does not invalidate the trace.

Nothing from D15 is an application override or a proposed upstream fix. The
diagnostic commit remains local until the user separately approves publication.
