# Findings and evidence ledger

## Verified findings

1. Steam AppID 247970 currently launches the 64-bit PE executable
   `bin/game/IL2Series.exe` from the `IL2Series` install directory.
2. `IL2Series.exe` statically imports `dxBackend12.dll`; that game backend
   imports `d3d12.dll` and `dxgi.dll` and exposes numerous D3D12 resource,
   barrier, command-list, descriptor, and texture operations.
3. No game-local D3D12, DXGI, D3D11, DXVK, or VKD3D DLL overrides were found.
4. The prefix D3D12 DLL hashes match VKD3D-Proton in the selected Proton
   Experimental build. Prefix DXGI and D3D11 hashes match that build's DXVK.
5. The selected VKD3D-Proton commit is
   `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`; it already contains many older
   game-specific synchronization, memory, aliasing, compression, and shader
   workarounds. A generic upgrade to those old fixes is therefore not an IL-2
   solution.
6. RADV exposes descriptor buffers, sparse resources, host-visible/ReBAR-capable
   memory topology, and multiple queue families on this host. Runtime logs
   confirm descriptor-buffer, host-visible upload, and multiple queue paths;
   they do not expose whether the affected terrain uses sparse resources.
7. A 16 GiB PCI BAR is mapped for the RX 7800 XT, strongly indicating ReBAR is
   enabled.
8. Linux reports one NUMA node containing CPUs 0-15. Wine/Windows API topology
   as seen by the shipped OpenMP runtime has not yet been captured.
9. VKD3D-Proton issue #3134 independently records the same tile-shaped terrain
   loss and menu squares on an RX 9070 XT with Mesa 26.1.3. Its log confirms the
   D3D12/VKD3D path, host-visible upload heaps, descriptor buffers, sparse
   capability, and multiple queue paths, but does not isolate one as defective.
10. Controlled local runs E00-r1 and E00-r2 reproduce the defect with no VKD3D diagnostic
    option. Runtime module loading confirms `dxBackend12.dll`, DXVK DXGI, and
    VKD3D-Proton D3D12/D3D12Core, with no D3D11 module load.
11. Both E00 runs used host-visible device-local upload heaps, ReBAR budgeting,
    descriptor buffers, and three VKD3D queue workers. This confirms all three
    planned controls address active baseline paths, but does not select a cause.
12. Neither E00 run produced Vulkan device loss, GPU hang/reset, or
    out-of-memory errors. Split `END_ONLY` warnings occur throughout both runs
    but remain non-causal without resource correlation.
13. `VKD3D_CONFIG=no_upload_hvv` is confirmed active in both E01 runs and
    changes the upload heap from device-local host-coherent memory to forced
    host-coherent memory. Both runs show denser vegetation, but both were
    captured near 1,350-1,500 m. Baseline and E02 screenshots show substantially
    less content near 5,000 m and more near 1,245-2,500 m. The E01 visual result
    is therefore altitude-confounded and classified inconclusive.
14. `VKD3D_CONFIG=single_queue` is confirmed active in both E02 runs. Logged
    staggered submissions use queue family 0, but menu and terrain corruption
    remain unchanged. Async compute/transfer queue selection is therefore
    unlikely to be the primary cause; single-queue execution does not rule out
    synchronization or lifetime defects within the remaining queue path.
15. An unmodified x64/x86 VKD3D-Proton development build succeeds from the
    exact installed source commit using the official retained-build method.
    D00 does not establish runtime parity: the local and packaged builds share
    the same build identifier, and D01a later proves that stock Proton replaces
    prefix-copied D3D12 DLLs with its packaged files during launch.
16. The user confirms correct rendering on native Windows. The problem is
    consequently reproducible only on the tested Linux compatibility/driver
    path. This does not yet distinguish a VKD3D-Proton defect, a RADV defect,
    or invalid/undefined D3D12 usage tolerated by the native Windows driver.
17. A trace-only VKD3D-Proton build at local commit `d0b4421f` was built for
    D01. It logs reserved resource creation, resource tiling queries, and tile
    map/copy operations behind `VKD3D_IL2_RESOURCE_TRACE=1`; it makes no
    behavior-changing application override or rendering-path change.
18. Before D01 ran, Steam auto-updated the game from build `24577563` to
    `24596901` (886,044,096 bytes downloaded; 5,418,273,475 bytes staged; 51
    files updated). Proton Experimental and VKD3D-Proton remained unchanged,
    while the update's install step restored the prefix D3D12 DLLs to the
    Proton-supplied hashes. U00 was therefore required as a new-build baseline.
19. U00 establishes that baseline. The user reports the same menu and terrain
    corruption on game build `24596901`. The log still identifies
    VKD3D-Proton `3dfc6f07d0953b1`, DXVK DXGI, descriptor buffers,
    host-visible upload heaps, and multiple queues, with no D3D11 module,
    device loss, or out-of-memory signature. The update is therefore unchanged
    for the investigated defect.
