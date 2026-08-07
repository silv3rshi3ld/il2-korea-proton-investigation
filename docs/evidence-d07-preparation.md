# D07 full terrain-page reinterpret diagnostic: prepared

## Purpose and scope

D07 tests whether VKD3D-Proton is carrying uncompressed source-footprint
dimensions into a block-compressed destination without converting units. It is
not an application override and is inert unless
`VKD3D_IL2_BC3_PAGE_COPY=1` is set.

The diagnostic accepts only:

- a 2048x2048, one-layer, one-mip `DXGI_FORMAT_BC3_UNORM` destination;
- a footprint-only `DXGI_FORMAT_R32G32B32A32_UINT` source;
- one of the observed extents: `64x64`, `128x128`, `64x1`, `1x64`, `128x1`,
  or `1x128`;
- matching 16-byte physical elements, block-aligned destination offsets,
  adequate row pitch, and an adjusted region wholly inside the destination.

Each accepted source element becomes one 4x4 BC3 block. The destination offset
is not scaled. The Vulkan image extent, buffer row length, and buffer image
height are expressed in destination texels. Every candidate is logged before
adjustment, so the run also directly verifies the square interiors' placed-
footprint format.

## Identity

- Upstream base: `84c87c8390d9df75ba41d911496296fe13f0e275`
- Diagnostic commit: `833cafa0b1bc87153b2e9d2859c6830f4553f80e`
- Build: `build/vkd3d-proton-vkd3d-proton-il2-d07-page-833cafa0/`
- Installed tool: `IL2-Korea-D07-PageCopy-833cafa0`
- Source Proton: Experimental `experimental-11.0-20260724c`

| File | SHA-256 |
|---|---|
| x64 `d3d12.dll` | `92826f63c7ff46dc377a6b77f82830eefc0b34ea97297e8c691aa156f6a0f3f4` |
| x64 `d3d12core.dll` | `0a3d12376ec6f6792871da5e908c8395932296f6b97de6b8d832c16c38bf4c3a` |
| x86 `d3d12.dll` | `9ec0c99d17fa04494b076233aaf751d2409933e34cd49d4ac4f3724926ded7fd` |
| x86 `d3d12core.dll` | `da9ac29cbf237877d9ac80e2b4c38d3950061d99904ded9402649c986413e505` |

Both architectures compiled with VKD3D-Proton's official development-package
method. Installed hashes match the build outputs. Proton Experimental and the
game prefix were not modified.

## Runtime protocol

Run ID: `D07-page-copy-r1`

Select `IL2-Korea-D07-PageCopy-833cafa0` and use exactly:

```bash
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D07-page-copy-r1 OMP_NUM_THREADS=16 KMP_AFFINITY=disabled VKD3D_IL2_BC3_PAGE_COPY=1 %command%
```

Use the same autumn free-flight mission and compare the menu plus terrain near
1,300-1,500 m. After exit:

```bash
./scripts/collect-proton-log.sh collect D07-page-copy-r1
./scripts/analyze-bc3-border-copy.py \
  captures/runs/D07-page-copy-r1/steam-247970.log \
  --output captures/runs/D07-page-copy-r1/bc3-page-copy-analysis.md
```

A valid causal result requires at least one square interior candidate with
source format `0x3`, no rejected candidate, and a repeatable visual change.
Rollback is selecting Proton Experimental again.
