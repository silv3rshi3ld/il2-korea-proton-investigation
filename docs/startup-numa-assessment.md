# Startup/NUMA assessment

Date: 2026-08-07

## Outcome

On the reporting system, the startup failure has a focused Wine-level cause.
The game's Intel OpenMP runtime calls `GetNumaNodeProcessorMaskEx`; current
Proton 11 Wine leaves that function as an unconditional stub returning
`ERROR_CALL_NOT_IMPLEMENTED`. Intel OpenMP treats that as a fatal affinity
initialization failure and aborts with Error #179.

The remedy is not an IL-2 customization and does not contain the reporter's
thread count. Existing upstream Wine MR !11604 implements the missing NUMA
queries from the topology Wine already discovers for the current machine.

DXR, the NVIDIA DLSS DLL, and the AMD FFX DLL are initialized earlier in the
game's log, but the same failure occurs in a small process that creates no D3D
device and loads only the game's OpenMP DLL. RTX/DXR is therefore useful as an
ordering symptom, not the cause of this startup abort.

## Evidence

The exact game `libiomp5md.dll` imports both
`GetNumaHighestNodeNumber` and `GetNumaNodeProcessorMaskEx`. Against the exact
installed Proton Experimental 11 Wine family:

| Environment | Result |
|---|---|
| no override | MaskEx fails with error 120; Intel OpenMP Error #179 |
| `OMP_NUM_THREADS=16` | same failure |
| `KMP_AFFINITY=disabled` | initializes successfully |
| both variables | initializes successfully |

This isolates affinity discovery as the failing path. It also proves that the
literal thread count in the historical launch parameters is not required for
the runtime to initialize.

Wine's existing
`GetLogicalProcessorInformationEx(RelationNumaNode)` result on the reproducer
is node 0, group 0, mask `0xffff`. The 16 set bits describe this host; neither
`16` nor `0xffff` is part of the candidate implementation.

With the candidate `kernelbase.dll` placed only in a disposable copy of the
same Proton build, the exact game OpenMP DLL initializes with no OpenMP
environment variables:

```text
active_groups=1 active_group0=16
GetNumaHighestNodeNumber: ret=1 highest=0
GetNumaNodeProcessorMaskEx(0): ret=1 mask=0xffff group=0
RelationNumaNode: node=0 mask=0xffff group=0
OpenMP: num_procs=16 max_threads=16
```

The untouched Steam compatibility tool remains unchanged. This component
substitution is validation evidence, not the proposed way to distribute the
fix.

The subsequent D09 test packaged both Wine architectures into a dedicated
Proton 11 compatibility tool. Steam selected that tool for AppID 247970 with
the launch-options field empty, and the user confirmed that the full game
started. This resolves the launch-parameter requirement on the reporting host.

The exact six commits from Wine MR !11604 were then backported without
conflicts to Proton 11's pinned Wine commit and packaged as D10. The packaged
component test passed, Steam launched the complete game with launch options
empty, and the live process loaded the patched ntdll, kernelbase, and kernel32
modules plus the game's OpenMP DLL. No OpenMP or topology override was present.
This independently validates the existing upstream implementation on the
reporting host.

## General implementation

MR !11604 implements the path at Wine's normal API layers:

1. ntdll derives a `SYSTEM_NUMA_INFORMATION` map from its canonical
   `RelationNumaNode` records and exposes it through
   `NtQuerySystemInformation(SystemNumaProcessorMap)`.
2. kernelbase implements `GetNumaHighestNodeNumber` and
   `GetNumaNodeProcessorMask[Ex]` from that system map.
3. kernel32 implements `GetNumaProcessorNode[Ex]` from the same map.
4. The public APIs validate absent nodes/processors and propagate NT query
   errors through the normal Wine conversion path.

Wine's ntdll already constructs this data from the host topology. On Linux it
reads the online NUMA nodes and their CPU masks from sysfs, with a one-node
all-CPU fallback when NUMA sysfs data is unavailable. Consequently, the
kernelbase implementation does not need to know whether the processor is AMD
or Intel, how many physical cores or SMT threads it has, or which application
made the query.