20. D01a visually reproduces the defect at 6,306 m, but it is not an
    instrumentation result. Its log contains zero `IL2TRACE` markers and the
    four post-run prefix hashes exactly match Proton Experimental rather than
    the diagnostic build. Prefix-only DLL installation is therefore unsuitable
    for stock Proton runtime tests; a dedicated custom Proton tool is required.
21. D01b is the first valid source-instrumented run. Its log contains one
    `IL2TRACE enabled` marker, the VKD3D build identifier carries the local `+`
    suffix, and all four post-run prefix hashes exactly match the diagnostic
    build. Across the corrupted menu and mission it records zero reserved
    resource creations, tiling queries, tile-map updates, or tile copies.
    D3D12 reserved/tiled resources are therefore not used on the failing path.
22. Read-only inspection of the updated `IL2Series.exe` and `dxBackend12.dll`
    supports an ordinary-resource streaming path. The game binary contains
    RTTI/symbol strings for `CBFManagerTiled`, `CBlocksArrayTiled`,
    `CDistantLOD`, and `CTerrainArray`, while the D3D12 backend exposes
    committed/placed resource creation, subresource/whole-resource copies,
    async subresource updates, mip generation, mip-level selection, and
    mip/slice SRV creation. No `CreateReservedResource` symbol was found in the
    focused backend scan. “Tiled” therefore describes the game's higher-level
    terrain organization here; it is not evidence of D3D12 sparse resources.
23. The backend contains explicit diagnostics for insufficient free buffer
    space during both async-thread and main-graphics-thread
    `UpdateSubresource` paths. This makes ordinary upload allocation and
    lifetime a focused lead, but no such assertion has yet been observed in a
    runtime log and no game defect is claimed from a compiled string alone.
24. D02's bounded, opt-in ordinary texture telemetry builds successfully for
    x86-64 and x86 at local VKD3D-Proton commit `54797ad3`. It records texture
    creation/destruction, normalized SRV mip ranges, `CopyTextureRegion`, and
    texture `CopyResource` calls by stable resource cookie without changing
    rendering behavior.
25. The separate `IL2-Korea-D02-Texture-Trace-54797ad3` custom Proton tool was
    created only after Steam exited. Recursive comparison against Proton
    Experimental shows exactly the intended four VKD3D-Proton DLL differences
    after excluding the custom manifest and metadata; every installed hash
    matches the D02 build. Proton Experimental, the D01 tool, game files, and
    prefix remain unchanged by creation.
26. D02-r1 is a valid instrumented run: it contains one `IL2TEX enabled`
    marker, the post-run DLL hashes match the custom tool, and D3D12/DXGI load
    without D3D11. The screenshot shows the same rectangular terrain-page
    failure at 1,385 m. Its timestamp precedes the copy-event cap by about 5.9
    seconds, so the bounded trace covers the visible failure moment.
27. D02 records 3,478 texture creations, 4,185 SRV descriptions, 39,978
    `CopyTextureRegion` calls, and 22 texture `CopyResource` calls. After
    correctly combining Z-slice uploads for Texture3D resources, 2,355
    multi-mip block-compressed resources have complete geometric upload
    coverage and none are partial. All observed SRVs use
    `ResourceMinLODClamp = 0`, and no logged SRV or copy follows destruction.
28. A narrower pre-cap class remains unresolved: 405 placed multi-mip BC3 textures
    have an SRV but no logged buffer upload or incoming texture copy. SRV
    creation alone does not prove shader use, and D02 lacks buffer/heap overlap,
    alias-barrier, descriptor-copy, and draw-use correlation. This class
    justifies D03 instrumentation but not an aliasing workaround.
29. D03-r1 is a valid trace-only run at diagnostic commit `cfca234e`: both
    telemetry gates occur once, all runtime D3D12/D3D12Core hashes match the
    custom tool, D3D12/DXGI load without D3D11, and the user reports the same
    visual corruption. No screenshot was saved for this run.
30. D03 corrects for its copy-event cap rather than treating late resources as
    missing uploads. It finds 585 block-compressed multi-mip textures created
    and exposed through an SRV before suppression with no logged incoming copy;
    every candidate has a matching placed-resource record. Another 952 broad
    candidates first became observable after suppression and are excluded.
31. None of the 585 covered candidates overlaps any traced placed buffer or
    texture range, including ranges reused outside overlapping lifetimes. The
    full run also records zero explicit legacy D3D12 alias barriers. D3D12
    placed-resource memory aliasing is therefore excluded with high confidence
    as the population path for the covered class. Descriptor-heap image/buffer
    type reuse remains a distinct, untested mechanism.
