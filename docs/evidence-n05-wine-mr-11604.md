# N05 upstream Wine MR !11604 test

Date: 2026-08-07

## Post-validation upstream status

At the final repository check on 2026-08-10, Wine MR !11604 remained open and
mergeable. Its six commits had also been applied to
[Valve's Wine fork](https://github.com/ValveSoftware/wine/compare/c3007e6f2a36914cc55301eb5efd067707bf8bb1...99166a7e25b08ccef0168217540542260eaed76f).
The
[Proton Bleeding Edge source branch](https://github.com/ValveSoftware/Proton/commit/d28e7f2c40da279452db93897c5b9c2c84356fac)
then pinned Wine revision `99166a7e25b08ccef0168217540542260eaed76f`.

The standard `experimental_11.0` and `proton_11.0` source branches were still
pinned before this series at that check. This section records source
integration status only; the runtime results below remain the controlled test
of the exact MR series in the custom D10 Proton tool.

## Question

Does the existing upstream Wine NUMA series in
[Wine MR !11604](https://gitlab.winehq.org/wine/wine/-/merge_requests/11604)
fix Korea. IL-2 Series startup through Proton without an application override,
a hard-coded processor count, or Steam launch parameters?

## Source and packaging

The tested merge-request head is
`e8319c0e6bfe7f94512218b48e3158e0c286b481`. Its six commits implement
`SystemNumaProcessorMap` in ntdll and the associated kernelbase/kernel32 NUMA
APIs. The series was applied without conflicts to Valve's Proton 11 Wine commit
`81d78e4f3ea8ce868d775021fdc9f90122dc1a6b`.

At the final status check on 2026-08-07, the Wine merge request is open,
non-draft, conflict-free and marked mergeable. Its head pipeline 72369 is
successful. It is not merged or maintainer-approved yet.

The isolated Steam compatibility tool is
`IL2-Korea-D10-WineMR11604-Proton11`. It retains D08's previously validated
VKD3D-Proton terrain fix and replaces only these 64-bit Wine components:

```text
03a0732af9199f8ee769eb06f2cf6e564db216fb3b9efe344313f104206cd766  x86_64-unix/ntdll.so
7c24b9e14e364500759f9439505cc89f7f526b0198b2e209f594fa2b4b233c24  x86_64-windows/ntdll.dll
af4676508250a932c26975e706da76747e312f593911f9ac2fe393ac68d2d630  x86_64-windows/kernelbase.dll
fe3ac77b8d811d9f89742e08989b9015786ecf9913762f000cf9aa3141e3d7c9  x86_64-windows/kernel32.dll
```

The final binaries were built in Wine's conventional `--enable-win64` layout
with libunwind disabled, matching the architecture and runtime dependency
layout of the packaged Proton tool. An earlier combined-WoW64 substitution
passed the component probe but exited immediately in the complete Steam path;
that invalid packaging result is not evidence against the merge request.

D10 replaces only 64-bit modules because IL-2 is a 64-bit validation target;
it is not proposed as a distributable Proton release. The same source series
also built in the combined i386/x86-64 test tree. A normal Wine/Proton build
would compile and package every supported architecture from the general source
change.

## Component result

The final packaged D10 Wine binary was run in a fresh prefix with the exact
game `libiomp5md.dll`. `KMP_AFFINITY`, `KMP_HW_SUBSET`, `OMP_NUM_THREADS`, and
`WINE_CPU_TOPOLOGY` were all absent. It exited successfully:

```text
active_groups=1 active_group0=16
GetNumaHighestNodeNumber: ret=1 highest=0
GetNumaNodeProcessorMaskEx(0): ret=1 mask=0xffff group=0
RelationNumaNode: node=0 mask=0xffff group=0
OpenMP: num_procs=16 max_threads=16
```

The values are discovered from this host. A separate affinity sweep of the
exact merge-request build at 1, 2, 4, 8, and 16 allowed CPUs succeeded at every
size, and Intel OpenMP reported the corresponding allowed count. Nothing in
the six-commit series encodes 16, `0xffff`, AMD, IL-2, or AppID 247970.

## Full Steam result

Pass on the reporting host.

- AppID: `247970`
- Compatibility tool: `IL2-Korea-D10-WineMR11604-Proton11`
- Steam launch-options value: empty
- Launch time: 2026-08-07 16:26:55 CEST
- Historical OpenMP variables: absent from the live game environment
- Topology override: absent from the live game environment

Steam's process record names D10 and invokes the normal
`IL2Series.exe or_enable=0` command. The live process maps D10's patched
`ntdll.so`, `ntdll.dll`, `kernelbase.dll`, and `kernel32.dll`, plus the game's
own `libiomp5md.dll`. It remained active beyond the former startup-abort point,
reached 91 threads, used the OS-allowed CPU list `0-15`, and wrote fresh GUI,
localization, career, input, texture, and statistics state. This is a clean
startup pass without the historical launch workaround.

The user visually confirmed that the previously known shimmering squares are
still present. That is expected to be a separate issue: D10 changes CPU/NUMA
reporting and fixes startup while leaving the shimmering unchanged. Earlier
VKD3D-only D07/D08 tests fixed terrain and also left the shimmering unchanged.
The evidence excludes the NUMA/OpenMP defect as the cause of the shimmering;
it does not yet identify the shimmering's own cause.

## Evidence boundary

This validates the upstream series on one AMD, 16-logical-CPU,
single-NUMA-node host and demonstrates that the implementation is not a fixed
16-thread workaround. It does not validate a physically different CPU, sparse
NUMA node IDs, multiple NUMA nodes, or Wine's processor-group boundary. Those
remain upstream review and cross-hardware test cases; the result should not be
described as universally validated yet.
