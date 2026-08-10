# E02: single queue

- Configuration: `VKD3D_CONFIG=single_queue`
- Run 1: `E02-single-queue-r1`
- Run 2: `E02-single-queue-r2`
- Classification: **unchanged** across both runs

## Run 1 result

The user reports no change in either the main-menu artifacts or terrain in
both runs. The
flight screenshot still shows dark/absent ground, isolated rectangular terrain
pages, vegetation without the underlying surface, and magenta page edges.

Local capture, retained by hash but not published:
`E02-r1-terrain-unchanged-missing-pages-magenta-seams.png`,
SHA-256
`ca3ac0a873bd971cd601c8418b30fd1b2b1700356893605cba6e8c0514b7a1f1`.

Run-2 captures:

- `E02-r2-menu-aircraft-artifacts-unchanged.png` (local evidence, not published),
  SHA-256 `9853f57985ef0f18e69612b9ee074996841c9c77e9648d392d8cdafcd309fe8d`.
- `E02-r2-terrain-unchanged-missing-pages-magenta-seams.png` (local evidence, not published),
  SHA-256 `1fb07820ca53abf572a00a8a1e58c91a57ffa7dd2e7c315ab387a924dfa0a6cf`.

## Control verification

- `VKD3D_CONFIG='single_queue'` is logged.
- Host-visible device-local uploads remain enabled, matching E00 rather than
  E01.
- Descriptor buffers remain enabled.
- Three logical `vkd3d_queue` worker threads still exist. This does not
  invalidate the control: VKD3D documents `single_queue` as disabling
  asynchronous compute and transfer queues, rather than collapsing the game's
  logical D3D12 queues into one CPU worker.
- Logged staggered submissions in both runs use queue family 0.
- No D3D11 module, device loss, GPU hang, or out-of-memory error appears.

The unchanged result lowers the probability that ordinary asynchronous
compute/transfer queue selection alone causes the defect. It does not rule out
missing synchronization within one queue or resource visibility/lifetime bugs.

Generated comparison:
`captures/comparisons/E00-r2-vs-E02-r1.md`.

Repeatability comparison:
`captures/comparisons/E02-r1-vs-r2.md`.