32. E03-r1 definitely disables `VK_EXT_descriptor_buffer` and selects the
    `VK_EXT_mutable_descriptor_type` fallback. Menu aircraft blocks and terrain
    loss at both 4,858 m and 1,121 m remain unchanged. Steam was still using
    the D03-derived `cfca234e` tool, but both diagnostic gates were off and no
    telemetry markers occurred. This is useful negative evidence, but the E03
    row remains inconclusive pending one confirmation on Proton Experimental.
33. The reviewed MSFS 2020/2024 fixes are already ancestors of installed
    VKD3D-Proton `3dfc6f07`. They address shader pipeline creation, shader
    control-flow/value-dominance/ROV translation, or exceptional imported host
    memory. None is an unmerged application override which can be copied to
    IL-2, and IL-2 has no matching pipeline-failure or 16 GiB host-import
    signature in the representative logs.
34. Current upstream VKD3D-Proton `84c87c83` advances `dxil-spirv` from the
    installed `7ecda135` to `cc75a0c9`, a 36-commit range containing several
    control-flow structurizer changes. MSFS 2024 demonstrates that a scene-
    selected translator bug can evade `spirv-val`; this makes an unmodified
    current-upstream D04 run a justified discriminator, not a claimed fix.
35. D04 is visually unchanged at the menu and at 6,400 m. All four post-run
    prefix DLL hashes match unmodified current VKD3D-Proton `84c87c83`, so the
    selected source definitely ran. No Proton log was created, but valid DLL
    provenance and the screenshots are sufficient to close the broad
    current-upstream version lead without a repeat.
36. The game-owned D04 `tex.log` records 18 provider failures. Six are exact
    Korea winter terrain inputs and six are `GLASS_MAIN.DDS`. Static inspection
    of `dxBackend12.dll` shows that the message follows a failed provider call
    and the path then requests `graphics\textures\defWhite.bmp`. These are real
    fallback substitutions, not merely attempted-path diagnostics.
37. `Maps6.gtp` contains configuration references to all six failed Korea
    winter paths. D04's `packman.log` reports all map packages enumerated, a
    45,100-node package tree, 11,575 package-opened files, and no package-open
    or decode error. The precise lookup/decode/create failure stage remains
    unisolated, and the failures are not causal until compared with Windows.
38. The game binary's terrain module names an application-managed cache path:
    `BlocksCache`, `BakedTerrainCache`, `BakedTerrain`, `g_tTiles`,
    `stitchBorders`, and multiple distant-LOD selectors. D01b's zero sparse-API
    result and D02's ordinary-copy trace agree with this architecture.
39. D02 creates 164 placed 2048x2048, one-mip BC3 textures at mission setup.
    Fourteen receive 382 invalid border regions before the screenshot; the
    class reaches 432 regions across sixteen members before the trace cap. The
    affected members also receive observed 64x64 or 128x128 interior pages.
40. The same pool receives exactly 432 buffer-to-image regions with internal
    dimensions incompatible with BC3's 4x4 physical blocks: 118 `128x1`, 112
    `1x128`, 110 `64x1`, and 92 `1x64`. All offsets are block-aligned, but the
    one-texel extent neither forms a block nor reaches the mip edge.
41. Current VKD3D-Proton passes the D3D12 placed-footprint dimensions directly
    through `vk_buffer_image_copy_from_d3d12()` to
    `vkCmdCopyBufferToImage2()`. It does not convert buffer layout or extent
    units when the footprint and image formats use different block geometry.
    The code is unchanged in tested upstream commit `84c87c83`, explaining why
    D04 could not fix this path.
42. Vulkan expresses compressed buffer-image regions and buffer strides in
    destination image texels. The D3D12 placed footprint instead describes the
    buffer in its own format's texels. For equal 16-byte physical elements,
    VKD3D-Proton must convert through block counts, as its image-to-image path
    already does. Public D3D12 documentation does not clearly enumerate the
    BC3/RGBA32_UINT pair, so this is best described as a native-compatibility
    gap rather than an assertion that the game's use is specification-required.
43. D05c is a valid behavioral run. It adjusted 202/202 exact thin
    `R32G32B32A32_UINT`-to-BC3 candidates, rejected none, and touched eight
    destination resources. The user observed the same missing terrain pages
    and magenta edges. A border-only correction is therefore excluded; the run
    did not include the square page interiors later identified by D06.
44. Read-only extraction of Maps1-6 confirms the Korea terrain system defines
    five LODs and `textureQuadSize=800`. It contains ordinary mesh, surface,
    distant-LOD, and hundreds of DDS resources. This independently corroborates
    application-managed baked pages and explains the rectangular artifact
    geometry.