The upstream series adds topology-derived ntdll tests rather than encoding this
host's node count, processor count, or mask.

Same-host affinity and Proton topology-override sweeps initialized the exact
OpenMP runtime at 1, 2, 4, and 8 usable processors, in addition to the native
16. This confirms that neither the candidate nor the runtime result is fixed
to 16 threads. Proton's Valve-specific `WINE_CPU_TOPOLOGY` path retains the
host NUMA mask while overriding the active processor count, however, so this
is not evidence for a genuinely different NUMA layout. Upstream Wine master
does not contain that override path.

## Intended generality and current evidence boundary

The candidate is designed as a general fix for the topologies Wine currently
represents, rather than as a fix for a 16-thread system. Its inputs are
topology records, so differing CPU vendors and counts, SMT on or off, and one
or multiple NUMA nodes require no source-code changes.

That is a design property, not yet a cross-hardware test result. Runtime
validation so far covers one AMD, 16-logical-CPU, single-NUMA-node host. It does
not yet cover an Intel host, another processor count, SMT disabled, or multiple
NUMA nodes. Full-game startup without parameters is validated only on the
reporting host. The remaining cases must be tested before describing the
runtime behavior as generally validated.

Wine currently has a broader processor-group limitation: its topology path is
not yet a complete Windows-style representation for hosts with more than 64
logical processors in a 64-bit process (and the corresponding 32-bit limit).
That limitation predates this patch. Reimplementing processor groups is a
large ntdll/wineserver project and should not be hidden inside this startup
fix. Because the proposed functions consume Wine's canonical topology rather
than building their own, later processor-group improvements will flow through
without changing this implementation.

Microsoft documents MaskEx as returning a `GROUP_AFFINITY` for a NUMA node and
notes that current systems return the node's primary group. Returning Wine's
existing `GroupMask` matches that data model.

## Where it should land

The source fix is already proposed to Wine as MR !11604. It is a Win32 topology
API implementation, not a VKD3D-Proton, DXVK, Mesa, game-mod, or Proton
AppID-quirk change.

For development and game validation, the appropriate base is Valve's current
`proton_11.0` branch because issue #9906 reproduces there and the installed
Proton Experimental tool uses that family. A complete custom compatibility
tool should contain the Wine change and otherwise track the official branch.
Proton Experimental or Bleeding Edge is the normal public validation channel;
a Proton Hotfix release is a Valve prioritization decision, not a separate
implementation target. After Wine review, Valve can pick the commit into its
Wine fork and numbered Proton releases can inherit it.

## Remaining validation gates

- Follow MR !11604 through Wine review and any CI reruns; its current head
  pipeline is successful.
- Repeat the full-game no-options launch if startup repeatability is needed;
  complete D09 and exact-upstream D10 runs passed on the reporting host.
- Run the API/OpenMP probe on different logical-processor counts and CPU
  vendors, and on a real or virtual multi-NUMA-node system. Record the returned
  nodes, groups, and masks instead of assuming them.
- Test real multi-node and sparse-node-number layouts before describing the
  runtime behavior as universal.

## Primary references

- [Proton issue #9906](https://github.com/ValveSoftware/Proton/issues/9906)
- [Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604)
- [Microsoft: GetNumaNodeProcessorMaskEx](https://learn.microsoft.com/en-us/windows/win32/api/systemtopologyapi/nf-systemtopologyapi-getnumanodeprocessormaskex)
- [Microsoft: GetNumaHighestNodeNumber](https://learn.microsoft.com/en-us/windows/win32/api/systemtopologyapi/nf-systemtopologyapi-getnumahighestnodenumber)
- [Wine kernelbase implementation](https://github.com/wine-mirror/wine/blob/master/dlls/kernelbase/memory.c)
- [Valve: Proton versions](https://github.com/ValveSoftware/Proton/wiki/Proton-Versions)
