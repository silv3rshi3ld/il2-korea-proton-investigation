# D35 constant one-record-2 evaluation: result

## Result

D35 removes the original square-grid artifact. The user reports that the scene
is "miles better" overall and that the squares are gone. Record 2's illumination
is spatially smooth when it is evaluated exactly once for every target pixel,
independently of the real tiled light list.

This is a causal result, not a final fix. D35 deliberately ignores the real
`t9` count/start and `t10` membership and retains D34's fully-visible shadow
control. It demonstrates that record 2 and its ordinary per-pixel spotlight
math do not inherently produce square output. The grid requires the real
tile-dependent placement or multiplicity of record 2.

Combined with the earlier controls:

- D27 retains real per-tile counts and multiplicity but substitutes safe
  record 1, and is clean;
- D32 retains real membership and evaluates genuine record 2 where listed,
  and is defective;
- D34 excludes the returned shadow visibility;
- D35 evaluates record 2 exactly once everywhere, and is clean.

The remaining causal boundary is therefore narrower than either "bad light
record" or "bad loop count": the square grid requires the real tile-dependent
membership/evaluation mask for record 2. The next source-level target is the
spotlight culling predicate shared by `ComputeLightsCount` and
`ComputeLightsIndices`, plus the exact list written under the D25 allocation
control.

## Runtime verification

- Run: `D35-global-record2-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D35-global-record2-r1/steam-247970.log`
- Log size: 6,758,225 bytes, 79,832 lines
- Log SHA-256:
  `86fb19d798553a375e4891ff391561e1e458488fbcf71f51caf8cd7647a1d8a5`
- Override markers: one for each target shader
- Shader/GPU failure signatures: zero
- Visual result: original squares gone; overall rendering much better

Screenshot evidence:

- path:
  `/home/USER/Pictures/Screenshots/Screenshot_20260809_190918.png`;
- dimensions: 599x260;
- SHA-256:
  `38d798bf23a0d5e25ece457ba9fe8b054a67ffbbe5221170f54e7f04ce37ca53`.

## Fine-grained film grain

The screenshot contains a sandy, fine-grained pattern on the fuselage which
the user reports fading in and out. It is visually distinct from the original
artifact: it is stochastic/fine-grained rather than aligned to the light-tile
grid, and the user identifies it as the only remaining visible concern in this
run. Earlier screenshots contain ordinary mottled surface detail but cannot
establish whether this animated pattern was already present. D35 applies record
2 outside its real membership and disables its shadow visibility result, so
the pattern is currently classified as unresolved: either a diagnostic side
effect or pre-existing temporal/material noise exposed by altered lighting.
It is not evidence that D35 is a finished visual fix.

After D36, the user relayed confirmation that the same film-grain-like
lighting appears in the Windows version. This is not a matched native capture,
but it establishes reported native parity and removes the effect from the
Proton-defect scope unless contradictory Windows evidence appears. It should
no longer be grouped with the square blocks.

Do not merge this pattern into the original square-grid diagnosis without a
normal-rendering reproduction. D35 is not suitable for upload or use as a
shipping workaround.
