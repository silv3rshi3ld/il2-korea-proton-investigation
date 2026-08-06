# Read-only game binary inspection

## Scope and evidence hygiene

The installed game files were inspected read-only on 2026-08-06 using PE import
tables, exported/demangled symbol strings, and focused diagnostic strings. No
`.gtp` archive was unpacked, no game asset was copied, and no copyrighted game
binary is stored in this repository. Compiled strings identify implemented
paths and assertions; they do not expose source control flow or prove that a
particular path executed during the defective frame.

Game build ID: `24596901`.

| File | Size | SHA-256 |
|---|---:|---|
| `bin/game/IL2Series.exe` | 17,078,152 bytes | `4bbdc1f317d3b9f8875884de262143f523aad47705c70cad26c304775a6e7369` |
| `bin/game/dxBackend12.dll` | 1,293,824 bytes | `d193723615cffd964d90e72c01227d635bd5266954339b796c7a220c3a12403f` |

## D3D runtime path

The backend imports `d3d12.dll`, `dxgi.dll`, `dxcompiler.dll`, and
`D3DCOMPILER_47.dll`. This independently agrees with the runtime Proton logs:
the game uses its D3D12 backend through VKD3D-Proton and DXVK's DXGI; no
evidence requires a D3D11 rendering path.

The focused backend scan finds symbols for:

- `CreateCommittedResource` and `CreatePlacedResource`;
- `CopyResource` and `CopySubresourceRegion`;
- `UpdateSubresource` and `UpdateSubresourceAsync`;
- `GenerateMips` and `DXRenderer12::setTexMipLevel`;
- `DXTexture12::createShaderResourceViewMipsSlices`.

It also contains distinct assertion text for insufficient free
`UpdateSubresource` buffer space on async threads and on the main graphics
thread. These are useful instrumentation targets, not observed runtime errors.

No `CreateReservedResource` symbol was found by this focused scan. D01b's valid
runtime trace is the stronger evidence: it records zero reserved-resource,
tiling-query, tile-update, or tile-copy calls during reproduction.

## Meaning of “tiled” in the game

The executable contains RTTI/symbol strings for `CBFManagerTiled`,
`CBlocksArrayTiled`, `CDistantLOD`, `CTerrainArray`, `CDBTiledBlocks`, and
`ChunkSystem::resetTextureCashe`. These names show that the game organizes
terrain and distant LODs in tiled/page-like high-level structures. They do not
mean the D3D12 backend uses reserved/sparse resources.

The combined binary and runtime evidence therefore moves the next diagnostic
step to ordinary texture uploads, copies, mip/SRV selection, and lifetime.
D02 records those operations by stable VKD3D resource cookie without changing
rendering behavior.
