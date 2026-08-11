# D47 correctly wired allocator-only control: result

> [!IMPORTANT]
> Historical runtime result. D47 proved that correcting the allocator removes
> the pixels, but its SSBO/raw-sibling implementation is not the final upstream
> architecture. D50-D52 later isolated the texel-buffer view/OOB boundary, and
> Mesa MR !43672 is now the preferred upstream direction.

## Result

D47 is visually clean. With empty Steam launch options, the user reports the
block artifacts and broad light flicker are completely resolved. Real lighting
and shadows remain. The normal fine sandy or film-grain lighting is excluded
from the defect because it also occurs on native Windows.

This is the decisive minimality result. D47 keeps the original tiled-light
depth predicates and contains only the corrected allocator access plus the raw
descriptor selection. D38's depth-gate bypass is therefore unnecessary once
the allocator is repaired and must not be included upstream.

## Provenance

The prefix `config_info` identifies
`IL2-Korea-D47-AllocatorOnly-f3e06d0b`. All four prefix D3D12 DLL hashes match
the installed D47 package:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `480df8b1e4b97212b9d6503490f954978ac50857c764476fb1611aaaab1f21f0` |
| x86-64 | `d3d12core.dll` | `c4326ab9205af97fd55bd219c037449e7ee1d404754cf80ee3b308355bbee184` |
| x86 | `d3d12.dll` | `f0d05d1e37dc433485afad5581fe435c33437af1c252fd74f47e40106439b209` |
| x86 | `d3d12core.dll` | `137e04a0a1b43f6bd18b5e25ac172c122a1dd1d059cc1c8163504292318a0144` |

Source and binary checks additionally prove:

- `IL2Series.exe` is wired to `il2_korea_quirks`;
- exact shader `7cefa1bc80bb4c70` selects
  `VKD3D_SHADER_QUIRK_FORCE_TYPED_UAV_AS_SSBO`;
- typed-UAV lowering selects `VKD3D_SHADER_BINDING_FLAG_RAW_SSBO`;
- neither depth quirk nor either producer shader hash is present.

No RenderDoc layer, shader override, launch workaround, game modification, or
GitHub action is involved.

## Clean upstream candidate

The validated behavior was reproduced as one clean commit based directly on
VKD3D-Proton master `84c87c83`:

- branch: `fix-il2-tiled-light-allocator`;
- commit: `9b6e15be` (`vkd3d-shader: Work around IL-2 tiled-light allocator`);
- diff: 29 insertions in four files, with no diagnostic code;
- exported patch:
  `patches/0016-vkd3d-shader-Work-around-IL-2-tiled-light-allocator.patch`;
- patch SHA-256:
  `4d43ac526b47d07b9694633de42cacc284e961d9fc84050df5d166c650a7216a`.

The clean package builds without new compiler warnings for both MinGW
architectures. Its binaries are:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `2d0c06072a7badf0f95bc78e7a971ab32662c1bcf60ee0be944b3f712d67ce85` |
| x86-64 | `d3d12core.dll` | `f4cfd361669b31ec4429db946f47034ac4032d955a4bcee89a40c31964f181a4` |
| x86 | `d3d12.dll` | `8fe5c7718fbbb778182411dda6db34f7e573f575c04a7424d799e853d0d9d629` |
| x86 | `d3d12core.dll` | `bddc608b61a8f2fb1dcd7851133367968cc87a94c585b644ed6536d3d930ebe5` |

Source comparison confirms that the four changed files contain the same
allocator/descriptor behavior as D47. `IL2Series.exe` is present in both clean
`d3d12core.dll` architectures, and neither the depth quirk nor its producer
hashes are present.

A later fresh build of baseline `84c87c83` and this exact candidate completed a
matched runtime A/B. The baseline reproduced the blocks and temporal flashing;
the candidate removed both while preserving real lighting and shadows. Its
exact tool and screenshot provenance is recorded in
[`evidence-u01-upstream-candidate-ab.md`](evidence-u01-upstream-candidate-ab.md).

## D47 graphics conclusion

The game performs a 32-bit global atomic through an `R16_UINT` typed UAV. A
literal Vulkan typed-buffer translation is not a legal equivalent and makes
every workgroup reuse the same short allocation prefix. VKD3D-Proton already
emits a raw storage-buffer descriptor sibling for the D3D descriptor. The
minimal compatibility fix makes this one exact shader use that sibling and
emit the atomic as an SSBO operation.

This is app- and shader-hash-scoped, changes no unrelated game, and requires no
hard-coded processor value, launch parameter, or game-file customization.

That conclusion records the minimal successful D47 mechanism at the time. It
does not describe the final upstream ownership after D50-D52.

## Later interpretation

D50 held the buffer size, shader, pipeline, and dispatch fixed while changing
only an R32, R16, R32 view sequence. Only R16 reproduced the restart. D51 then
passed the exact game shader with a full-size R32 alias on both tested RADV
devices and both descriptor backends. D52 retained the normal texel-buffer
lowering and unchanged dxil-spirv, changed only the exact resource's descriptor
selection, and remained visually clean in two game runs.

D47 therefore remains valid evidence that repairing the allocation repairs the
visible artifact. It does not establish that SSBO lowering is necessary.
[Mesa MR !43672](https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/43672)
addresses the RADV GFX10+ texel-buffer OOB behavior directly and is cleaner
than retaining the D47 per-game quirk. The experimental dxil-spirv PR #296 and
VKD3D-Proton PR #3207 were closed unmerged as superseded. Mesa MR !43672
remains open and is the preferred delivery path, but the exact MR revision has
not been game-tested in this investigation.
