# Environment baseline

Collected locally on 2026-08-06. Static component inspection and the six
completed runtime logs are distinguished where relevant.

## Host

| Item | Value |
|---|---|
| Distribution | CachyOS (rolling) |
| Kernel | `7.1.6-1-cachyos` |
| Session | Wayland (`XDG_SESSION_TYPE=wayland`, `WAYLAND_DISPLAY=wayland-0`) |
| CPU | AMD Ryzen 7 7800X3D, 8 cores / 16 threads |
| Linux NUMA topology | One node, CPUs 0-15, distance 10 |
| GPU | AMD Radeon RX 7800 XT, Navi 32, PCI ID `1002:747e` |
| Kernel driver | `amdgpu` |
| Mesa packages | `mesa`, `lib32-mesa`, `vulkan-radeon`, `lib32-vulkan-radeon` all `3:26.1.6-1` |
| Vulkan driver | RADV, `Mesa 26.1.6-arch3.1`, Vulkan API 1.4.354 |
| Vulkan instance | 1.4.357 |

## Resizable BAR

`lspci -vv` reports a 16 GiB prefetchable BAR at PCI `0000:03:00.0`, plus the
legacy 256 MiB BAR. This is strong evidence that Resizable BAR is enabled for
the RX 7800 XT. Capability decoding was access-restricted during collection,
so the mapped 16 GiB aperture—not a decoded PCI capability flag—is the recorded
evidence.

## Vulkan capabilities relevant to this investigation

RADV advertises at least:

- `VK_EXT_descriptor_buffer`
- `VK_EXT_graphics_pipeline_library`
- `VK_EXT_image_compression_control`
- `VK_EXT_memory_budget`
- `VK_EXT_memory_priority`
- `VK_KHR_buffer_device_address`
- `VK_KHR_present_id` and `VK_KHR_present_wait`
- `VK_KHR_maintenance1` through `VK_KHR_maintenance10` as applicable

Sparse binding, sparse buffers, sparse 2D/3D images, aliased sparse residency,
standard 2D/3D block shapes, and strict non-resident behavior are advertised.
Sparse multisample residency is not advertised.

Available queue families on the discrete GPU include:

- one graphics + compute + transfer + sparse-binding queue;
- four compute + transfer + sparse-binding queues;
- a sparse-binding-only queue (plus separate video queues).

E00 created three `vkd3d_queue` worker threads and emitted out-of-band fallback
messages for queue families 0, 1, and 5. E02 confirms `single_queue` disables
asynchronous compute/transfer queue use, with logged staggered submissions on
queue family 0. Both E02 runs are visually unchanged.

## Steam, game, and prefix

| Item | Value |
|---|---|
| Steam AppID | `247970` |
| Steam library | `/home/silv3rshi3ld/.local/share/Steam` |
| Manifest | `steamapps/appmanifest_247970.acf` |
| Install directory | `steamapps/common/IL2Series` |
| Build ID | `24596901` (auto-updated 2026-08-06; previous baseline `24577563`) |
| Executable | `bin/game/IL2Series.exe` (PE32+, x86-64) |
| Prefix | `steamapps/compatdata/247970` |
| Prefix version | `11.0-100` |

Steam's launch record confirms the command target is
`bin/game/IL2Series.exe`. Static imports establish this chain:

```text
IL2Series.exe
  -> dxBackend12.dll (game DLL)
       -> d3d12.dll
       -> dxgi.dll
```

The game directory has no local `d3d12.dll`, `d3d12core.dll`, `dxgi.dll`, or
`d3d11.dll`, so resolution falls to the prefix/Proton DLLs.

## Selected Proton components

| Component | Version / commit |
|---|---|
| Proton Experimental | `experimental-11.0-20260724c` |
| VKD3D-Proton | `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5` (`vkd3d-1.1-5438-g3dfc6f07`) |
| DXVK | `1a5919b7edd111887648d1e8bf0c32733e2e00d3` (`v3.0.2-5-g1a5919b7`) |
| Wine VKD3D library | `b342273b68c86c8ac460c31c6c4c3ce8e3766db3` |

Prefix hashes match the selected Proton payload:

| DLL | Provider | SHA-256 |
|---|---|---|
| `system32/d3d12.dll` | VKD3D-Proton | `c383f4c513aa12f9d93177c798e67f70a73758ab1d4537ecd3c47119b6e51cd2` |
| `system32/d3d12core.dll` | VKD3D-Proton | `a9fe0f0c9741c1fd3c152883c66f2aa941eacf456f7ebfd3f6e7a61d22e7b661` |
| `system32/dxgi.dll` | DXVK | `f73b1401547a9f93fa84eff413c7678f7e961a4ef317de45f48965d904f6c180` |
| `system32/d3d11.dll` | DXVK | `98925b21f5a9b81f456ac85098d902e4cbd7065ecca282f23289f35bdac510e2` |

DXVK's DXGI DLL is part of Proton's normal D3D12 path and does not by itself
show that the game renders through D3D11. E00-r1 confirms DXGI, D3D12,
D3D12Core, and the game backend loaded; no D3D11 module line was present.

## Evidence still required for stronger attribution

- BIOS-side ReBAR screen or unrestricted PCI capability decode if upstream
  needs evidence beyond the mapped 16 GiB aperture.
- Exact in-game graphics settings, resolution, mission identifier, and
  deterministic camera positions for a future pixel-matched comparison batch.
- Resource-level terrain telemetry; ordinary Proton logs do not expose mips,
  tile mappings, residency, or the resource affected by split barriers.