45. The six tested autumn terrain paths in `tex.log` are absent from installed
    packages and loose files. Nearly the same absent-reference set occurs in
    every season configuration. Because the same Windows build is reported to
    render correctly on Windows, these are proven fallbacks but not yet a
    Proton-specific cause.
46. Wine's `BitmapScaler_Initialize unsupported mode 3` warnings do not indicate
    an image-load abort: current Wine falls back to nearest-neighbour scaling,
    initializes the source, and returns success. The owning log context points
    to the Noesis UI path rather than a demonstrated terrain-provider failure.
47. D06 is a valid trace-only run and is visually unchanged as expected. Its
    six active BC3 cache resources receive 292 `64x64`, 354 `64x1`, and 354
    `1x64` buffer-to-image copies.
48. The D06 square interiors are placed on a 256-texel grid while VKD3D emits
    only 64x64 regions. Border offsets such as 252, 508, and 764 align with the
    same page ends. Current union coverage is only 3.46-6.32% per active cache;
    converting each 128-bit source element to one 4x4 BC3 block projects to
    54.69-100% coverage.
49. D06 does not log the placed-footprint format for square interiors, so the
    1:4 interpretation is not yet proven for them. D07 combines exact format
    telemetry with a tightly gated full-page behavioral conversion, avoiding a
    separate additional trace run.
50. D07 is valid. It records 522 target candidates, 522 adjustments, zero
    rejects, zero cap markers, and four destination resources. Every source is
    a footprint-only `R32G32B32A32_UINT` region and every destination is BC3.
51. D07 converts 178 `64x64` interiors to `256x256`, 182 `64x1` borders to
    `256x4`, and 162 `1x64` borders to `4x256`. The screenshots at 5,491 m and
    5,501 m show continuous detailed terrain without the former rectangular
    holes or magenta page edges.
52. The successful D07 run still contains 40,408 split `END_ONLY` warnings and
    the game's `tex.log` still contains Korea summer/common-fallback failures.
    Neither is required for the rectangular terrain defect.
53. A general VKD3D-Proton fix at commit `cf11ba76` converts buffer-image
    geometry through physical blocks whenever the source and destination block
    sizes match. It has no executable check, AppID check, or IL-2 override.
54. The focused regression test fails four assertions on the old helper and
    passes all 22 assertions with `cf11ba76`. Existing
    `test_copy_texture_bc_rgba` (147 assertions) and
    `test_copy_block_compressed_texture` (50 assertions) also pass.
55. Before formal PR review, candidate `64ec55e7` narrows activation to
    equal-sized physical elements whose block width or height differs. Copies
    with matching block geometry and copies with unequal physical sizes now
    stay exactly on the old path. IL-2's 16-byte 1x1-to-4x4 mapping still
    selects unchanged conversion arithmetic.
56. The narrowed candidate builds natively and with MinGW x64. Its focused
    test passes 22/22 assertions and the full native copy subset passes
    6,429,713 checks with zero failures.
57. The game's `libiomp5md.dll` imports
    `GetNumaHighestNodeNumber` and `GetNumaNodeProcessorMaskEx`. Proton 11's
    Wine returns success with node zero from the former semi-stub but returns
    `ERROR_CALL_NOT_IMPLEMENTED` from the latter stub. With no OpenMP override,
    the exact DLL consequently aborts with Intel OpenMP Error #179 naming
    `GetNumaNodeProcessorMaskEx`.
58. In an isolated four-way environment matrix, `KMP_AFFINITY=disabled`
    succeeds without `OMP_NUM_THREADS`, while `OMP_NUM_THREADS=16` without the
    affinity bypass still fails. The thread-count setting is therefore not the
    cause of successful OpenMP initialization on this host.
59. Wine already exposes node zero, group zero, and the runtime-discovered
    mask `0xffff` through `GetLogicalProcessorInformationEx(RelationNumaNode)`.
    A focused kernelbase candidate obtains the requested node from that same
    canonical topology instead of encoding a CPU count, mask, vendor, game, or
    AppID.
60. Substituting only the candidate `kernelbase.dll` into a disposable copy of
    the exact installed Proton 11 family makes the exact game OpenMP DLL
    initialize with no `KMP_AFFINITY` or `OMP_NUM_THREADS` setting. The probe
    reports 16 processors because that is this host's discovered topology, not
    because 16 appears in the implementation.
61. A complete D09 compatibility tool containing both architecture variants of
    the candidate was selected for AppID 247970. With Steam launch options
    empty, Steam invoked D09, `IL2Series.exe` remained active through startup,
    the game initialized its GUI, and the user confirmed that the full game
    started. This resolves the launch-parameter requirement on the reporting
    host; it does not yet validate other CPU or NUMA layouts.
