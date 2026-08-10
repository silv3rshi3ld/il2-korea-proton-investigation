# U01 clean upstream-candidate A/B result

> [!IMPORTANT]
> U01 remains valid runtime and mechanism evidence: its clean one-commit build
> removed the blocks and broad flicker while preserving real lighting. The
> implementation architecture was later superseded after maintainer review.
> The current ABI-safe paired dxil-spirv and capability-gated VKD3D-Proton
> result is D49; see
> [`evidence-d49-compiler-aware-result.md`](evidence-d49-compiler-aware-result.md).
> This evidence file remains at its original path so existing references and
> image provenance stay stable.

## Purpose

U01 validated the visual behavior of the first clean lighting implementation
independently of the diagnostic branches. It compares a fresh build of
unmodified VKD3D-Proton master with a fresh build of the single clean candidate
commit:

- baseline: upstream `84c87c8390d9df75ba41d911496296fe13f0e275`;
- candidate: `9b6e15be29fc1ebb1c26796477009152cb1c760d`, exactly one
  commit ahead of that baseline;
- candidate diff: 29 insertions in four source files.

Both builds completed for x86-64 and x86. Source and binary inspection confirms
that the candidate contains the `IL2Series.exe` mapping and allocator-only
quirk, while the baseline does not. Neither build contains the rejected depth
quirk or its producer shader hashes.

## Controlled Proton tools

The two isolated tools were created from the same copy-on-write source tool,
`IL2-Korea-D42-Complete-2d9a7467`. Its version is
`experimental-11.0-20260724c-wine-mr11604-d10`, so both sides include the same
NUMA-capable Wine components.

- `IL2-Korea-PR-Baseline-84c87c83`
- `IL2-Korea-PR-Candidate-9b6e15be`

A recursive comparison finds only the expected differences: the tool-name VDF,
diagnostic metadata, and the four replaced VKD3D-Proton DLLs. The relevant
x86-64 Wine components are bit-identical between the tools:

| Component | SHA-256 |
| --- | --- |
| `x86_64-unix/ntdll.so` | `03a0732af9199f8ee769eb06f2cf6e564db216fb3b9efe344313f104206cd766` |
| `x86_64-windows/ntdll.dll` | `7c24b9e14e364500759f9439505cc89f7f526b0198b2e209f594fa2b4b233c24` |
| `x86_64-windows/kernelbase.dll` | `af4676508250a932c26975e706da76747e312f593911f9ac2fe393ac68d2d630` |
| `x86_64-windows/kernel32.dll` | `fe3ac77b8d811d9f89742e08989b9015786ecf9913762f000cf9aa3141e3d7c9` |

The installed VKD3D-Proton binaries are:

| Architecture | File | Baseline SHA-256 | Candidate SHA-256 |
| --- | --- | --- | --- |
| x86-64 | `d3d12.dll` | `90efbd66560bf329be618980156ee3fc4f439d49d82232e764f233850ffac6ae` | `effc65c16745831c276d5fdf2a50c26ad8b51e355356eb62c5b7ede940721a65` |
| x86-64 | `d3d12core.dll` | `d16e2b869f22ab95788a5780dec76dd492afe27c916d3601e24371edeeaf170c` | `164847d8ad795d308fa076f91567a3a9320b8c6eb24b1bad5b2f92527d90e72b` |
| x86 | `d3d12.dll` | `47f31e9cbc9e3b31c46ba0f5e0d34351065d067c4956da9e9754ceda75e268b0` | `17de6a419afe8c1dd90e8af25bb9e6d95a58ddbf2edfb0ed935bec9ea23c6e72` |
| x86 | `d3d12core.dll` | `92ca913fe2534470bbb12beea0727c9f4ed71761591f897a98a9befbdaccdc5a` | `73a77a5c27bc73c584a8a8ec7b226558d6528bef1f0387468eb074439b2beeca` |

These binary hashes identify the exact tested builds; PE build timestamps mean
that an independent rebuild from the same source need not have identical full
file hashes.

## Runtime result

Both sides used empty Steam launch options, the same game settings and hangar
scene, and no RenderDoc layer, shader override, or game modification.

The upstream baseline reproduced the translucent tiled blocks and their broad
temporal flashing. The static Before screenshot understates the severity: the
blocks are less pronounced in that captured frame than they were while they
flashed during normal runtime.

The clean candidate removed the blocks and broad flashing in the matched
scene. Real lighting and shadows remain visible. Prefix `config_info` was read
after the candidate run and identifies
`IL2-Korea-PR-Candidate-9b6e15be`. This fresh candidate was run once; the earlier
D45 and allocator-only D47 controls provide the independent clean restarts.
The fine sandy or film-grain lighting is excluded because it is also present
on native Windows.

| Role | Repository evidence file | SHA-256 |
| --- | --- | --- |
| Before, upstream master | [`images/lighting-before-upstream-84c87c83.png`](images/lighting-before-upstream-84c87c83.png) | `8f88c75baaf51f595edd94362d5663554415082390e890076ba7d2209d3682be` |
| After, allocator-only candidate | [`images/lighting-after-candidate-9b6e15be.png`](images/lighting-after-candidate-9b6e15be.png) | `29df6e3346c79597135fe0f8dc833aed149e2e099308057bb497a18144ddc454` |

Both files are unmodified 2560x1080 PNG screenshots. Their repository paths
provide stable image URLs for the issue comment and pull-request description;
no crop, recompression, or annotation was applied.

## Conclusion

The clean one-commit candidate reproduces D47's allocator-only correction on
top of its then-current upstream master. No depth workaround, launch parameter,
hard-coded processor value, or game customization was required. This proves the
required runtime behavior, but not the suitability of the original
one-repository implementation. That implementation was proposed in
[VKD3D-Proton PR #3207](https://github.com/HansKristian-Work/vkd3d-proton/pull/3207)
and was superseded after valid maintainer feedback about how dxil-spirv and
descriptor layouts interpret the resource.

D49 retains the same exact executable and shader scope while moving the generic
legalization into dxil-spirv, avoiding public callback-struct growth, requiring
a valid SSBO remap, and allowing VKD3D-Proton to withhold the quirk on unsafe
layouts. U01 should therefore be cited as historical causal and visual evidence;
D49 is the current implementation result.
