# D14 exact light/reflection shader dump: preparation record

## Purpose

D13 does not show a simple missing clear, state transition, or UAV barrier. It
does identify four stable compute hashes around the 80x34x2 `R32_UINT`
light-reference grid and two stable reflection-pass pixel hashes immediately
after the full-resolution self-light target becomes shader-readable.

D14 passively dumps the original application DXIL and VKD3D-Proton's translated
SPIR-V for those exact hashes. It does not replace a shader, alter a descriptor,
insert a barrier, or apply an application override.

## Target hashes

Light-grid compute candidates:

```text
ce5553a11c1e3c3d
e41c75bf472dc42b
14096b77d9f7cb60
651194bd0a21772e
```

Reflection-pass shaders correlated by D12/D13:

```text
7ab2e6bc4f546a86  (vertex)
df0bd777fd1bb89d  (pixel)
a2d104d5c813322e  (pixel)
```

The dump is useful even without a new visual comparison. It can establish:

- the compute shaders' declared thread-group dimensions and therefore whether
  `10,5,*` dispatches cover all 80x34x2 cells;
- the original DXIL resource classes, dimensions, formats, and register use;
- SPIR-V image/view types and array/3D coordinates selected by dxil-spirv;
- signed/unsigned and bounds behavior around the two-layer integer grid; and
- whether the reflection shaders statically contain bindings compatible with
  self-light or the tiled light-reference/list resources.

Static disassembly still cannot reveal the actual descriptor chosen for a
dynamic bindless index. If that boundary remains ambiguous, the next step will
be a narrowly filtered descriptor trace or one frame capture.

## Build and safety identity

- Selected compatibility tool: `IL2-Korea-D13-LightUsage-395d9747`
- VKD3D-Proton build: `395d974767f56f1`
- Terrain-fix base: `cf11ba76a1cdbee`
- Wine NUMA base: exact MR !11604 D10 package
- D13 light telemetry gate: **off** for D14
- OpenMP/topology overrides: none

The D13 source changes are inert unless `VKD3D_IL2_LIGHT_TRACE=1` is set. D14
uses VKD3D-Proton's upstream `VKD3D_SHADER_DUMP_PATH` developer feature. That
feature forces pipeline-library SPIR-V reuse off so encountered shaders are
translated and dumped; this can make initial loading slower, but it does not
change shader code intentionally.

## Prepared run

The collector has a dedicated `shader-dump` variant. It creates a fresh,
ignored output directory so existing dumps cannot suppress writes:

```bash
./scripts/collect-proton-log.sh prepare D14-shader-dump-r1 shader-dump --no-openmp-override
```

The exact launch options are recorded in the prepared run directory. The run
uses:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D14-shader-dump-r1 VKD3D_SHADER_DUMP_PATH=/tmp/il2-D14-shader-dump-r1/shaders %command%
```

The run protocol is:

1. Start the game and wait until the affected menu aircraft is fully visible.
2. Leave that scene visible for about 10 seconds so all stable menu shaders are
   encountered.
3. Exit the game. No cockpit or fire reproduction and no screenshot are needed.
4. Collect the Proton log, then inspect only the seven target hash families in
   the local shader-dump directory.

Shader dumps are application code and remain local/ignored. They must not be
committed or uploaded. Only original analysis, hashes, resource signatures,
and short disassembly facts may enter the evidence repository.

## Decision rule

- If the original DXIL and translated SPIR-V disagree in resource dimension,
  integer type, coordinate width, bounds handling, or thread-group semantics,
  isolate that translator behavior in a minimal test before proposing a fix.
- If the translation is structurally faithful and full-grid coverage is clear,
  move to actual descriptor/view resolution rather than guessing a shader
  quirk.
- A visually unchanged D14 is expected and is not a negative result; this is a
  data-collection run, not a behavior-changing experiment.
