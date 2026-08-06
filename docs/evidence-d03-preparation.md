# D03 placed-resource alias trace preparation

## Diagnostic question

D02's corrected cap-aware analysis found 405 pre-cap placed, multi-mip BC3 textures that receive an SRV but no logged
incoming upload or texture copy. D03 asks whether those resources:

- overlap a placed buffer or another texture in the same D3D12 heap;
- overlap while both resources are alive;
- or are named by an explicit D3D12 aliasing barrier.

This is a diagnostic build, not a rendering workaround. It does not change
allocation, resource contents, descriptor contents, barriers, layouts, queues,
or synchronization.

## Source and build

- VKD3D-Proton source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- D02 parent: `54797ad35d0dcd921f2e65a98121f2c6d98754a4`
- D03 diagnostic commit: `cfca234ebaff261e5fc1aa1df2a9f5520fef5e96`
- Runtime gates: `VKD3D_IL2_TEXTURE_TRACE=1` and
  `VKD3D_IL2_ALIAS_TRACE=1`
- Build method: official `package-release.sh --dev-build`, x86-64 and x86
- Build directory: `build/vkd3d-proton-il2-alias-trace-3dfc6f07/`
- Custom tool: `IL2-Korea-D03-Alias-Trace-cfca234e`
- Custom-tool creation: `2026-08-06T18:08:49+00:00`

The alias gate logs stable resource cookie, buffer/texture kind, heap pointer,
D3D heap/backing size, heap type/flags, relative offset, allocation size,
resource description/flags, destruction, and explicit legacy alias barriers.
Event caps are 30,000 creates, 30,000 destroys, and 20,000 barriers, each with
an explicit suppression marker.

`scripts/analyze-alias-trace.py` combines these records with D02's SRV and copy
events from the same run. It reports same-heap range overlap, lifetime overlap,
and alias-barrier involvement for the no-incoming-copy SRV class. Heap pointers
are local diagnostic identifiers and must be redacted if a log is posted.

## Build and installed hashes

| File | SHA-256 |
|---|---|
| x64 `d3d12.dll` | `f749aaa87761c3bcd1e1d34b3d83a5840692e75730f4b12c4a554cc5f5cebbc9` |
| x64 `d3d12core.dll` | `8ade4f832496025fb2ab843aea2aeda191692c2671cfc70e7a3983ce7e51461f` |
| x86 `d3d12.dll` | `a5568dc072754683b65f7d467e19c2eea7427c0dc805083b3f69d812328fcd1e` |
| x86 `d3d12core.dll` | `5edf49a453a3daaef42efd73ee4a4032707904f84228aee77b12169aeff7454f` |

All installed custom-tool hashes match the retained build. Recursive comparison
with Proton Experimental reports exactly these four intended DLL differences
after excluding the new compatibility manifest and diagnostic metadata.

## Prepared run

- Run: `D03-r1`
- Exact launch option:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D03-r1 OMP_NUM_THREADS=16 KMP_AFFINITY=disabled VKD3D_IL2_TEXTURE_TRACE=1 VKD3D_IL2_ALIAS_TRACE=1 %command%
```

The run is valid only if the log contains exactly one `IL2TEX enabled` marker,
exactly one `IL2ALIAS enabled` marker, and the post-run prefix DLL hashes match
the table above. One representative reproduction is sufficient. A screenshot
is needed only to establish that the defect was visible during the trace; the
same low-altitude Singo-dong view is preferred.

## Safety and rollback

The D03 tool is a separate copy-on-write compatibility tool. Proton
Experimental, the D01/D02 tools, the prefix, and game files were not modified
when it was created. Rollback is selecting Proton Experimental or D02 in Steam.
Do not delete any custom compatibility tool while Steam is running.
