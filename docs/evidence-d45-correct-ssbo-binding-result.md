# D45 correct SSBO descriptor binding: result

## Runtime result

D45-r1 and an independent D45-r2 restart are both visually successful. With
empty Steam launch options, the user reports the square blocks and broad light
flicker are completely resolved. Real lighting remains present. The fine sandy
or film-grain lighting is excluded because it also occurs on native Windows.

This is the first integrated build which repairs both spatial and temporal
presentations of the tiled-light failure repeatably. The restart gate was
important because an earlier D42 frame looked favourable while its allocation
was still broken.

## Runtime provenance

The running Steam command selects
`IL2-Korea-D45-CorrectSSBO-1368b538` for AppID 247970. Prefix `config_info`
points at that tool. The running `IL2Series.exe` memory map contains the prefix
`d3d12.dll` and `d3d12core.dll`, plus the D45 tool's VKD3D support DLLs.

The live prefix DLL hashes exactly match the D45 package:

| Architecture | File | SHA-256 |
| --- | --- | --- |
| x86-64 | `d3d12.dll` | `cb9171d18a137f4d07b276048c5f748042e6ffe6c83ce5d919e17b1d9bf59f7f` |
| x86-64 | `d3d12core.dll` | `a164833c4a798c21f16e3d7690538a9ff7de31f78f7a31fd4d25d317153d4d47` |
| x86 | `d3d12.dll` | `fbe779f41b59a7f13987547dfec5cb9958c3b2ca6edaa95b84d732419928e063` |
| x86 | `d3d12core.dll` | `e489f24fd7864e2b2e71201b3e35fd3c88c0fef021d27727bd2b761c02c7a9ce` |

No RenderDoc layer, shader override, launch workaround, game modification, or
GitHub action is involved.

## Isolation gate

D46 attempted to remove D38's depth-gate bypass while retaining the corrected
allocator binding. Although prefix provenance confirmed the D46 tool, later
source review found that its `IL2Series.exe` application-table entry was also
absent. The remaining allocator quirk was therefore never selected. D46 is an
invalid no-quirk control, not evidence that the depth behavior is independently
required. Corrected allocator-only D47 restores the application wiring and is
visually clean with the original depth predicates. The depth bypass is
therefore not part of the minimal fix. See
[`evidence-d47-allocator-only-wired-result.md`](evidence-d47-allocator-only-wired-result.md).
