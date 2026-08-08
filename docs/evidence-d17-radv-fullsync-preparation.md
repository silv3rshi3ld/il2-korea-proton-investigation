# D17 RADV full-cache synchronization: preparation record

## Purpose

D15 proves that the game submits the expected D3D12 UAV dependencies and final
shader-read transitions. D16 proves that the affected draws read the exact
producer resources through stable, correctly shaped `t9` and `t10` views. One
boundary remains between that evidence and GPU value capture: whether the
translated Vulkan synchronization and RADV cache handling make the writes
visible as intended.

D17 changes one rendering behavior relative to D16 by adding RADV's documented
`fullsync` debug option. Mesa defines it as synchronizing all pending work after
every draw/dispatch and flushing all caches. The harmless `startup` token makes
RADV emit startup information so the driver environment is observable in the
Proton log.

This is a broad diagnostic and will substantially reduce performance. It is
not a proposed launch option, game workaround, Proton fix, or Mesa fix.

## Controlled comparison

- Game build: `24615759`
- Custom Proton tool: `IL2-Korea-D16-DescriptorTrace-274f6f8e`
- VKD3D-Proton: `274f6f8e2d5b785`
- Wine: exact MR !11604 D10 package
- Baseline run: `D16-descriptor-trace-r1`
- D16 descriptor gate retained: `VKD3D_IL2_DESCRIPTOR_TRACE=1`
- Added behavior: `RADV_DEBUG=startup,fullsync`
- OpenMP/topology overrides: none

Keeping the descriptor gate enabled makes D17 directly comparable to D16. The
sidecar adds CPU overhead but does not alter GPU commands. No binary rebuild or
compatibility-tool change is needed.

## Prepared run

- Run ID: `D17-radv-fullsync-r1`
- Launch options:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D17-radv-fullsync-r1 RADV_DEBUG=startup,fullsync VKD3D_IL2_DESCRIPTOR_TRACE=1 %command%
```

Protocol:

1. Keep the D16 compatibility tool selected.
2. Replace Steam launch options with the exact line above.
3. Start the game and wait for the affected main-menu aircraft to render.
4. Observe the square/shimmering artifact for about ten seconds. Do not change
   graphics settings during this run.
5. Exit normally. A mission and screenshot are not required.

The menu may run much more slowly because the driver waits and flushes after
every draw and dispatch. Slowness does not classify the visual result.

## Decision rule

- If the squares disappear or materially improve, inspect the emitted Vulkan
  barriers/access masks and narrow the excessive driver synchronization to the
  exact producer/consumer boundary. Do not ship `fullsync`.
- If the squares are unchanged, translated cache visibility is strongly
  weakened and the next source-level step is produced-value capture or a
  focused typed-buffer/compiler A/B.
- If the menu cannot render because the debug mode is prohibitively slow, mark
  the run inconclusive; do not interpret startup alone.

Nothing from D17 will be uploaded without separate user confirmation.
