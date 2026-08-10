# Tiled-light upstream drafts

Prepared locally for review and the staged publication sequence below.

## Proposed update to existing issue #3134

This belongs in #3134 because its original report already mentions the menu
squares and our previous comment says that symptom remained unresolved. No new
issue is needed.

Suggested comment:

```text
Follow-up on the menu squares mentioned in the original report: this separate
lighting problem is now isolated and fixed locally. PR #3202 remains only the
terrain-page fix.

In simple terms, the game asks a 16-bit typed buffer to perform a 32-bit atomic
allocation. Windows drivers tolerate that mismatch. VKD3D-Proton translated it
literally, so every tiled-light workgroup started writing at the beginning of
the same small list. Different lights then overwrote each other. That is why
the corruption appeared as stable square blocks but flickered as lighting
changed.

Three consecutive affected-frame captures confirm the mechanism: all 50
workgroups allocate from zero, 12,126 requested light references collapse into
offsets 0–320, and 69–107 overwritten light IDs change between adjacent frames
while depth/tile metadata stays identical.

VKD3D-Proton already creates a raw storage-buffer version of this descriptor.
For `IL2Series.exe` and exact shader `7cefa1bc80bb4c70`, selecting that raw
descriptor and lowering the access as an SSBO restores one correct global
allocation. The allocator-only build removes the blocks and broad flicker with
empty launch options while retaining the original depth predicates, real
lighting, and shadows. The fine sandy/film-grain lighting is also present on
Windows and is not part of this defect.

Principally, the game should bind a view compatible with the atomic width. In
practice, a compatibility layer sometimes needs a narrow allowance for
behavior accepted by native Windows drivers. The candidate is therefore a
surgical quirk for one executable and one shader hash, not a general renderer
change, launch workaround, game mod, or Mesa workaround.

I have a local 29-line candidate based directly on current VKD3D-Proton master
and will prepare it as a separate PR after feedback here.

Environment used for the final A/B:
- GPU: Radeon RX 7800 XT
- Driver: Mesa/RADV 26.1.6
- CPU: Ryzen 7 7800X3D
- Proton/Wine base: Proton Experimental 11 with Wine MR !11604 backported
- Launch options: empty

Investigation notes: https://github.com/silv3rshi3ld/il2-korea-proton-investigation
```

## Proposed pull request

Suggested title:

```text
vkd3d-shader: Work around IL-2 tiled-light allocator
```

Suggested body:

```text
Addresses the remaining tiled-light artifact reported in #3134. The terrain
corruption from the same issue is handled separately by #3202.

## What changed

Add a shader quirk which lowers a typed buffer UAV as an SSBO and explicitly
selects VKD3D-Proton's raw descriptor sibling. Apply it only to
`IL2Series.exe` and shader hash `7cefa1bc80bb4c70`.

## Why

IL-2's tiled-light reference allocator performs a 32-bit atomic through an
`R16_UINT` typed UAV. Native D3D12 drivers tolerate that invalid combination,
but a literal Vulkan typed-buffer translation makes all 50 workgroups reuse
the same short allocation prefix.

Three consecutive affected frames request 12,126 entries but address only
offsets 0–320, with 69–107 overwritten light IDs changing per adjacent frame.
The malformed list produces the visible tile-shaped blocks and broad light
flicker.

VKD3D-Proton already emits a raw storage-buffer sibling for the descriptor.
The quirk changes both required halves of the translation: SSBO access and raw
descriptor selection. Without the second half, the generated StorageBuffer
still addresses the typed descriptor set and the game remains defective.

## Validation

- Exact shader with legal `R32_UINT`: correct global allocation on RX 7800 XT
  and Raphael iGPU, through descriptor-buffer and mutable-descriptor paths.
- Exact shader with live `R16_UINT`: deterministic overlapping allocation on
  both GPUs/backends.
- Raw SSBO control: correct global allocation on both GPUs/backends.
- Integrated build with access-class change only: unchanged artifact.
- Integrated build with SSBO access plus raw descriptor selection: blocks and
  broad flicker gone on two independent starts.
- Allocator-only minimality build with original depth predicates: clean with
  empty launch options; real lighting and shadows retained.
- x64 and x86 MinGW package builds complete without new compiler warnings.

## Visual comparison

| Before: stock typed-buffer path | After: allocator-only quirk |
| --- | --- |
| BEFORE_IMAGE_URL | AFTER_IMAGE_URL |

The before image shows the large light-aligned blocks on the menu aircraft and
hangar floor. The after image must come from a provenance-verified D47 or clean
candidate run; an older temporarily clean diagnostic screenshot must not be
substituted.

The fine sandy or film-grain lighting remains and is also present on native
Windows, so it is not treated as a regression.

## Scope

The game should ideally use a view compatible with its atomic width. This
patch emulates native-driver tolerance as narrowly as possible: one executable
and one exact shader hash. It contains no AppID-wide renderer change, depth
culling override, launch option, game-file modification, or Mesa workaround.
```

## Publication order

1. Review and post the follow-up comment to existing issue #3134.
2. Allow an interval for maintainer response and for terrain PR #3202 review.
3. Recheck current master and rebase the local one-commit branch if necessary.
4. Rebuild both architectures and update hashes.
5. Incorporate applicable maintainer feedback, then push the branch and open
   the PR as a separate later step.
6. Update Proton #9906, the community, and the investigation repository
   only when the corresponding handoff is ready.

Never upload captured shaders, RenderDoc files, game assets, prefixes, Steam
configuration, credentials, or unfiltered giant logs.
