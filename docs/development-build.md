# VKD3D-Proton development build

## Purpose

The launch-option controls did not isolate the graphics defect. Development
work therefore begins with an unmodified build of the exact VKD3D-Proton commit
inside the selected Proton Experimental build. This is a parity control, not a
candidate fix.

## Source and toolchain

- Repository: `https://github.com/HansKristian-Work/vkd3d-proton.git`
- Source commit: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Generated build identifier: `0x3dfc6f07d0953b1`
- VKD3D source version: `v3.0.1-236-g3dfc6f07`
- Meson: `1.11.2`
- Ninja: `1.13.2`
- x64 and x86 MinGW-w64 GCC: `16.1.0`
- Build type: release development build; build directories retained

The recursive source checkout is locally retained under ignored `src/`; build
products are retained under ignored `build/`.

## Reproduction command

From `src/vkd3d-proton` at the pinned commit:

```bash
./package-release.sh il2-baseline-3dfc6f07 ../../build --dev-build
```

This is VKD3D-Proton's official package-release development-build path. Both
architectures completed successfully.

## Build artifacts

Directory:
`build/vkd3d-proton-il2-baseline-3dfc6f07/`

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `79c6d14d110bf0337cdec9b4f712cc61b01ac59fc15bdc8446a2cb75bec7f481` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `ba3395a3a20dc82039b8ff011c51b7d0c19921504d12969e71a863c63d191ca9` |
| `x86/d3d12.dll` | PE32 i386 | `e0f64e7b97d148c796b137c404cb8e54810cbf7114b1549f2abbb9185ca36c30` |
| `x86/d3d12core.dll` | PE32 i386 | `b55cb67ae733810ffa130b7952981820de8836706af77e3c19f13aa4f0a87152` |

Different hashes from Proton's packaged DLLs are expected because the local
compiler and link environment differ. Architecture and D3D12 export names were
checked. D00 did not establish runtime parity because stock Proton replaced the
prefix-copied DLLs during launch.

## Safety state

The full prefix backup
`il2-korea-247970-prefix-20260806T114004Z.tar.zst` exists under the user's local
state directory and passes its SHA-256 verification.

The unmodified local DLLs were installed into the AppID 247970 prefix on
2026-08-06. Immediately before installation, the installer created and
verified this DLL-only backup:

```text
/home/USER/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T155429Z
```

The four DLL hashes matched the build-artifact table immediately after
installation, and no Proton installation file was modified. This state was not
durable: Proton recopied its packaged files when the game launched.

Installation was restricted to the four D3D12 DLLs in the AppID 247970 prefix:

```bash
./scripts/install-vkd3d-build.sh install \
  --build-dir "$PWD/build/vkd3d-proton-il2-baseline-3dfc6f07" \
  --yes
```

Rollback, with Steam and the game stopped:

```bash
./scripts/install-vkd3d-build.sh restore \
  --backup-dir /home/USER/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T155429Z \
  --yes
```

## D00 invalidated parity attempt

D00 completed on 2026-08-06 and reproduced the same corruption. Because the
local build and packaged build use the same source identifier, the log cannot
distinguish them. D01a later proved that the prefix-copy method is overwritten
by Proton at launch. D00 must therefore be treated as another packaged-Proton
reproduction, not local-build parity. See
[`evidence-d00-local-build.md`](evidence-d00-local-build.md).

## D01 trace-only build

- Source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Local diagnostic commit: `d0b4421f` (`il2-korea-resource-trace`)
- Build directory: `build/vkd3d-proton-il2-resource-trace-3dfc6f07/`
- Gate: `VKD3D_IL2_RESOURCE_TRACE=1`

The instrumentation records stable resource cookies and the parameters of
`CreateReservedResource`, `GetResourceTiling`, `UpdateTileMappings`, and
`CopyTileMappings`. Logging is disabled unless the gate is set. It does not
alter synchronization, image layouts, descriptors, allocations, queues, or
resource lifetime.

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `37a302c0768f5755f47dca7c26724cdfc1ccd291825b3b397ccd64f5260d8942` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `e8b4de0df971fbf6e4e4b267210815b8bad1f38479c00d6bc8c7e8ebd484ac19` |
| `x86/d3d12.dll` | PE32 i386 | `7b867e13c54908dac7adf044c01a8a9985c59d538af4377871c52c6091962807` |
| `x86/d3d12core.dll` | PE32 i386 | `3260a153211725afa7e84364d3bc931be7e81b4df841e78a8acd4610d35c5d04` |

