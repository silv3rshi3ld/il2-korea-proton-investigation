# D46 allocator-only minimality control: result

## Result validity

D46 reproduces the block artifacts, but it is not a valid allocator-only
result. Later source review found that reverting the depth quirk also removed
the `IL2Series.exe` entry from `application_shader_quirks`. The allocator quirk
table remained compiled but unused, exactly matching the build warning that
`il2_korea_quirks` was defined but not used.

The Steam prefix `config_info` identifies
`IL2-Korea-D46-AllocatorOnly-1d5049e4`. The installed D46 DLL hashes were
verified against the package before the run. That proves the D46 package ran,
but not that its unwired quirk activated. Launch options were empty and no
RenderDoc layer or shader override was involved.

## Interpretation

D45 contains two independently motivated, shader-scoped compatibility
behaviors:

1. translate the exact allocator shader's invalid 32-bit typed-UAV atomic to a
   raw SSBO operation and select VKD3D-Proton's raw descriptor sibling;
2. bypass the common packed-mask and scalar tile-depth rejection in the two
   exact light-producer shaders while retaining geometric culling.

The first behavior repairs the demonstrated workgroup-local allocation and
changing overwritten light IDs. D46 accidentally tested neither behavior and
therefore cannot decide whether the second remains necessary after the first
is active.

D45 remains the complete tested graphics candidate: it was clean on two
independent starts, preserves real lighting and shadows, and requires no launch
parameters or game modification. Corrected D47 must restore only the allocator
quirk plus application wiring before minimal upstream scope is decided. The
fine sandy or film-grain lighting is normal on native Windows and remains
outside the defect.

Nothing has been posted or uploaded.
