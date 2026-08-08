# D20 tiled-light value capture: preparation record

## Purpose

D15-D19 exclude the broad synchronization, binding, compression-only, and ACO
wait-state explanations for the light-aligned square artifact. D20 captures one
queue submission containing exact consumer pixel shader
`df0bd777fd1bb89d`. The goal is to inspect the values in its `t9` tiled-light
grid and `t10` light-index buffer at the producer-to-consumer boundary.

This is diagnostic capture support, not a game modification, application
override, launch workaround, or proposed fix.

## Capture mechanism

VKD3D-Proton already contains shader-targeted RenderDoc support. The D20 source
changes only the Meson default for `enable_renderdoc` from false to true. The
feature remains inactive unless the explicit auto-capture variables are set.

When the target shader is recorded, VKD3D-Proton marks its command list and
wraps the selected queue submission in RenderDoc capture markers. This is much
narrower than capturing the whole game and changes no shader or D3D12 command.

The target is the D14/D16 pixel shader `df0bd777fd1bb89d`, which reads:

- `t9`: cookie 4002, `rtLightRefs25`, `80x34x2 R32_UINT` Texture3D;
- `t10`: cookie 4001, 87,040-byte buffer viewed as 43,520 `R16_UINT`
  elements, or sixteen indices per screen tile.

## Built artifacts

- Source: `src/vkd3d-proton-menu-trace`
- Source commit: `5735f64f643236a8cd189297e56e4015bcdf3c55`
- Parent diagnostic commit: `274f6f8e2d5b785fa871cedb0e3267e6a2af9abf`
- Build: `build/vkd3d-proton-il2-d20-renderdoc-5735f64f`
- Build method: official `package-release.sh --dev-build`, x86-64 and x86
- Meson verification: `enable_renderdoc=true` for both architectures

DLL SHA-256:

```text
b0f0893a0237a0cbf9459218a3cf9fcccf9f7615d46ed4c1be241b0e39e1e985  x64/d3d12.dll
9e5f443cccdb5ace6d87f0266bc1b4ff5c8e6ec931f378bb7cd02177bd7cd3a5  x64/d3d12core.dll
31725dabd78a2de40f135e5b7c48fc0ece1d63446daf9e12b1ba6e9f56abd9ab  x86/d3d12.dll
adbfbe0236b06e36313ad3b20ca451b0658b4b3336f3d2b333778d8d9c46c2ec  x86/d3d12core.dll
```

The built `d3d12core.dll` contains the expected
`VKD3D_AUTO_CAPTURE_SHADER` and `capture-marker,begin_capture` strings.

The custom tool was created at
`IL2-Korea-D20-RenderDoc-5735f64f` from the D16 tool after Steam exited. All
four installed DLL hashes match the build hashes above. The source D16 tool and
the game prefix were not modified.

## Local RenderDoc tool

RenderDoc 1.45 was downloaded from the CachyOS package repository and unpacked
under the ignored `build/` tree. It was not installed system-wide and no Vulkan
layer registration was added to the user account.

- Version: `1.45`, commit `2fc0bc04cb95499635f63986a55bc6f67849dd9f`
- Package SHA-256:
  `6f6b4df21cca642a3e3cb3c2b818b0a887a6d2c9daa1da2d87ac097269e7ef8a`
- APIs reported: Vulkan, GL, GLES

## Run protocol

Steam must be fully stopped before the D20 custom Proton tool is created and
before the capture launcher is used.

1. Create `IL2-Korea-D20-RenderDoc-5735f64f` by cloning the D16 custom tool and
   replacing only its four VKD3D-Proton DLLs with the verified D20 build.
2. From the repository root, run:

   ```text
   ./scripts/launch-d20-renderdoc.sh
   ```

3. In the Steam session it opens, select that compatibility tool for AppID
   247970 and use this ordinary launch line only for the Proton log:

   ```text
   PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D20-renderdoc-light-values-r1 %command%
   ```

4. Start IL-2. Wait until the affected menu
   aircraft appears and the capture finishes, then close the game normally.
5. Exit Steam before collecting the Proton log and opening the capture.

The launcher sets these variables only for its Steam process tree:

```text
ENABLE_VULKAN_RENDERDOC_CAPTURE=1
VKD3D_AUTO_CAPTURE_SHADER=df0bd777fd1bb89d
VKD3D_AUTO_CAPTURE_COUNTS=0
```

The expected log must show RenderDoc capture enabled, the exact target shader,
and a trigger for the matching command list. The run is invalid if the D20
build identity or capture marker is absent.

## Analysis gate

The first capture question is factual: do `t9` and `t10` contain internally
plausible values at the affected draw?

- `t9` tile counts/offsets must stay within the sixteen-index-per-tile layout.
- `t10` indices must stay within the available light-data range used by the
  consumer.
- Repeated square groups, impossible counts/offsets, out-of-range indices, or
  values inconsistent with the corresponding lit regions select the producer
  or typed-buffer translation path.
- Plausible inputs select the consumer's tile-coordinate/value interpretation
  or later reflection/light-composition output.

The `.rdc` may contain captured game resources. It stays local and ignored and
must not be uploaded or attached to a public report without a separate content
review and explicit user confirmation.

Nothing from D20 will be uploaded or posted without explicit confirmation.