Installation displaced the unmodified local DLLs into the verified rollback
directory:

```text
/home/USER/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T160657Z
```

Before D01 launched, Steam auto-updated the game to build `24596901`. Its
install step restored all four prefix D3D12 DLLs to hashes matching Proton
Experimental, so the trace marker cannot be active in U00. The diagnostic DLLs
remain available in the retained build directory. U00 subsequently confirmed
the same corruption, so the trace build is cleared for reinstallation before
D01.

The trace build was copied into the prefix again after U00 on 2026-08-06. The
clean Proton-DLL rollback point was:

```text
/home/USER/.local/state/il2-korea-proton-investigation/vkd3d-dll-backups/vkd3d-dll-backup-20260806T165614Z
```

D01a contained no `IL2TRACE` marker. Post-run prefix hashes exactly matched the
packaged Proton DLLs, proving that Proton replaced the diagnostic files before
VKD3D initialized. Zero reserved/tiled-resource calls in that log are therefore
not evidence about the game. See
[`evidence-d01-invalid-prefix-install.md`](evidence-d01-invalid-prefix-install.md).

## Corrected custom-Proton method

`scripts/create-custom-proton.sh` creates a Btrfs copy-on-write clone of Proton
Experimental under Steam's `compatibilitytools.d`, replaces only the clone's
four packaged VKD3D-Proton DLLs, adds a unique compatibility-tool manifest, and
records hashes and source versions. It does not modify Proton Experimental or
the game prefix. When the clone is selected, Proton's normal DLL-copy step
installs the diagnostic files rather than overwriting them.

After Steam is stopped, create the tool with:

```bash
./scripts/create-custom-proton.sh \
  --source-tool "/home/USER/.local/share/Steam/steamapps/common/Proton - Experimental" \
  --build-dir "$PWD/build/vkd3d-proton-il2-resource-trace-3dfc6f07" \
  --destination "/home/USER/.local/share/Steam/compatibilitytools.d/IL2-Korea-Diagnostic-3dfc6f07" \
  --tool-name IL2-Korea-Diagnostic-3dfc6f07 \
  --yes
```

Restart Steam and select `IL2-Korea-Diagnostic-3dfc6f07` for AppID 247970.
The next log must contain `IL2TRACE enabled` before any API counts are
interpreted. Rollback is selecting Proton Experimental again; the custom tool
must not be removed while Steam is running.

The custom tool was created and verified on 2026-08-06. A recursive comparison
against Proton Experimental reports only the four intended DLL differences,
plus the new manifest and diagnostic metadata. All recorded DLL hashes verify.
On Btrfs its apparent size is 1.45 GiB, while the clone shares 974.88 MiB of
extents and reports only 16 KiB exclusive before runtime-generated files.

D01b subsequently passed all validity gates: the trace marker occurred once,
the build string carried the modified-tree `+` suffix, and the four post-run
prefix hashes matched the diagnostic artifacts. No reserved-resource or
tile-mapping API was called during the corrupted menu and mission. See
[`evidence-d01b-sparse-trace.md`](evidence-d01b-sparse-trace.md).

## D02 ordinary-texture trace build

- Source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Prior sparse diagnostic commit: `d0b4421f129b72e6127e6b9abd4028e8df946ea7`
- D02 diagnostic commit: `54797ad35d0dcd921f2e65a98121f2c6d98754a4`
- Gate: `VKD3D_IL2_TEXTURE_TRACE=1`
- Build identifier: `0x54797ad35d0dcd9`

The gate records bounded texture creation/destruction, normalized SRV mip and
layer ranges, texture-region copies, and whole-resource texture copies using
stable VKD3D resource cookies. Event limits are 20,000 creates, 40,000 SRVs,
40,000 copies, and 20,000 destroys. A one-time suppression marker makes
truncation explicit. The patch changes no resource states, Vulkan layouts,
allocation choices, descriptor contents, queues, or synchronization.

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `37a302c0768f5755f47dca7c26724cdfc1ccd291825b3b397ccd64f5260d8942` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `f09342b31fd5092778ebf20c5d66af37b4973968f0eef4220b909d5f0858e52a` |
| `x86/d3d12.dll` | PE32 i386 | `7b867e13c54908dac7adf044c01a8a9985c59d538af4377871c52c6091962807` |
| `x86/d3d12core.dll` | PE32 i386 | `8283693760de86dd7bca8e20a0de94bb6d23cbc56e59ab51ad6361e4c6133083` |

