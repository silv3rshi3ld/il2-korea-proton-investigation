# D08 general block-unit conversion result

## Result

D08 validates the clean general VKD3D-Proton fix at `cf11ba76`. The game
loaded the dedicated `IL2-Korea-D08-GeneralFix-cf11ba76` tool and identified
VKD3D-Proton build `cf11ba76a1cdbee`. The launch environment contained no D07
diagnostic gate, and the full log contains no `IL2BCCOPY` marker.

Terrain is continuous and detailed in supplied views at 4,813 m, 2,427 m, and
742 m. The former black/hollow rectangular pages and magenta page seams are
absent. The user withdrew the earlier fidelity concern after this controlled
run and classified the terrain as looking great.

The main-menu aircraft still has visible rectangular/block artifacts, and the
user confirms that shimmering remains. The terrain and menu symptoms are
therefore separate defects. D08 fixes the terrain only.

## Runtime identity and controls

- Game build: `24596901`
- Proton base: `experimental-11.0-20260724c`
- Candidate commit: `cf11ba76a1cdbee2daa4d5403913e596fb29938f`
- DXVK: `1a5919b7edd111887648d1e8bf0c32733e2e00d3`
- Mesa/RADV: `26.1.6`
- D3D12/D3D12Core module lines: 2
- DXGI module lines: 2
- D3D11 module lines: 0
- D07 enable/adjustment markers: 0
- Split `END_ONLY` warnings: 29,108
- GPU device loss, reset/hang, or OOM signature: none

The same missing-file fallbacks recorded in the successful D07 runs remain in
the game's `tex.log`. Correct terrain with those fallbacks and split-barrier
warnings still present excludes both as necessary causes of the rectangular
terrain failure.

## Patch validation

The general fix converts placed-buffer footprint geometry through physical
block counts when source and destination formats have equal physical block
sizes. It contains no IL-2 executable name, Steam AppID, resource dimensions,
or diagnostic environment variable.

The focused `64x64 R32G32B32A32_UINT` to `256x256 BC3` regression reports four
deterministic failures with the old helper and passes all 22 assertions with
`cf11ba76`. Neighboring compressed-copy tests pass 147/147 and 50/50. D07-r1,
D07-r2, and this clean D08 build all produce the same terrain repair.

This establishes high confidence that the missing block-unit conversion is
causal for terrain and that the general VKD3D-Proton patch is the correct
remedy. It does not resolve or explain the menu aircraft corruption.

## Screenshots

The local evidence copies remain ignored by Git until the user selects what to
publish:

| File | Observation | SHA-256 |
|---|---|---|
| `D08-r1-menu-aircraft-block-artifacts-shimmering-persists.png` | menu defect unchanged | `b48424b508b1a481d305fbc74a73b01cc63794d8c9d0f75ee588541b9b345c32` |
| `D08-r1-terrain-repaired-cockpit-4813m.png` | distant terrain fixed at 4,813 m | `4f2c221c7b3bfb9bc25b69beca3ae0446dd67024743e870f1c53f9537207a789` |
| `D08-r1-terrain-repaired-dive-2427m.png` | terrain fixed at 2,427 m | `78217f416f0262903f8c5aa1e22ea74c529d0b6212fd6a13e830f9e0c05fcbf1` |
| `D08-r1-terrain-repaired-low-altitude-742m.png` | terrain fixed at 742 m | `2c0b1e25bd394c192ba9b33e7e387ae7d521afcf0386ee629fda5d4a7d711900` |

The collected run is retained locally under
`captures/runs/D08-general-fix-r1/`. Nothing from this result has been posted
or pushed automatically.
