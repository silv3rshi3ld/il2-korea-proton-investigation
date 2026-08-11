# E00 controlled baseline evidence

- Run IDs: `E00-baseline-r1`, `E00-baseline-r2`
- Date: 2026-08-06
- Classification: **unchanged** across two controlled runs; repeatable baseline

## Visual result

The user reports that the menu aircraft renders but has visible block artifacts.
This temporal corruption is not reliably represented by a single still frame.
In flight, the captured failure is severe and closely matches VKD3D-Proton
issue #3134:

- most terrain surface is dark or absent;
- isolated rectangular texture pages remain visible;
- bright magenta/purple pixels occur along some page edges;
- trees or vegetation remain visible where the underlying terrain texture is
  absent;
- cockpit, aircraft, HUD, clouds, and the map overlay continue rendering;
- the failure persists across cockpit and external cameras and different
  altitudes/distances.

The user has observed the same behavior in earlier game runs. Both controlled
runs reproduce it, so this is accepted as the baseline appearance rather than
treated as a first-launch or cold-cache anomaly.

## Screenshot provenance

Curated copies are stored under `captures/curated/e00-baseline/` with
descriptive names. Three reviewed selections are published and linked below;
the other three remain ignored local evidence. Each image is 2560x1080.

| Run | Curated file | SHA-256 | Evidence |
|---|---|---|---|
| r1 | [`E00-r1-menu-aircraft-block-artifacts.png`](../captures/curated/e00-baseline/E00-r1-menu-aircraft-block-artifacts.png) | `4d752c078160bc705f540c0fd1fa960d0a965845a64ff85547334742667a85ef` | Menu aircraft; user observed dynamic block artifacts |
| r1 | [`E00-r1-terrain-cockpit-missing-pages.png`](../captures/curated/e00-baseline/E00-r1-terrain-cockpit-missing-pages.png) | `02588ae5ac934f23e75dcd8bcfaa10153cd2656ade55c187aa81f6b17b9fa899` | Cockpit view; dark terrain, isolated pages, magenta edges |
| r1 | `E00-r1-terrain-external-missing-pages.png` (local evidence, not published) | `508f098869c40ceeb9bcdf0d2357f39b14bc6ed5ffc40ae2e0b47397d0b08bfc` | External view; tiled terrain loss around aircraft |
| r1 | `E00-r1-terrain-external-wide-missing-pages.png` (local evidence, not published) | `16cdb8a9b35a8eaa3d8a3077e7a86a968d50613dbd29cc3d9895dab799c3f7d9` | Paused external view; widespread page pattern at distance |
| r2 | `E00-r2-menu-aircraft-block-artifacts.png` (local evidence, not published) | `c608752d5d3583ca86d0ee8d7d5fb89f701cfcd9179651d0a2999ad9c0541249` | Second menu reproduction; user confirms blocks persist |
| r2 | [`E00-r2-terrain-external-missing-pages-magenta-seams.png`](../captures/curated/e00-baseline/E00-r2-terrain-external-missing-pages-magenta-seams.png) | `d613c4f044d7a9dccef12cba9992aa7210909f86c7feca82ae5459cb07badacd` | Second terrain reproduction; most ground missing, bright magenta borders |

## Runtime path and controls

- Exact command target: `bin/game/IL2Series.exe`
- `VKD3D_CONFIG=''`
- VKD3D-Proton build: `3dfc6f07d0953b1`
- Program hash: `666cfc6e76a2547b`
- Loaded: game `dxBackend12.dll`, native prefix `dxgi.dll`, `d3d12.dll`,
  and `d3d12core.dll`
- Not loaded: `d3d11.dll`
- RADV device: Radeon RX 7800 XT, Mesa 26.1.6
- Host-visible device-local upload heaps: enabled
- ReBAR budgeting: applied to memory-type mask `0x218`
- `VK_EXT_descriptor_buffer`: enabled
- Three `vkd3d_queue` worker threads were created. VKD3D also reported that
  out-of-band work for queue families 0, 1, and 5 would occur on in-band
  queues. The exact logical role of each queue is not printed by this log.

## Log comparison

| Signal | r1 | r2 |
|---|---:|---:|
| Raw log size | 23,732,915 bytes | 9,151,730 bytes |
| Compressed log SHA-256 | `4da51f651a522fc0faeb189d19727fd2bdb50df161ff055e19bc935f53b2639b` | `0964fb81963a3eeb6161ebe1aa4e7cafc56552c7952df832a8770f0c1bbccd7f` |
| Split `END_ONLY` warnings | 106,384 | 18,562 |
| Split-warning interval | 390.199 s | 116.506 s |
| D3D11 module lines | 0 | 0 |
| Vulkan device loss / GPU hang | 0 | 0 |
| Vulkan/host/device out-of-memory | 0 | 0 |
| Ignored `c0000005` user-callback exceptions | 19 | 9 |

The raw counts are not severity measurements because r1 spent substantially
longer in the affected workload. The path/configuration signals agree: both
runs use the same VKD3D build, host-visible upload path, descriptor-buffer path,
ReBAR budget, and queue setup. The generated comparison is retained at
`captures/comparisons/E00-baseline-r1-vs-r2.md`.

The game created two threads named `BlocksCache` during r1,
but the normal Proton log contains no resource identities or calls that connect
those CPU threads to the missing GPU texture pages. The name is useful for
future filtered instrumentation, not a root-cause finding. The name was not
logged in r2, despite the same visible failure, further weakening it as a direct
diagnostic signal.

The Intel OpenMP module `libiomp5md.dll` loaded. With the affinity workaround
active, the log contains no `GetNumaNodeProcessorMaskEx` call; it does contain a
Wine `GetNumaHighestNodeNumber semi-stub` message. This belongs to the separate
startup track.
