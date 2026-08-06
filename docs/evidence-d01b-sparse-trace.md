# D01b valid sparse-resource trace

- Run: `D01b-r1`
- Game build: `24596901`
- Compatibility tool: `IL2-Korea-Diagnostic-3dfc6f07`
- VKD3D source base: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- Diagnostic commit: `d0b4421f129b72e6127e6b9abd4028e8df946ea7`
- Visual result: unchanged; missing terrain pages remain at 1,921 m
- Instrumentation validity: **valid**

## Validity gates

- `IL2TRACE enabled` occurs exactly once.
- VKD3D reports build `3dfc6f07d0953b1+`; the `+` distinguishes the modified
  tree from the packaged build.
- All four post-run prefix DLL SHA-256 values exactly match the diagnostic
  build artifacts.
- D3D12/D3D12Core and DXVK DXGI load; no D3D11 module loads.

## API counts

| Instrumented call | Count |
|---|---:|
| Reserved-resource creation | 0 |
| `GetResourceTiling` | 0 |
| `UpdateTileMappings` | 0 |
| Submitted tile updates | 0 |
| `CopyTileMappings` | 0 |

The run covered both the corrupted menu and an in-mission reproduction. The
complete compressed log is 8,920,349 raw bytes and has SHA-256:

```text
4af6c9192ea8540bc34dda2d8956cb578d012ba7845d2a74b769c5ed8d8177f8
```

The screenshot
`D01b-r1-cockpit-bank-missing-terrain-pages-1921m.png` has SHA-256:

```text
704fa8a305693c8f10381ac29e684f2146843a8790b16ab8cb8378e336854111
```

## Conclusion

The game does not use the D3D12 reserved/tiled-resource APIs on the reproduced
path. The rectangular visual pattern is therefore produced by ordinary
resources or higher-level game texture paging, not D3D12 sparse residency.
Next instrumentation must census ordinary texture creation, mip-range views,
upload copies, and resource lifetime using stable cookies and bounded logs.