62. Wine MR !11604 independently implements the wider NUMA API set in ntdll,
    kernelbase, and kernel32. Its exact six-commit head
    `e8319c0e6bfe7f94512218b48e3158e0c286b481` applies cleanly to Proton 11's
    pinned Wine commit. In a fresh prefix, the resulting D10 package initializes
    the exact game OpenMP DLL without any OpenMP or topology override.
63. With AppID 247970 mapped to D10 and Steam launch options empty, the full
    game maps all four patched 64-bit Wine modules plus `libiomp5md.dll`, passes
    the former abort point, and writes fresh GUI/game state. The live process
    has no `KMP_*`, `OMP_*`, or `WINE_CPU_TOPOLOGY` setting. This confirms the
    existing upstream series fixes startup on the reporting host; physical
    multi-node and sparse-node layouts remain unvalidated.
64. The user visually confirmed that the shimmering squares remain in the D10
    run. D10 changes the Wine CPU/NUMA path and fixes startup without changing
    that artifact; D07/D08 changed the VKD3D copy path and fixed terrain while
    also leaving it unchanged. The shimmering is therefore independent of the
    NUMA/OpenMP startup defect and the resolved terrain-copy defect. Its own
    cause remains open.
65. Read-only inspection of `dxBackend12.dll` shows exported D3D12 VRS controls,
    including `enableVRS`, `setVRSRate`, `setVRSCombiners`, and
    `getVRSFeatures`. VKD3D-Proton maps that path through
    `VK_KHR_fragment_shading_rate` and stops advertising D3D12 VRS when that
    extension is disabled. This makes E05 a valid capability A/B test, but the
    symbols alone do not prove that the menu enables VRS.
66. The game renderer also names current and previous screen-space reflection
    targets (`rtSSR`, `rtSSRPrev`, and `g_tPrevReflections`) and distinct screen,
    overlay, cockpit, and accumulated-reflection passes. Because the visible
    blocks move over reflective metal while the base aircraft texture remains
    present, a temporal reflection resource is the leading alternative if E05
    is unchanged. This remains a hypothesis until runtime pass/resource use is
    correlated.
67. E05 ran on updated game build `24615759` with exact runtime VKD3D build
    `cf11ba76`, no OpenMP/topology override, and 228 explicit warnings confirming
    that `VK_KHR_fragment_shading_rate` was disabled. The user confirmed that
    the same main-menu shimmering squares remain. No device-loss, OOM, or GPU
    reset signature was found. The artifact therefore does not require the VRS
    path, and the temporal/reflection lead now ranks above G15.
68. D11 is visually unchanged and records 3,145 application-supplied resource
    names with no trace suppression. The game allocates `m_prtTargetReflections`,
    current and previous full-resolution SSR color/weight targets, and
    `rtTempSSR` while constructing the affected menu. It makes no PIX marker or
    begin-event calls. Runtime allocation is now confirmed, but binding/use and
    causality remain unproven.
69. D12 records 100,000 focused usage events over about 2,003 repeated
    reflection cycles. The current/previous SSR targets are bound, cleared,
    rendered with stable shader hashes, resolved, and transitioned through
    coherent render-target/shader-resource/resolve states. Every logged
    transition on this named family has flags `0`; no tracked UAV, alias, or
    copy event occurs. This weakens a simple missing or split transition on the
    named SSR targets without clearing reflection shaders or descriptors.
70. D12's usage cap was reached around 21:12:49 local time and its broad name
    cap around 21:18:12. The cockpit and exterior/fire screenshots were written
    at 21:35:31 and 21:35:54. They validly show the same artifact outside the
    menu, but their rendering operations are not inside D12's bounded usage
    window.
71. D12 names multiple `rtLightRefs*` generations as `80x34x2`, one-mip,
    `DXGI_FORMAT_R32_UINT` resources with render-target and UAV flags. At
    2560x1080, this maps the screen into approximately 32x32-pixel tiles.
    `enviro.dll` and `renderers.dll` independently name `rtLightRefs`,
    `g_tLightRefsRW`, `g_tLightsListRW`, light-list collection/draw, self-light,
    and light-volume paths. Together with the cockpit/fire reproduction, this
    promotes tiled dynamic lighting to the strongest current lead. It remains
    correlation until D13 identifies the actual clear/write/read sequence.
72. D13 is visually unchanged and records the complete 200,000-event budget
    over about 33.4 seconds of initial menu rendering. The final
    `rtLightRefs27` generation completes more than 1,500 stable cycles. All
    tracked transitions have flags `0`; explicit resource UAV barriers separate
    its compute stages, and `rtSelfLight10` is zero-cleared on every frame.
    This weakens an omitted clear or simple state/UAV dependency as the cause.
