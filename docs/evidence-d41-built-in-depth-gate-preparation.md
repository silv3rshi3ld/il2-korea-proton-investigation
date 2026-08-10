# D41 built-in tiled-light depth-gate compatibility quirk: preparation

## Goal

Convert the visually successful D38 shader experiment into normal
VKD3D-Proton behavior for IL-2, without `VKD3D_SHADER_OVERRIDE`, a game mod,
or a general change affecting unrelated games.

## Scope

The implementation starts from clean VKD3D-Proton commit
`84c87c8390d9df75ba41d911496296fe13f0e275`. It introduces the shader quirk
`VKD3D_SHADER_QUIRK_BYPASS_TILED_LIGHT_DEPTH_CULLING`, selected only for the
exact executable `IL2Series.exe` and the two exact producer hashes:

- `651194bd0a21772e` — `ComputeLightsCount`
- `11e32439a86036ba` — `ComputeLightsIndices`

The post-translation transformation implements the same predicate as D38:

```text
original_membership -> merged_near < merged_far
```

This keeps each shader's real light-volume calculation, count/index agreement,
genuine light records, consumer loops, per-light calculations, and shadows.
It bypasses only the later tile-depth rejection which D38-D40 isolate as the
source of the non-native blocks.

## Safety contract

The transformation is not a broad pattern rewrite. Each shader has an exact
contract covering its expected type IDs, near/far values, original membership
value, merge label, branch labels, and instruction adjacency. If any part does
not occur exactly once, shader compilation fails rather than silently patching
different code. A future game shader change therefore cannot accidentally
receive a guessed rewrite under the old hash.

This is a pragmatic native-compatibility allowance. Ideally the application
would use depth culling that translates portably, but compatibility software
commonly reproduces native Windows driver tolerance when a narrowly proven,
application-scoped behavior is safe and preserves the intended rendering.

## Build verification

- Source worktree: `src/vkd3d-proton-d38`
- Branch: `il2-depth-gate-compat`
- Build: `build/vkd3d-proton-il2-depth-gate-84c87c83`
- VKD3D identity: `v3.0.1-259-g84c87c83+`
- `git diff --check`: pass
- x86-64 and x86 MinGW builds: pass

| File | SHA-256 |
| --- | --- |
| `x64/d3d12.dll` | `57d179ffedc436e99f89f7c39beb6821b9969385afcb4da48c234a8911554233` |
| `x64/d3d12core.dll` | `f80270c191feeb7eb5ae5e309561c87470a2c591b2dcec8d3d6b9f75cc6d273b` |
| `x86/d3d12.dll` | `c4ae5d12f141d1a11c79e186ef9177b25b9f8a6f675e3f002a74adc1edb79a97` |
| `x86/d3d12core.dll` | `b341d6e490bd5c1d2dbce46bf873d6ea1c6eefad4d8a240306e5d53ab6a27a6d` |

The next gate is a local custom Proton tool retaining the already validated
Wine NUMA startup fix, followed by an ordinary run with empty Steam launch
options. A successful D41 run must remove the large squares while retaining
real lighting and shadows. Native sandy/film-grain lighting is explicitly
accepted.
