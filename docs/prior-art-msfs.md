# Microsoft Flight Simulator prior-art assessment

## Conclusion

Microsoft Flight Simulator 2020 and 2024 provide useful debugging precedent,
but no existing MSFS application workaround can simply be copied to IL-2.
Their most relevant fixes fall into two classes: DXIL-to-SPIR-V translation
bugs and exceptional host-memory-import behavior. The installed IL-2 Proton
build already contains all of the historical fixes below.

The transferable lesson is to test an unmodified current VKD3D-Proton and
`dxil-spirv` revision before adding IL-2-specific behavior, then use shader
hashes or host-import telemetry to select the next diagnostic. It is not to
enable `host_import_fallback` or an MSFS quirk speculatively.

## Cases reviewed

### MSFS 2024: initial world-loading failure

The November 2024 Proton report recorded `vkCreateGraphicsPipeline` failures
and a Mesa SPIR-V parser error involving unstructured control flow. A linked
VKD3D-Proton update advanced `dxil-spirv` by one change which corrected
clip/cull handling in tessellation and geometry stages. Users then confirmed
that the bleeding-edge Proton build launched and rendered map tiles and
satellite imagery.

This was a hard pipeline-creation failure. IL-2 currently creates its pipelines
and reaches both menu and flight, and its collected logs contain no matching
SPIR-V parser or `vkCreateGraphicsPipeline` error. The exact MSFS 2024 fix is
therefore already present and its failure signature does not match IL-2.

Primary references:

- [Proton MSFS 2024 report #8255](https://github.com/ValveSoftware/Proton/issues/8255)
- [reported SPIR-V parser failure](https://github.com/ValveSoftware/Proton/issues/8255#issuecomment-2495296400)
- [VKD3D-Proton submodule update 5675d7cd](https://github.com/HansKristian-Work/vkd3d-proton/commit/5675d7cd607e88f7ab33009c7f271074fbdddcfd)
- [`dxil-spirv` clip/cull fix de184b1f](https://github.com/HansKristian-Work/dxil-spirv/commit/de184b1f8075a2ac29529173397ea47301f6fb20)
- [successful map-tile confirmation](https://github.com/ValveSoftware/Proton/issues/8255#issuecomment-2498346930)

### MSFS 2024: grass and near-ground crash

In January 2025, MSFS 2024 could crash near the ground when grass was enabled.
The VKD3D-Proton maintainer identified a `dxil-spirv` bug that `spirv-val` did
not catch. PR #2321 updated the translator; its included change sinks access
chains that violate value dominance. Users confirmed that both the launch
error and grass-triggered error disappeared.

This is the strongest methodological match to IL-2's altitude-dependent
terrain failure: a camera/scene change selected a shader path and exposed a
translator defect, while ordinary validation did not diagnose it. It does not
prove that IL-2 has the same defect, and the MSFS patch is already in the
installed VKD3D-Proton revision.

Primary references:

- [maintainer diagnosis](https://github.com/ValveSoftware/Proton/issues/8255#issuecomment-2624352639)
- [VKD3D-Proton PR #2321](https://github.com/HansKristian-Work/vkd3d-proton/pull/2321)
- [`dxil-spirv` value-dominance fix](https://github.com/HansKristian-Work/dxil-spirv/commit/7d86eba5d763c9f3d72be89c1fd9f462ddec9ad5)
- [user confirmation](https://github.com/ValveSoftware/Proton/issues/8255#issuecomment-2625607915)

### MSFS 2020: huge host-memory import

VKD3D-Proton issue #1847 found that MSFS 2020 used
`OpenExistingHeapFromAddress` to import a 16 GiB host allocation. Queue
submissions became extremely slow in the Linux kernel and memory use became
very large. This explains why later MSFS advice mentions
`VKD3D_CONFIG=host_import_fallback`.

IL-2's representative logs do not contain a host-import or
`OpenExistingHeapFromAddress` signature, and its symptoms are visual
corruption rather than extreme GTT use and roughly 5 FPS. The flag is not a
justified IL-2 experiment unless a focused trace first proves that IL-2 calls
the host-import path.

Primary reference:

- [VKD3D-Proton MSFS 2020 issue #1847 and diagnosis](https://github.com/HansKristian-Work/vkd3d-proton/issues/1847#issuecomment-1911923254)

### MSFS 2020: imported-memory clear and ROV analysis

The November 2025 MSFS 2020 DX12 crash exposed two real issues. PR #2686
stopped VKD3D-Proton from zeroing or command-clearing externally imported
allocations. PR #2687 updated `dxil-spirv` to handle an early-return case in
ROV analysis which otherwise segfaulted. The maintainer separately suggested
`host_import_fallback` for unusable AMD performance caused by host imports.

IL-2 has no crash, ROV-analysis error, imported-memory clear evidence, or
extreme host-import performance signature. These fixes are already included
in the installed VKD3D-Proton revision and are not candidate IL-2 overrides.

Primary references:

- [VKD3D-Proton MSFS 2020 issue #2685](https://github.com/HansKristian-Work/vkd3d-proton/issues/2685)
- [PR #2686: avoid clearing imported resources](https://github.com/HansKristian-Work/vkd3d-proton/pull/2686)
- [PR #2687: `dxil-spirv` update](https://github.com/HansKristian-Work/vkd3d-proton/pull/2687)
- [`dxil-spirv` early-return ROV fix](https://github.com/HansKristian-Work/dxil-spirv/commit/07c1244e6c417998b15a1ecb6cc727df42284c1b)

### Missing textures on an old Intel Mesa stack

One MSFS 2024 reporter had widespread missing textures on Intel Mesa 24.0.9
while also forcing `enable_experimental_features`. Updating Mesa and removing
that option fixed the report. This is evidence that a driver/version A/B can
be decisive, not evidence that IL-2's current RADV 26.1.6 is defective.

Primary reference:

- [Intel missing-texture report and resolution](https://github.com/ValveSoftware/Proton/issues/8255#issuecomment-2496157101)

## Consequences for the IL-2 investigation

1. Do not copy `host_import_fallback`; first trace
   `OpenExistingHeapFromAddress` and imported-allocation creation.
2. Do not expect `spirv-val` alone to exclude shader translation. MSFS 2024
   demonstrated a real translator bug which passed that validator.
3. Treat altitude as a path selector. Matched high- and low-altitude captures
   can identify shader hashes or resources that appear only in the failing
   distant-terrain path.
4. Test current upstream VKD3D-Proton without IL-2 changes. Installed Proton
   uses VKD3D-Proton `3dfc6f07` with `dxil-spirv` `7ecda135`; current upstream
   VKD3D-Proton `84c87c83` uses `dxil-spirv` `cc75a0c9`, 36 translator commits
   newer. Several of those changes alter control-flow structurization. This is
   a justified version discriminator, not evidence that one of them fixes IL-2.
5. If current upstream is unchanged, continue with descriptor QA and filtered
   shader-hash/resource-use telemetry. If it fixes the scene, bisect the
   VKD3D-Proton or `dxil-spirv` range before proposing any patch.

D04 subsequently tested that current-upstream range and was visually
unchanged. The MSFS-derived version lead is therefore closed; no source-range
bisection or MSFS-specific workaround is selected.

All historical commits reviewed here are ancestors of the installed
VKD3D-Proton commit. No MSFS-named application profile or executable override
was found in current VKD3D-Proton source.
