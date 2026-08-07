# N04 full-game NUMA candidate test

Date: 2026-08-07

## Question

Does the full game pass Intel OpenMP initialization and start through Proton
without `KMP_AFFINITY`, `OMP_NUM_THREADS`, or any other Steam launch option?

## Compatibility tool

Steam tool: `IL2-Korea-D09-NUMA-Proton11`

The tool is an isolated copy of the previously validated
`IL2-Korea-D08-GeneralFix-cf11ba76` tool. D08 is retained untouched. D09 differs
only in its tool name/version and the i386/x86-64 Wine `kernelbase.dll` pair.
This keeps the validated terrain candidate constant while adding the focused
NUMA candidate.

Wine source base:
`81d78e4f3ea8ce868d775021fdc9f90122dc1a6b` (`proton_11.0`)

Candidate binary hashes:

```text
7bf2e914f9661e823719c4599dd73d91802ea1b1ec03e4c361eea9845b2a0845  i386-windows/kernelbase.dll
6e017470b28f401db1331c1b368b203274410450b222a176af9ec5423762b5a8  x86_64-windows/kernelbase.dll
```

Both DLLs and both architecture variants of the Wine process-test object build
successfully in a combined i386/x86-64 Proton Wine tree.

## Preflight result

The packaged D09 Wine binary was run with a fresh prefix, the exact game
`libiomp5md.dll`, and both OpenMP variables absent. It returned exit status 0:

```text
active_groups=1 active_group0=16
GetNumaHighestNodeNumber: ret=1 highest=0
GetNumaNodeProcessorMaskEx(0): ret=1 mask=0xffff group=0
RelationNumaNode: node=0 mask=0xffff group=0
OpenMP: num_procs=16 max_threads=16
```

The values describe the reporting host; they are not encoded in the candidate.

## Steam test state

- AppID 247970 compatibility mapping: `IL2-Korea-D09-NUMA-Proton11`
- Launch options: empty
- Game files: unchanged
- Existing compatdata prefix: retained
- Steam configuration backups:
  `/tmp/il2-d09-localconfig.vdf.before` and
  `/tmp/il2-d09-config.vdf.before`

## Result gate

The test is a pass for the startup issue only if the full game proceeds beyond
the former OpenMP/loading abort with launch options still empty. Reaching the
menu does not validate other CPU topologies. Any later graphics or gameplay
failure is recorded separately and must not be attributed to OpenMP without
new evidence.

## Result

Pass on the reporting host.

Steam started AppID 247970 through
`IL2-Korea-D09-NUMA-Proton11` with the recorded launch-options field empty.
The actual process chain names D09 and contains no `KMP_AFFINITY` or
`OMP_NUM_THREADS` assignment. `IL2Series.exe` continued through startup, wrote
fresh game logs, initialized the GUI/localization and career list, and the user
confirmed that the game started.

This validates removal of the OpenMP launch workaround for this installation.
It does not establish behavior on a different CPU count/vendor, a multi-node
host, or Wine's existing processor-group boundary.

## Processor-count sweeps

Two same-host sweeps were run with the exact OpenMP DLL and no OpenMP launch
variables:

- Linux affinity restricted the process to 1, 2, 4, 8, and 16 CPUs. Intel
  OpenMP initialized each time and reported exactly 1, 2, 4, 8, and 16 usable
  processors respectively.
- Proton Wine's `WINE_CPU_TOPOLOGY` override was set to 1, 2, 4, and 8. The
  Windows active-processor count and Intel OpenMP result changed to each
  requested value, and all four runs initialized successfully.

The second sweep also exposed a pre-existing Proton Wine inconsistency:
`RelationNumaNode` retained the physical host mask `0xffff` while the overridden
active-processor count changed. `GetNumaNodeProcessorMaskEx` returned that same
canonical Wine mask, as designed. The override is Valve-specific and is absent
from upstream Wine master; it therefore cannot serve as proof of a different
real NUMA layout. The sweep proves that this OpenMP runtime is not forced to 16
threads, but real cross-topology testing remains outstanding.

## Upstream follow-up

N04's local one-patch candidate is superseded for upstream work by existing
Wine MR !11604. The exact six-commit merge-request head was tested separately
as D10 and also starts the complete game with launch options empty. See
[`evidence-n05-wine-mr-11604.md`](evidence-n05-wine-mr-11604.md).
