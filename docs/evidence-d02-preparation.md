# D02 ordinary-texture trace preparation

## Scope

D02 targets ordinary texture creation/lifetime, normalized SRV mip ranges,
`CopyTextureRegion`, and texture `CopyResource` operations after valid D01b
excluded the D3D12 reserved/tiled-resource API path. It is diagnostic only and
does not alter rendering behavior.

## Source and build

- VKD3D-Proton source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Sparse diagnostic parent: `d0b4421f129b72e6127e6b9abd4028e8df946ea7`
- D02 diagnostic commit: `54797ad35d0dcd921f2e65a98121f2c6d98754a4`
- Runtime gate: `VKD3D_IL2_TEXTURE_TRACE=1`
- Proton source tool: Experimental `experimental-11.0-20260724c`
- Custom tool: `IL2-Korea-D02-Texture-Trace-54797ad3`
- Created UTC: `2026-08-06T17:39:03+00:00`

## Installed DLL verification

| Custom-tool file | SHA-256 |
|---|---|
| `x86_64-windows/d3d12.dll` | `37a302c0768f5755f47dca7c26724cdfc1ccd291825b3b397ccd64f5260d8942` |
| `x86_64-windows/d3d12core.dll` | `f09342b31fd5092778ebf20c5d66af37b4973968f0eef4220b909d5f0858e52a` |
| `i386-windows/d3d12.dll` | `7b867e13c54908dac7adf044c01a8a9985c59d538af4377871c52c6091962807` |
| `i386-windows/d3d12core.dll` | `8283693760de86dd7bca8e20a0de94bb6d23cbc56e59ab51ad6361e4c6133083` |

All four hashes match the retained build artifacts. Recursive comparison with
Proton Experimental reports only these four differences after excluding the
new `compatibilitytool.vdf` and `il2-korea-diagnostic-metadata.txt` files.

## Safety and rollback

The tool is a separate copy-on-write compatibility tool. Its creation did not
modify Proton Experimental, the D01 custom tool, the game prefix, or game
files. Rollback is selecting Proton Experimental or the D01 tool in Steam; do
not remove any custom tool while Steam is running.

No D02 runtime conclusion is recorded here. A run is valid only if its Proton
log contains the `IL2TEX enabled` marker and the post-run prefix DLL hashes
match this table.