The D01 custom tool remains an untouched rollback/control. D02 must use a
separately named custom tool created from Proton Experimental, so selecting
Proton Experimental or the D01 tool is sufficient rollback.

After Steam is fully stopped, create it with:

```bash
./scripts/create-custom-proton.sh \
  --source-tool "/home/USER/.local/share/Steam/steamapps/common/Proton - Experimental" \
  --build-dir "$PWD/build/vkd3d-proton-il2-resource-trace-3dfc6f07" \
  --destination "/home/USER/.local/share/Steam/compatibilitytools.d/IL2-Korea-D02-Texture-Trace-54797ad3" \
  --tool-name IL2-Korea-D02-Texture-Trace-54797ad3 \
  --yes
```

The first creation attempt safely refused because Steam was still running; no
custom-tool file was created or overwritten.

After Steam exited, the same command succeeded at
`2026-08-06T17:39:03+00:00`. A recursive comparison against Proton Experimental
reports exactly the intended four D3D12/D3D12Core differences after excluding
the new compatibility manifest and diagnostic metadata. All four installed
hashes match the D02 build table above. See
[`evidence-d02-preparation.md`](evidence-d02-preparation.md).

D02-r1 subsequently passed the runtime validity gates. The corruption remained
visible at 1,385 m, 2,355 multi-mip block-compressed resources had complete
geometric upload coverage, no partial resource was found, and all 4,185 SRV
descriptions used a zero minimum-LOD clamp. The corrected pre-cap discriminator
is 405 placed BC3 textures with an SRV but no logged incoming upload/copy. See
[`evidence-d02-ordinary-texture-trace.md`](evidence-d02-ordinary-texture-trace.md).

## D03 placed-resource alias trace build

- D03 diagnostic commit: `cfca234ebaff261e5fc1aa1df2a9f5520fef5e96`
- Gates: `VKD3D_IL2_TEXTURE_TRACE=1` and `VKD3D_IL2_ALIAS_TRACE=1`
- Build directory: `build/vkd3d-proton-il2-alias-trace-3dfc6f07/`
- Custom tool: `IL2-Korea-D03-Alias-Trace-cfca234e`

The fresh official development build completed for x86-64 and x86. Its alias
gate records only placed-resource heap intervals, destruction, and explicit
legacy alias barriers. The D02 gate remains enabled in the same run so stable
cookies can identify the no-incoming-copy SRV class; combining diagnostic logs
does not change rendering behavior.

The custom tool was created after Steam exited. Its four installed DLL hashes
match the retained build, and recursive comparison with Proton Experimental
reports only those four intended differences after excluding the custom
manifest and metadata. Exact hashes, launch options, and rollback are recorded
in [`evidence-d03-preparation.md`](evidence-d03-preparation.md).

D03-r1 passed both runtime trace gates and the DLL-hash verification. All 585
same-run candidate textures observable before the copy cap have matching
placed-resource records; none overlaps any traced placed buffer or texture
range. The full run records zero explicit legacy alias barriers. See
[`evidence-d03-alias-trace.md`](evidence-d03-alias-trace.md).

## D04 unmodified current-upstream control

- VKD3D-Proton: `84c87c8390d9df75ba41d911496296fe13f0e275`
- `dxil-spirv`: `cc75a0c98d34d7bcc03560527c799b52e48b4d1f`
- Build directory: `build/vkd3d-proton-il2-upstream-84c87c83/`
- Custom tool: `IL2-Korea-D04-Upstream-84c87c83`

The official development-build method completed for both architectures in an
isolated, unmodified worktree. Recursive comparison with Proton Experimental
shows only the four intended VKD3D DLL changes plus custom-tool metadata.

D04-r1 is visually unchanged in the menu and at 6,400 m. All four post-run
prefix hashes match the build, closing the broad current-upstream version lead.
The game's own `tex.log` adds a distinct texture-provider fallback lead; see
[`evidence-d04-upstream-preparation.md`](evidence-d04-upstream-preparation.md)
and [`evidence-d04-upstream-result.md`](evidence-d04-upstream-result.md).

## D05a gated BC3 border normalization

- Base VKD3D-Proton: `84c87c8390d9df75ba41d911496296fe13f0e275`
- Diagnostic commit: `35bd875cf58a555a64fa366926c04cd6b0664611`
- Gate: `VKD3D_IL2_BC3_BORDER_COPY=1`
- Build directory: `build/vkd3d-proton-il2-d05-bc3-35bd875c/`
- Custom tool: `IL2-Korea-D05-BC3-35bd875c`

