## Summary

Implement `GetNumaHighestNodeNumber()` and
`GetNumaNodeProcessorMaskEx()` using the NUMA topology Wine already exposes
through `SystemLogicalProcessorInformationEx`.

## Why

`GetNumaNodeProcessorMaskEx()` currently fails unconditionally with
`ERROR_CALL_NOT_IMPLEMENTED`, while `GetNumaHighestNodeNumber()` always
reports node zero. Applications which use these APIs for affinity discovery
therefore receive inconsistent topology information.

This was observed in the Intel OpenMP runtime shipped with Korea. IL-2 Series.
Its affinity initialization fails before renderer initialization completes,
which prevents the game from starting without an environment workaround.

Related report: https://github.com/ValveSoftware/Proton/issues/9906

## Implementation

- Query `SystemLogicalProcessorInformationEx` for `RelationNumaNode`.
- Return the matching topology entry's `GroupMask` for a requested node.
- Derive the highest node number from those same entries.
- Test the results against the reported topology rather than assuming a CPU,
  node, group, mask, or thread count.

The change contains no application, processor-vendor, processor-count, Steam
AppID, or environment-variable special case.

## Testing

- Built i386 and x86-64 `kernelbase.dll`, `kernel32/tests/process.o`, and
  `kernel32_test.exe` from Wine master.
- Built the same source changes against the Wine commit pinned by Proton 11.
- Ran the i386 and x86-64 `process` test executables under the patched Proton
  runtime; the new NUMA assertions produced no failures. Two existing-version
  mismatch failures remained in each run because the current-Wine test binary
  was executed against the older Proton runtime.
- Initialized the game's exact Intel OpenMP runtime without an OpenMP
  environment workaround while restricting the process to 1, 2, 4, 8, and 16
  CPUs.
- Started the complete game with an otherwise validated Proton 11 tool and
  empty Steam launch options.

The full-game and processor-count tests were performed on one physical host.
Wine Test Bot/CI and review are needed to validate native Windows behavior and
additional physical NUMA layouts.

## Scope

This merge request only replaces the two NUMA API stubs and adds their tests.
Wine's existing processor-group behavior above 64 logical processors, and
Valve's separate `WINE_CPU_TOPOLOGY` override behavior, are not changed here.
