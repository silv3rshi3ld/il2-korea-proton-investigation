# Wine NUMA candidate

`0001-kernelbase-Implement-NUMA-node-query-functions.patch` is a local upstream
draft, not an application override. It has been superseded for submission and
runtime testing by existing
[Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604),
which implements the wider NUMA API path across ntdll, kernelbase, and
kernel32 and merged on 2026-08-10. The local draft is retained only as
investigation history. It is based on Wine master commit
`a37867ddf8418439b37e062abe156a42accf4d32`
(2026-08-06) and also builds on Valve's pinned Proton 11 Wine commit
`81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`.

The patch implements `GetNumaHighestNodeNumber` and
`GetNumaNodeProcessorMaskEx` by consuming Wine's existing
`RelationNumaNode` topology. It contains no processor count, affinity mask,
CPU vendor, executable name, Steam AppID, or environment-variable workaround.

Validation completed:

- Wine master i386 and x86-64 `kernelbase.dll` build successfully in a
  combined-architecture tree.
- Wine master i386 and x86-64 `kernel32/tests/process.o` and
  `kernel32_test.exe` build successfully.
- The same source changes build against Proton 11 Wine.
- The i386 and x86-64 `process` tests were executed with the candidate APIs
  under the validated Proton 11 runtime. The new NUMA assertions reported no
  failures; each run retained two unrelated failures caused by running a
  current-Wine test binary against the older Proton runtime.
- A current-Wine in-tree test of the local draft was attempted, but the
  intentionally partial build did not contain all runtime prerequisites and
  therefore did not reach a reportable result. This draft is now superseded by
  MR !11604, whose head pipeline succeeds.
- A disposable Proton 11 component test initializes the game's exact
  `libiomp5md.dll` without OpenMP launch variables on the reporting host.
- The component test succeeds with 1, 2, 4, 8, and 16 CPUs available to the
  process; no thread count is encoded in the patch.
- A complete D09 Proton 11 compatibility tool starts the full game on the
  reporting host with Steam launch options empty.
- The exact six commits at Wine MR !11604 head
  `e8319c0e6bfe7f94512218b48e3158e0c286b481` apply cleanly to Proton 11's
  pinned Wine commit and build in the conventional 64-bit Proton layout.
- The resulting D10 package initializes the exact game OpenMP DLL and starts
  the full game with launch options empty and no OpenMP/topology override.

This does not yet validate a different physical CPU or a multi-node topology.
MR !11604 was open and mergeable, and its head pipeline 72369 was successful at
the 2026-08-07 validation check. By 2026-08-10, the equivalent six-commit
series was also present in Valve's Wine fork and pinned by the Proton Bleeding
Edge source branch. Wine merged the MR later that day with final head
`663fd7cc06f042c81fa299fe799376ab70c4cfa5`.

This investigation tested earlier MR head
`e8319c0e6bfe7f94512218b48e3158e0c286b481`, not the final rebased head.
Physical cross-topology validation and confirmation in a published standard
Proton release remain. Any future local submission still requires the real
contributor's identity; this repository does not invent one.