D05a changes only the four one-texel BC3 border shapes already observed on the
2048x2048, one-mip baked-terrain cache. It expands the thin dimension to one
complete four-texel compressed block and logs every adjustment. It is disabled
by default and is a causal diagnostic, not a proposed application override or
general fix. Both architectures compile, the installed custom tool differs
from Proton Experimental only in the four VKD3D DLLs plus its manifest and
metadata, and `D05-bc3-r1` is prepared. Exact hashes and the test protocol are
in [`evidence-d05-preparation.md`](evidence-d05-preparation.md).

D05a-r1 loaded the intended DLLs but emitted zero adjustment records. Its
unchanged image is therefore not evidence against normalization. The helper
required a non-null source box and exact source format before it could log a
match; D02 had only recorded the already-converted extent. See
[`evidence-d05-result.md`](evidence-d05-result.md).

## D05b footprint-aware revision

- Diagnostic commit: `f6416c79dafabcb76e2e095935dfcd0c428b9208`
- Build directory: `build/vkd3d-proton-il2-d05b-bc3-f6416c79/`
- State: completed; 432 candidates, zero adjustments, 432 explicit rejections

D05b accepts either an explicit source box or the footprint-only representation
used by `CopyTextureRegion`. Before adjustment it logs every target-class
candidate, source representation, formats, physical row capacity, and a safety-
rejection bitmask. This prevents another visually ambiguous zero-match run.
The run showed that every source footprint is `R32G32B32A32_UINT`, so the
BC3-source safety model was incorrect. See
[`evidence-d05b-result.md`](evidence-d05b-result.md).

## D05c exact reinterpret-copy revision

D05c commit: `5391ec7f427795fe0fc151047422629d849e35be`

D05c retains the opt-in gate and exact 2048x2048 one-mip BC3 destination plus
the four observed source shapes. It maps each 128-bit
`R32G32B32A32_UINT` source texel to one 4x4 BC3 block, including Vulkan
`imageExtent`, `bufferRowLength`, and `bufferImageHeight`. This mirrors the
block-unit conversion already used by VKD3D-Proton's image-to-image copy path.
Its runtime behavior has now been measured: all 202 encountered candidates were
adjusted with zero rejects, and the image remained unchanged. See
[`evidence-d05c-result.md`](evidence-d05c-result.md).

The official development build completed for x86-64 and x86 in:

```text
build/vkd3d-proton-il2-d05c-bc3-5391ec7f/
```

| File | Architecture | SHA-256 |
|---|---|---|
| `x64/d3d12.dll` | PE32+ x86-64 | `9c05de19c472684f5a1910fd7f123fc0d1b4fad31ece2dd2b6ce3d41dff147d2` |
| `x64/d3d12core.dll` | PE32+ x86-64 | `bebc057635a65bf9071afb38a5df96a0e5e88e0e71f274338c67488b773f4cc4` |
| `x86/d3d12.dll` | PE32 i386 | `0a4166157ab5576ba1711d4a9a047bcb879b32508013b69febd5c5a19b798288` |
| `x86/d3d12core.dll` | PE32 i386 | `6b2033e2939d349af998c36af006b819ffadab10619836112c41b0d718eb5943` |

String inspection confirms the D05c enable, adjustment, rejection, and log-cap
markers. A synthetic test containing all four observed shapes validates the
new extents and buffer layouts. The isolated custom tool
`IL2-Korea-D05c-Reinterpret-5391ec7f` was created from Proton Experimental
`experimental-11.0-20260724c`; all four installed DLL hashes match the table.

## D06 focused baked-cache trace

D05c executed its intended conversion on 202/202 candidates with zero rejects,
but the terrain image remained unchanged. D06 returns to a trace-only current-
upstream build at diagnostic commit
`376652dc46fc323d2d2eb59ae8bd0ebd6cf3d189`. It filters only 2048x2048,
one-mip textures and records creation, SRVs, copies, barriers/layouts, and
destruction when `VKD3D_IL2_BAKED_CACHE_TRACE=1` is set.

Both DLL architectures built successfully. The isolated custom tool is
`IL2-Korea-D06-CacheTrace-376652dc`; see
[`evidence-d06-preparation.md`](evidence-d06-preparation.md) for hashes and the
runtime protocol. No prefix DLL or source Proton file was overwritten.
The source tool and prefix remain unchanged.
