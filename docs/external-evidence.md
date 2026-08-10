# External evidence: VKD3D-Proton issue #3134

[VKD3D-Proton issue #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134),
“IL-2 Korea - Textures not loading in-game,” is the closest known public
report. It was opened on 2026-06-29 and remains open with no maintainer comments
as of 2026-08-06. It directly reports the same menu squares and absent ground
textures; it is not merely an analogous defect from another game.

## Artifact handling

The public screenshot and log were inspected from temporary storage. They are
not copied into this repository because the screenshot contains game content
and the unpacked Proton log is about 226 MB.

| Artifact | Upstream link | SHA-256 |
|---|---|---|
| Screenshot, 2554x1440 PNG | [Issue attachment](https://github.com/user-attachments/assets/1bb3ce83-9dfc-4401-89e1-f039683213e2) | `5bcd2982f683c8566566ace5f36b6a7bd488d8ef6898be6788c06bd658bed7ad` |
| Compressed Proton log, 5,854,495 bytes | [Issue attachment](https://github.com/user-attachments/files/29466992/steam-12814649275476606976.tar.gz) | `e7fcc9b4677b8b1fd4d7081dc7c3bf2d254386db1d7e0e644fa30c8897caa1f1` |

The supplied signed `private-user-images.githubusercontent.com` URL is
temporary. The stable GitHub issue-attachment URL above should be used in
notes and drafts.

## Screenshot observations

- Most of the terrain surface is black or absent.
- A small number of rectangular terrain tiles remain visible.
- Bright magenta fringes or seams appear at some visible tile boundaries.
- Some roads and vegetation patches remain visible while the surrounding
  ground is missing.
- The cockpit and UI render, so this is not a total render-target failure.

The tile-shaped pattern increases the priority of streaming, sparse/reserved
resources, residency, mip selection, aliasing, and synchronization hypotheses.
It does not distinguish between them by itself.

## Filtered log facts

The issue log used a Faugus-launched, bleeding-edge Proton build rather than a
normal Steam AppID 247970 launch, so it is corroborating evidence rather than
the controlled local baseline.

- Launcher: `/home/USER/Faugus/IL2Series/Launcher.exe`
- Game process later launched: `IL2Series.exe`
- VKD3D-Proton build identifier: `074c5b6352d58f4`
- GPU: Radeon RX 9070 XT
- Driver: RADV, Mesa 26.1.3
- Game module path: `IL2Series.exe` loads the game's `dxBackend12.dll`, then
  native-overridden prefix `d3d12.dll`, `d3d12core.dll`, and `dxgi.dll`.
- No D3D11 module was observed after the game process began.
- Host-visible device-local upload heaps were reported viable and enabled.
- `VK_EXT_descriptor_buffer` and mutable descriptor types were enabled.
- Sparse-resource support was advertised.
- VKD3D reported that it could not allocate three out-of-band queue families
  and would perform that work on in-band queues. This is diagnostic context,
  not evidence of incorrect synchronization.
- No VKD3D error, Vulkan device loss, or GPU hang was present in the log.
- The split-barrier `END_ONLY` warning appeared 64,140 times over roughly 118
  seconds.
- `GetNumaNodeProcessorMaskEx` was not present in this log, so it contributes
  no evidence to the independent startup investigation.

The application also printed many `no saver for property` messages. They appear
to originate in its mission/property serialization path, and there is no
evidence yet connecting them to missing terrain.

## Split-barrier interpretation

At the locally installed VKD3D-Proton commit, `BEGIN_ONLY` transition barriers
are skipped and their matching `END_ONLY` barriers are processed as ordinary
transitions because Vulkan has no D3D12 split-barrier equivalent. The warning is
emitted after that conservative handling. See
[`d3d12_command_list_ResourceBarrier()` at commit 3dfc6f07](https://github.com/HansKristian-Work/vkd3d-proton/blob/3dfc6f07d0953b1e8b41705275c2c59cc7374fc5/libs/vkd3d/command.c#L12242-L12358).

The warning volume shows heavy use of split barriers, but does not establish
that VKD3D omits them. Correlation with a resource, queue, or visible failure is
still required.

## Consequences for this investigation

1. Treat issue #3134 as the direct external report and update it with controlled
   results instead of opening a duplicate issue.
2. Preserve E01 (`no_upload_hvv`), E02 (`single_queue`), and, if testing
   resumes, E03 (descriptor buffer disabled) as separate tests. The issue log
   exercised all three normal paths together and therefore cannot isolate one.
3. The cross-GPU reproduction argues against a defect unique to Navi 32, but it
   does not clear RADV or Mesa generally.
4. If the basic controls are unchanged, prioritize filtered telemetry for
   terrain tile creation, sparse mappings/residency, mip ranges, upload paths,
   aliasing, and the queue dependency that makes a tile visible.
