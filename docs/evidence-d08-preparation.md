# D08 general VKD3D-Proton fix: preparation record

> D08 is complete. See [`evidence-d08-result.md`](evidence-d08-result.md) for
> the validated runtime result. This document retains the pre-run protocol and
> hashes.

## Purpose

D08 validates the clean general replacement for D07. It must reproduce the
terrain repair without an IL-2 executable/AppID check, terrain-cache filter,
observed-shape filter, or diagnostic environment variable.

## Identity

- Upstream base: `84c87c8390d9df75ba41d911496296fe13f0e275`
- Candidate commit: `cf11ba76`
- Build: `build/vkd3d-proton-il2-general-block-copy-cf11ba76/`
- Custom tool: `IL2-Korea-D08-GeneralFix-cf11ba76`
- Source tool: Proton Experimental `experimental-11.0-20260724c`

| File | SHA-256 |
|---|---|
| x64 `d3d12.dll` | `616873065fe60edb671a09d98201951ccb32a91fd591c0c2f5e2ad55983ff22a` |
| x64 `d3d12core.dll` | `0ec85f20a6edd0052d672baad3e7662e02c68ca8901e2379dcd448443c6f1d86` |
| x86 `d3d12.dll` | `7765f67186f286048bdb02ae1feb37c1fc47545458221618649abfbeef8e384b` |
| x86 `d3d12core.dll` | `87a0679156c49261dfd2fe0c1eda90ed77d4b12428c1c59d1c69f43ec4c92e60` |

The custom tool was created only after Steam stopped. The source Proton tool,
game files, and prefix were not modified. All four installed VKD3D-Proton DLL
hashes match the retained package.

## Patch and test scope

The patch converts buffer-to-image copy geometry through physical blocks when
the placed-footprint and destination image formats use equal-size physical
blocks. The included 64x64 RGBA32_UINT-to-256x256 BC3 regression reports four
failures on the old helper and passes all 22 assertions with the fix. Existing
BC/RGBA and block-compressed copy tests pass all 147 and 50 assertions.

## Runtime protocol

Restart Steam and select `IL2-Korea-D08-GeneralFix-cf11ba76` for AppID 247970.
Use exactly:

```bash
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D08-general-fix-r1 OMP_NUM_THREADS=16 KMP_AFFINITY=disabled %command%
```

Do not set `VKD3D_IL2_BC3_PAGE_COPY`.

1. Inspect the rotating menu aircraft for blocks, flicker, and shadow/effect
   corruption.
2. Run the same Korea flight and capture terrain near 5,000-5,500 m.
3. Check for rectangular missing pages, black/hollow terrain, and magenta page
   seams.
4. Exit the game, then run:

```bash
./scripts/collect-proton-log.sh collect D08-general-fix-r1
./scripts/collect-game-logs.sh D08-general-fix-r1
```

A valid run must identify VKD3D-Proton commit `cf11ba76`, contain no D07 enable
marker, and load the four general-fix DLLs. A successful result repeats the D07
terrain repair without a diagnostic gate. Menu observations are classified
separately.

Rollback is selecting Proton Experimental or the retained D07 tool. Do not
delete a custom tool while Steam is running.