73. Four stable compute hashes are light-grid producer candidates:
    `ce5553a11c1e3c3d`, `e41c75bf472dc42b`, `14096b77d9f7cb60`, and
    `651194bd0a21772e`. The first two nearby draw candidates after self-light
    becomes readable use pixel shaders `df0bd777fd1bb89d` and
    `a2d104d5c813322e`, which D12 records writing
    `m_prtTargetReflections`. This is strong frame-order correlation between
    self-light and reflection rendering, but D13 does not resolve descriptor
    tables and therefore does not claim an exact texture read.
74. The D13 screenshot sequence retains the moving square blocks; its final
    frame captures them across the aircraft, its shadow, and the lit floor.
    The visual alignment supports a light/reflection interaction but does not
    distinguish a bad light-list value from a bad view, descriptor, shader
    translation, or reflection consumer.
75. D14 captures all target DXIL/SPIR-V pairs and identifies six exact
    light-list compute stages. D13's bounded dispatch window missed
    `ComputeLightsFirstRef` (`7cefa1bc80bb4c70`) and
    `ComputeLightsIndices` (`11e32439a86036ba`). The latter writes a separate
    uint light-index buffer after the count stage.
76. Pixel shaders `PixOutLight_msp` (`df0bd777fd1bb89d`) and
    `PixOutLight_mss` (`a2d104d5c813322e`) statically read both a 3D uint
    light-reference SRV and that separate index buffer. They derive a tile
    coordinate from screen position and unpack count/start bits from the grid,
    so incorrect data can affect one complete approximately 32x32 screen tile.
    Reflection is therefore involved as an output pass, but tiled-light data is
    the narrower causal lead.
77. The relevant translated SPIR-V modules validate and preserve the DXIL
    resource dimensions, `8x8` group sizes, bounds checks, integer packing, and
    atomics. This weakens an obvious structural translator mismatch without
    proving descriptor correctness, computed values, or runtime visibility.
78. D13's named-image transition does not resolve whether the separate
    light-index buffer receives an adequate dependency after
    `ComputeLightsIndices`. D15 commit `9c6a4338` passively logs the final
    dispatches and all immediately intervening barriers before any forced
    barrier A/B is considered.
79. D15 is visually unchanged and captures 1,593 occurrences each of
    `ComputeLightsFirstRef` and `ComputeLightsIndices` over approximately 29.4
    seconds. The game remained stable, and the log reports no device loss,
    OOM, GPU reset, or hang.
80. Every covered final-generation sequence gives both `rtLightRefs25`
    (cookie 4002) and the separate 87,040-byte UAV buffer (cookie 4001) an
    inter-stage UAV dependency and a final UAV-to-shader-read transition. The
    D16 later resolves the buffer as 43,520 `R16_UINT` elements, so its size
    equals `80 * 34 * 16 * sizeof(uint16_t)`, matching sixteen light indices
    per screen tile.
81. Translated SPIR-V for both final producers uses Vulkan Device scope for
    its atomic increments. Relaxed atomic memory semantics are consistent with
    D3D interlocked arithmetic; the explicit D3D12 UAV barriers provide the
    required inter-dispatch dependencies.
82. D15 therefore excludes the missing-synchronization form of G17 for this
    sequence. A forced pre-compute/global barrier or IL-2 application override
    would duplicate synchronization already requested by the game and is not
    a valid fix candidate.
83. D14 reflection fixes the affected pixel-shader inputs at SRV `t9`
    (`g_tLightsList`) and `t10` (`g_bufLightsIndices`). The next passive test
    can resolve those exact descriptor-table entries to runtime resources and
    view metadata before escalating to a GPU value capture.
84. D16 commit `274f6f8e` resolves all 13,236 covered fixed-slot lookups with
    zero failures. Each shader has 3,309 stable `t9` and 3,309 stable `t10`
    events. `t9` is the expected `rtLightRefs25` cookie 4002, viewed as a
    `80x34x2 R32_UINT` Texture3D; `t10` is the expected cookie-4001 buffer,
    viewed as 43,520 `R16_UINT` elements. Root signature, table base, heap
    offsets, descriptor serials, types, resource cookies, and view shapes do
    not vary.
85. The D16 artifact remains visible. Wrong descriptor selection,
    propagation, type, and view shape for the two tiled-light inputs are
    excluded for the covered draws. The trace changes no GPU command and
    reports no device loss, OOM, reset, or hang. The post-run configuration
    does not verify the user's anti-aliasing/HDR UI A/B, so that observation is
    retained as reported rather than promoted to a setting exclusion.
86. A read-only upstream refresh finds no relevant newer translator fix.
    VKD3D-Proton master remains the D04 baseline `84c87c83`; the four newer
    `dxil-spirv` commits only affect NVIDIA/SM 6.9 ray-tracing local-root-table
    constant loads. The next one-variable discriminator is RADV `fullsync`,
    followed by produced-value or typed-buffer code-generation inspection if
    unchanged.

