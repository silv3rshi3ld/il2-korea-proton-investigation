# D47 correctly wired allocator-only control: preparation

## Why D47 exists

D46 was intended to test only the corrected tiled-light allocator. Source
review after its failed visual run found that the `IL2Series.exe` application
mapping had been removed together with the depth-gate quirk. The compiled
allocator quirk table was unused, so D46 actually tested no IL-2 lighting
quirk. Its blocks are not evidence against allocator-only sufficiency.

D47 adds back only this application mapping:

```c
{ VKD3D_STRING_COMPARE_EXACT, "IL2Series.exe", &il2_korea_quirks },
```

## Required source chain

D47 must contain all four of these elements:

1. executable match `IL2Series.exe`;
2. allocator shader hash `7cefa1bc80bb4c70`;
3. `VKD3D_SHADER_QUIRK_FORCE_TYPED_UAV_AS_SSBO` changing typed-UAV lowering;
4. `VKD3D_SHADER_BINDING_FLAG_RAW_SSBO` selecting the raw descriptor sibling.

It must contain neither
`VKD3D_SHADER_QUIRK_BYPASS_TILED_LIGHT_DEPTH_CULLING` nor producer hashes
`651194bd0a21772e` and `11e32439a86036ba`.

Local source identity:

- branch: `il2-d47-allocator-only-wired`;
- commit: `f3e06d0b`;
- package: `build/vkd3d-proton-il2-d47-allocator-only-wired-f3e06d0b`.

Both MinGW architectures build without the D46 unused-quirk warning. The x64
`d3d12core.dll` contains `IL2Series.exe`, and source inspection confirms all
four required links while finding neither depth quirk nor producer hash.

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `480df8b1e4b97212b9d6503490f954978ac50857c764476fb1611aaaab1f21f0` |
| x86-64 | `d3d12core.dll` | `c4326ab9205af97fd55bd219c037449e7ee1d404754cf80ee3b308355bbee184` |
| x86 | `d3d12.dll` | `f0d05d1e37dc433485afad5581fe435c33437af1c252fd74f47e40106439b209` |
| x86 | `d3d12core.dll` | `137e04a0a1b43f6bd18b5e25ac172c122a1dd1d059cc1c8163504292318a0144` |

The isolated tool `IL2-Korea-D47-AllocatorOnly-f3e06d0b` was created from the
same NUMA-capable D42 Wine base as D45 while Steam was stopped. All four
installed hashes match the package. No source tool, official Proton build,
game file, or launch option was changed.

## Runtime decision

Run the isolated custom Proton tool with empty launch options, no RenderDoc,
and no shader override.

- Clean blocks and broad flicker: the allocator correction is the minimal
  graphics fix; omit the depth bypass upstream.
- Blocks return with the allocator activation verified: both D45 behaviors are
  required and must be justified in one app-scoped proposal.

Fine sandy or film-grain lighting is normal on Windows and is not a failure.

The runtime result is recorded in
[`evidence-d47-allocator-only-wired-result.md`](evidence-d47-allocator-only-wired-result.md).
