# Tiled-light artifact: ownership and upstream plan

Date: 2026-08-10

## Final technical conclusion

The main-menu, cockpit, and fire-lit square blocks and broad light flicker are
caused by one malformed tiled-light allocator shader. IL-2 performs a 32-bit
global atomic through an `R16_UINT` typed UAV. Translating that literally as a
typed Vulkan texel-buffer access does not provide a legal equivalent.

Three consecutive affected frames prove the visible mechanism:

- all 50 compute workgroups independently allocate from zero;
- 12,126 requested light references collapse into offsets 0–320;
- depth and tile metadata remain bit-identical;
- 69–107 overwritten light IDs change between adjacent frames.

This produces stable screen-tile boundaries with unstable light membership,
which explains both the square blocks and their broad flicker.

## Minimal compatibility fix

VKD3D-Proton already emits a raw storage-buffer descriptor sibling for the D3D
descriptor. For executable `IL2Series.exe` and exact shader hash
`7cefa1bc80bb4c70`, the fix must do both:

1. lower the typed UAV access as an SSBO operation;
2. select `VKD3D_SHADER_BINDING_FLAG_RAW_SSBO` so that operation addresses the
   raw descriptor rather than the typed texel-buffer binding.

D25 implemented only the first half and remained defective. D44 captured that
incomplete translation selecting the typed descriptor set. D45 implemented
both halves and was clean on two independent starts, but also contained an
earlier diagnostic depth-gate bypass. D46 attempted to remove that bypass but
accidentally removed the executable mapping too, making its result invalid.
D47 restored the mapping and retained only the allocator correction. D47 is
clean with the original depth predicates, lighting, and shadows.

Therefore the depth-gate bypass is not part of the final fix. It only hid how
the malformed light list was presented.

## Ownership

Principally, the game should not perform a 32-bit atomic through a 16-bit typed
view. Correct API use remains preferable and the upstream report should say so
plainly.

Practically, compatibility layers routinely emulate narrowly demonstrated
native-driver allowances for shipped games. This case meets that bar:

- native Windows renders correctly;
- legal Vulkan atomic variants pass on both tested RADV GPUs;
- the live `R16_UINT` variant reproduces the malformed allocation;
- an SSBO control repairs the standalone allocation;
- the correctly wired game integration repairs the actual pixels;
- the behavior is scoped to one executable and one shader hash.

The proper practical owner is therefore VKD3D-Proton as a surgical
native-compatibility quirk. This is not a Mesa workaround, Proton launch
parameter, game mod, or custom lighting engine.

## Separate IL-2 tracks

| Track | Mechanism | Upstream path |
| --- | --- | --- |
| Startup without parameters | Wine lacked `GetNumaNodeProcessorMaskEx`; the shipped Intel OpenMP runtime aborts | Existing Wine MR !11604; validated locally without a hard-coded thread count |
| Distant terrain pages | Buffer-to-BC3 copy geometry used source texel units instead of destination block geometry | Existing VKD3D-Proton PR #3202 |
| Square blocks and broad light flicker | Invalid typed-UAV atomic selected the wrong Vulkan descriptor class and corrupted the tiled-light list | Update existing VKD3D-Proton issue #3134, then one narrow PR |

The fine sandy or film-grain lighting is present on native Windows and is not a
Proton defect.

## Publication sequence

1. Keep terrain PR #3202 separate and allow its review to proceed.
2. Add one concise follow-up to existing VKD3D-Proton issue #3134, whose
   original report already mentions the menu squares, and note that #3202 fixes
   only terrain.
3. Include compact evidence and hashes; do not upload RenderDoc captures,
   shader binaries, game assets, prefixes, or giant unfiltered logs.
4. Allow a reasonable interval for maintainer feedback.
5. Rebase the one-commit allocator fix on then-current VKD3D-Proton master,
   rebuild, and open one PR referencing #3134.
6. After the PR exists, prepare short updates for Proton #9906, the original
   affected users, and this investigation repository.

## Upstream scope gate

The proposed code may contain only:

- one new typed-UAV-as-SSBO shader quirk;
- raw descriptor-sibling selection under that quirk;
- the `IL2Series.exe` entry and allocator shader hash.

It must not contain:

- either depth-producer hash or the D38 SPIR-V rewrite;
- a processor/thread-count constant;
- a launch option, environment-variable requirement, game-file patch, or Mesa
  workaround;
- captured shaders or RenderDoc files.

The issue-comment and PR drafts live locally for review.