## Observations not yet promoted to findings

- Split `END_ONLY` barrier warnings span most of E00-r1 and issue #3134, but
  their temporal/resource relationship to corruption is unknown.
- The game creates `BlocksCache` CPU threads on mission entry. The ordinary log
  does not connect them to a D3D12 resource or the visible missing pages.
- During D04 the mission loading display advanced from 25% to 26% and then
  entered flight. Package configuration and runtime timing show that baked-page
  cache activity continues after mission entry, so this percentage is not
  evidence that loading aborted. It may be phase-local.
- Screenshot evidence shows strong altitude/distance dependence across several
  configurations. The user reports that additional low-fidelity assets begin
  loading below roughly 1,500 m, while captures near 5,000 m show substantially
  greater loss. D02 also proves that rectangular pages remain missing at 1,385
  m, so the threshold changes severity rather than fixing the defect. The
  mechanism remains unproven.
- The user visually reconfirmed the shimmering squares during the successful
  D10 NUMA run. They remain after both the terrain-copy and startup fixes and
  therefore remain a separate open graphics track.

## External-report and prior-art assessment

Issue #3134 is a direct report of this defect and will be the VKD3D-Proton
handoff target. It has no maintainer diagnosis or resolution yet. Among
resolved *other-game* examples, Wuthering Waves had flickering broken textures
fixed by a shader-specific barrier quirk after identifying a missing UAV
dependency. Satisfactory also has a targeted missing-barrier workaround. Arma
Reforger's unusual asset loading motivated a `no_upload_hvv` workaround. These
cases increase the diagnostic value of E01, E02, and later resource/shader
tracing; they do not justify copying an override.

MSFS 2024 supplies a different and now higher-value precedent: grass or
near-ground rendering selected a `dxil-spirv` bug which `spirv-val` did not
catch. MSFS 2020's `host_import_fallback` advice instead belongs to an
exceptional 16 GiB `OpenExistingHeapFromAddress` path and is not transferable
without IL-2 host-import evidence. D04 tested the newer translator and is
unchanged, so no MSFS-derived fix path remains selected. See
`prior-art-msfs.md` and `evidence-d04-upstream-result.md`.

## Root-cause assessment

| Track | Assessment | Confidence |
|---|---|---|
| Startup | Wine's missing NUMA API behavior makes the shipped Intel OpenMP affinity initialization abort. A local focused candidate and exact upstream MR !11604 both restore the topology contract and start the game with no launch workaround. | High on the reporting host; cross-topology validation pending |
| Graphics | The game uses D3D12 through VKD3D-Proton and DXVK's DXGI. Two D07 runs and clean general-build D08 prove that VKD3D-Proton under-populates the application's 800 m baked-terrain pages by retaining source-footprint units in a BC3 Vulkan copy. | High; causal for terrain and addressed by narrowed PR candidate `64ec55e7` |
| Queue selection | Two `single_queue` runs leave the defects unchanged, making ordinary asynchronous compute/transfer queue selection unlikely to be the primary trigger. | Medium |
| Upload allocation | `no_upload_hvv` changes the allocation path, but its apparent improvement is inseparable from lower capture altitude. | Low/inconclusive |
| Streaming/residency/LOD | D02 confirms active ordinary compressed-texture streaming and narrows it to complete uploads plus an unresolved no-upload SRV class; it does not identify which resources produce the visible pages. | Medium for relevance, low for mechanism |
| D3D12 sparse/tiled resources | Valid D01b instrumentation records zero reserved-resource and tile-mapping API use during reproduction. | Excluded for this path, high |
| Ordinary mip uploads | D02 geometrically verifies complete buffer uploads for 2,355 multi-mip compressed resources, zero partial resources, and zero non-zero SRV minimum-LOD clamps. | Broad incomplete-mip/min-LOD explanation weakened, medium-high |
| No-upload SRV class | Corrected D02 analysis finds 405 pre-copy-cap placed multi-mip BC3 textures with SRVs but no logged incoming upload/copy; actual binding/use is unknown. | Medium for relevance, low for defect/cause |
| Placed-resource aliasing | D03 matches all 585 same-run pre-cap candidates and finds zero range overlap plus zero explicit legacy alias barriers. | Excluded for covered class, high |
| Descriptor-buffer backend | One verified disable run on the D03-derived build is visually unchanged and uses the mutable-descriptor fallback; stock-Proton confirmation remains. | Medium that descriptor buffers are not the primary cause |
| Current upstream | D04 with unmodified VKD3D-Proton `84c87c83` is visually unchanged and all four runtime hashes match. | Excluded as an existing broad version fix, high |
| Game texture-provider failure | Six exact Korea autumn terrain inputs fail both requested and common fallback lookup and default to white. Package inspection proves the references absent, but a nearly identical absent set occurs in every season. | High that fallbacks occur; low-medium that they cause the Linux corruption |
| BC3 baked-terrain cache copies | D07 adjusts 522/522 complete-page and border copies with zero rejects and repairs terrain near 5,500 m. D07-r2 repeats the repair. Clean general-build D08 repairs terrain at 4,813 m, 2,427 m, and 742 m without a diagnostic gate. The general regression fails on the old helper and passes with D08 predecessor `cf11ba76` and narrowed PR candidate `64ec55e7`. | Root cause and general remedy validated |
| Menu/cockpit square artifact | Persists after the terrain-copy and Wine-NUMA fixes and after E05 removes advertised VRS support. D14 identifies a six-stage tiled-light producer. D15 proves both outputs receive explicit dependencies/read transitions. D16 resolves every covered `t9`/`t10` consumer binding to the exact expected resources and `R32_UINT`/`R16_UINT` views. | Cause open; VRS, reflection-target transitions, obvious structural translation errors, missing final-producer synchronization, and wrong descriptors/views for the two principal tiled inputs are weakened or excluded. Test RADV full cache synchronization once, then inspect produced values/code generation. |

