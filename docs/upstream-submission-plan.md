# Upstream submission plan

> [!NOTE]
> Historical completed plan. The terrain branch was published as
> [VKD3D-Proton PR #3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202).
> The separate startup and tiled-light outcomes were later published through
> Wine MR !11604 and VKD3D-Proton PR #3207. Current status is maintained in
> [`final-report.md`](final-report.md).

The user approved publication on 2026-08-07. The investigation repository is
public, VKD3D-Proton PR
[#3202](https://github.com/HansKristian-Work/vkd3d-proton/pull/3202) is open,
and updates were posted to VKD3D-Proton #3134 and Proton #9906.

## Correct repository

The code change belongs in **VKD3D-Proton**, because the demonstrated defect is
in the conversion of a D3D12 placed buffer footprint to Vulkan buffer-to-image
copy geometry. It should not be submitted as a duplicate patch to Valve
Proton. Proton consumes VKD3D-Proton as a component and will receive an accepted
fix through a later component update.

Recommended sequence:

1. Review the local commit, patch, test evidence, and selected screenshots.
2. Fork `HansKristian-Work/vkd3d-proton` under the user's GitHub account.
3. Push local branch `fix-buffer-image-block-units`, based on upstream commit
   `84c87c8390d9df75ba41d911496296fe13f0e275`, to that fork.
4. Open a VKD3D-Proton pull request referencing issue #3134 and Steam AppID
   247970.
5. After the pull request exists, add a short update to ValveSoftware/Proton
   #9906 linking to it. Do not duplicate the implementation in Proton.
6. Respond to review and retest any revised commit before it is considered
   final.

## Why propose a general fix first

VKD3D-Proton already converts compressed/uncompressed **image-to-image** copy
geometry through physical blocks and tests `R32G32B32A32_UINT` to/from BC3.
The candidate supplies the corresponding conversion for the demonstrated
placed-buffer-to-image upload direction. The defect is in a shared translation
helper, not in an IL-2 configuration flag, and the candidate contains no
executable name, AppID, resource dimensions, or environment-variable gate.

This does not mean the risk is zero. Public D3D12 documentation describes
texture-copy formats as identical or from a compatible type group; it does not
clearly define the game's `R32G32B32A32_UINT` to `BC3_UNORM` combination. Native
Windows nevertheless accepts the pattern, and VKD3D-Proton already emulates
the corresponding image-to-image reinterpretation. The pull request should
state this compatibility context without claiming that the game call is
unambiguously valid D3D12.

## Impact boundary and regression risk

Current candidate `64ec55e7` changes only placed-buffer-to-image copies for
which the source and destination physical elements have the same byte size
**and** their block width or height differs. The IL-2 case is 16 bytes on each
side: one uncompressed `R32G32B32A32_UINT` texel maps to one 4x4 BC3 block.
Same-block-geometry uploads and copies whose physical element sizes differ
stay exactly on the original path.

Other applications using the same uncommon equal-byte, different-block-
geometry reinterpretation can therefore be affected. The expected effect is
to make those copies match the physical data represented by the footprint,
but a claim of zero cross-game risk would be unjustified.

Evidence currently limiting that risk:

- focused regression: four deterministic failures with the old helper and
  22/22 assertions passing with current candidate `64ec55e7`;
- all native tests selected by `VKD3D_TEST_FILTER=copy`: 6,429,713 executed,
  zero failures, 14 successful todo, one skipped, eight todo, zero bugs;
- existing neighboring compressed-copy tests: 147/147 and 50/50 passing;
- native and MinGW x64 test builds compile, and x64/x86 packages build;
- two gated D07 runs and one clean D08 run repair the terrain;
- D08 reports no device loss, GPU reset/hang, OOM, or new fatal error.

The retained local current-candidate transcripts are
`captures/validation/64ec55e7-focused-copy-test.log` and
`captures/validation/64ec55e7-copy-tests.log`, with SHA-256 values recorded in
[`evidence-pr-scope-refinement.md`](evidence-pr-scope-refinement.md). They are
intentionally ignored by Git; the concise results above are sufficient for the
pull request.

Known limitations:

- the focused test has not yet been run on native Windows hardware;
- the runtime validation covers RADV on one RDNA3 system;
- the inverse image-to-placed-buffer helper was not changed because IL-2 does
  not demonstrate that direction;
- startup/NUMA and menu shimmering are unrelated and remain unresolved.

## If maintainers request narrower scope

Do not begin with an executable override while the general translation fix has
both a regression test and matching upstream precedent. The first narrowing—
requiring different block geometry with equal physical element size—was
implemented in `64ec55e7`. If reviewers still consider the native-Windows
behavior too application-specific, narrow in this order:

1. restrict it to the demonstrated
   `R32G32B32A32_UINT -> BC3_UNORM` upload pair;
2. use an exact `IL2Series.exe` application quirk only as the final fallback.

Each narrower revision must be rebuilt and retested. No format-pair or
application-specific fallback has been implemented in the current candidate.

## Minimal initial pull-request evidence

Keep the submission compact:

- the single clean commit and included regression test;
- one broken/fixed terrain screenshot pair;
- the focused old-versus-fixed result and the copy-suite summary;
- the D07 conversion counts and clean D08 build identity;
- a link to VKD3D-Proton #3134.

Large Proton logs, all diagnostic patches, duplicate screenshots, game files,
prefix data, and unrelated menu/NUMA investigation material should not be
attached to the initial pull request.
