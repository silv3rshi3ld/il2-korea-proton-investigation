# Upstream submission plan

Nothing in this plan has been posted or pushed. Publication remains the user's
decision.

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

The current candidate changes only placed-buffer-to-image copies for which the
source and destination physical elements have the same byte size. The IL-2
case is 16 bytes on each side: one uncompressed `R32G32B32A32_UINT` texel maps
to one 4x4 BC3 block. Ordinary same-format uploads and copies whose physical
block sizes differ retain their existing results or existing fallback path.

Other applications using the same uncommon equal-byte, different-block-
geometry reinterpretation can therefore be affected. The expected effect is
to make those copies match the physical data represented by the footprint,
but a claim of zero cross-game risk would be unjustified.

Evidence currently limiting that risk:

- focused regression: four deterministic failures with the old helper and
  22/22 assertions passing with `cf11ba76`;
- all native tests selected by `VKD3D_TEST_FILTER=copy`: 6,429,713 executed,
  zero failures, 14 successful todo, one skipped, eight todo, zero bugs;
- existing neighboring compressed-copy tests: 147/147 and 50/50 passing;
- native and MinGW x64 test builds compile, and x64/x86 packages build;
- two gated D07 runs and one clean D08 run repair the terrain;
- D08 reports no device loss, GPU reset/hang, OOM, or new fatal error.

The retained local full copy-test transcript is
`captures/validation/cf11ba76-copy-tests.log` with SHA-256
`0a9410ada9861d59c02445354340d514ec430c3d2ca2ebb3b58462685d36c970`.
It is intentionally ignored by Git; the concise result above is sufficient for
the initial pull request.

Known limitations:

- the focused test has not yet been run on native Windows hardware;
- the runtime validation covers RADV on one RDNA3 system;
- the inverse image-to-placed-buffer helper was not changed because IL-2 does
  not demonstrate that direction;
- startup/NUMA and menu shimmering are unrelated and remain unresolved.

## If maintainers request narrower scope

Do not begin with an executable override while the general translation fix has
both a regression test and matching upstream precedent. If reviewers consider
the native-Windows behavior too application-specific, narrow in this order:

1. restrict the conversion to differing block geometry with equal physical
   element size;
2. if still required, restrict it to the demonstrated
   `R32G32B32A32_UINT -> BC3_UNORM` upload pair;
3. use an exact `IL2Series.exe` application quirk only as the final fallback.

Each narrower revision must be rebuilt and retested. No speculative fallback
has been implemented in the current candidate.

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