No application override is justified. D08 validates general predecessor `cf11ba76`;
current PR candidate `64ec55e7` preserves its IL-2 conversion while leaving
same-block-geometry copies on the old path. The terrain track is complete; the
menu aircraft blocks/shimmering remain a separate open track.

## Source-level investigation gate

The broad launch-option batch ended after E02 because two baseline runs, two
upload-path runs, and two single-queue runs all retained the core defect. E03
was later run once and remained visually unchanged with the extension disable
verified; E04 was not run. The investigation has resumed at the
development-build stage.

1. D00 and D01a prefix-copy controls are invalid because Proton replaced the
   local DLLs before the game loaded them.
2. U00 is complete: the updated game build is unchanged for the defect.
3. D01b is complete and excludes D3D12 reserved/tiled resources from the
   failing path.
4. D02 is complete. It weakens broad missing-mip and minimum-LOD explanations
   and identifies 405 pre-cap SRV-bearing compressed textures without a logged upload.
5. D03 is complete and excludes placed-resource range aliasing for all 585
   same-run pre-cap candidates. It does not test descriptor-heap type reuse.
6. E03-r1 disables the active descriptor-buffer extension and is visually
   unchanged on the D03-derived tool with its telemetry gates off. A stock
   confirmation remains desirable but is not the next source-level gate.
7. D04 is complete and unchanged on unmodified current VKD3D-Proton
   `84c87c83`; do not repeat it or bisect the source range.
8. Re-analysis of D02 finds 432 thin `R32G32B32A32_UINT` to BC3
   reinterpret-copy regions in the active baked-terrain cache. D05a's binary
   and enable marker are valid, but its zero adjustments invalidate the visual
   comparison.
9. D05b is complete. It recorded exactly 432 candidates and rejected all 432;
   every source is a footprint-only `R32G32B32A32_UINT` resource and every
   destination is BC3. Its unchanged image is not a causal negative because it
   changed no Vulkan command.
10. D05c is complete. It applies the documented 1:4 reinterpret mapping to
    202/202 thin candidates with zero rejects and the visible defect is
    unchanged, excluding borders alone.
11. Package inspection confirms five LODs, 800 m texture quads, and the
    installed absence of the logged fallback inputs. The 26% transition is not
    evidence of aborted loading because cache activity continues after entry.
12. D06 is complete. It exposes square interiors emitted at one quarter of the
    page spacing in each dimension and narrows the leading cause to a full-page
    block-unit conversion.
13. D07 is complete and repairs the high-altitude terrain while adjusting all
    522 observed page-family copies with zero rejects.
14. D08 validates the upstream-oriented `cf11ba76` change in game without the
    diagnostic gate. Terrain is fixed; menu blocks/shimmering are unchanged.
15. PR candidate `64ec55e7` narrows the activation predicate without changing
    the conversion selected for IL-2; same-block-geometry copies retain the
    original path.
16. The NUMA caller is resolved on the reporting host by exact Wine MR !11604.
    E05 disables only advertised fragment shading rate on D10 and is visually
    unchanged. D11-D14 narrow the remaining defect to the tiled-light grid and
    separate light-index buffer consumed by the reflection/light passes.
    Structural shader translation checks pass. D15 proves that both final
    resources receive explicit UAV dependencies and shader-read transitions,
    so no barrier quirk is justified. D16 resolves all fixed `t9`/`t10`
    descriptors to the expected resources and view shapes. Run RADV `fullsync`
    once as a cache/synchronization discriminator, then inspect produced values
    or typed-buffer code generation if unchanged; do not propose an
    application workaround without a causal discriminator.
