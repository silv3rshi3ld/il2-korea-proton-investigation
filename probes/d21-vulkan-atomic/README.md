# D21 Vulkan storage-texel-buffer atomic test

This headless probe distinguishes a RADV storage-texel-buffer atomic failure
from the remaining VKD3D-Proton path. It does not load or modify IL-2.

Both compute shaders dispatch `10x5` workgroups with a local size of `8x8` and
apply an `80x34` active bound, matching the captured tiled-light allocation
shape. All 2,720 active invocations atomically increment one shared counter and
record the returned value.

- `atomic_texel.comp` uses the game-like `R32_UINT` storage texel buffer and
  compiles to `OpImageTexelPointer` plus `OpAtomicIAdd`.
- `atomic_ssbo.comp` uses a storage-buffer atomic as the control.

Correct output has a final counter of 2,720 and exactly one copy of every value
from 0 through 2,719. Only one workgroup can contain zero. Repeated sequences
beginning at zero would reproduce the D20 workgroup-local allocation pattern.

Build, list devices, and run with validation enabled:

```text
make -C probes/d21-vulkan-atomic
make -C probes/d21-vulkan-atomic list
make -C probes/d21-vulkan-atomic run DEVICE=0
```

Select another enumerated physical device with `DEVICE=1`. Generated SPIR-V
and the native executable stay in the ignored local `build/` directory.

After D21 passes, `run-exact` performs D22 with the local, ignored
`ComputeLightsFirstRef` SPIR-V captured in D14. The game shader is read from its
existing local capture path and is not copied into the tracked probe:

```text
make -C probes/d21-vulkan-atomic run-exact DEVICE=0
```

D22 runs the shader's exact runtime descriptor arrays twice: once through a
mutable descriptor set and once through `VK_EXT_descriptor_buffer`, matching
the normal D20 backend. It supplies an `80x34x2 R32_UINT` grid with
deterministic 1-5 counts and checks that the packed intervals cover one global
8,160-entry range without overlap. Override
`EXACT_SPV=/path/to/shader.spv` if the local capture path differs.

`run-all` adds the two descriptor-shape discriminators used after D22 passed:

```text
make -C probes/d21-vulkan-atomic run-all DEVICE=0
make -C probes/d21-vulkan-atomic run-all DEVICE=1
```

- D23 binds the exact original shader's counter through the game's live
  `R16_UINT` typed view. This deliberately reproduces the invalid 32-bit
  atomic/16-bit-view combination; `output_status=CORRUPT` is the expected
  diagnostic observation rather than a probe failure.
- D24 uses a locally generated version of the same shader with only the
  counter translated as a 32-bit storage-buffer atomic. Correct output must
  again cover one global 8,160-entry range.

`EXACT_R16_SSBO_SPV` defaults to the local ignored D24 shader. It can be
overridden when that generated artifact is stored elsewhere. See
`docs/evidence-d21-d25-atomic-result.md` for the results and the later D25
in-game negative. That negative was subsequently shown to be incomplete: D25
emitted an SSBO atomic but still selected the typed descriptor. D45 and D47
paired the SSBO operation with the raw descriptor sibling and established the
allocator as the visual cause. See `docs/final-report.md` for the final
interpretation.
