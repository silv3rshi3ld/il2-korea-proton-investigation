# D38 conservative tiled-light depth-gate control: result

## Result

D38 removes the original large blocky artifacts. The user reports no blocky
artifacts with the conservative geometric-membership predicate. Real lighting
remains present, although the user observes a smaller amount of light
flickering.

This is the first producer-side result which removes the original blocks while
retaining genuine light IDs, consumer loops, ordinary per-light math, and
shadow behavior. It positively isolates the large square artifact to the
common packed tile-depth rejection that follows the type-specific geometric
light-volume intersection.

The fine sandy/film-grain-like temporal lighting has since been confirmed to
occur in the native Windows renderer and is accepted as normal. However, the
original report that the light "flickers a little" was too broad to equate
entirely with that film grain. Later integrated D42 and D43 tests distinguish
larger aircraft-light flicker from the native fine-grain effect. That broader
flicker remains a failure condition.

## Runtime verification

- Run: `D38-depth-gate-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D38-depth-gate-r1/steam-247970.log`
- Log size: 8,229,833 bytes, 89,690 lines
- Log SHA-256:
  `be6d137b03e5d0f4f8831c097479c8ab51c05a783266d85329a4fa2521984024`
- Visual result reported by the user: no blocky artifacts; light flickers a
  little
- D38 producer overrides: exactly two
- D37 finite-`rsqrt` quirks: zero
- Shader/device failure: zero

The log loads exactly one override for `ComputeLightsCount`
`651194bd0a21772e` and one for `ComputeLightsIndices`
`11e32439a86036ba`. No consumer override is active.

## Causal conclusion

For the covered scene, the original predicate can be summarized as:

```text
valid light-volume interval
&& packed logarithmic depth-mask overlap
&& tile min/max depth overlap
```

D38 retains the first term and removes the two depth terms. The original
blocks disappear. Therefore at least one of the packed depth mask or min/max
depth-overlap terms is required for the original artifact. Earlier X/Y light
geometry alone is not required for those large blocks.

D39 and D40 subsequently showed that retaining either the scalar depth overlap
or the packed mask overlap restores the blocks. D38 is therefore the minimum
tested predicate for removing the spatial blocks, but D42 later shows it is
not yet a complete temporal-lighting fix. No attempt should be made to remove
the native film-grain lighting; the distinct broad flicker still requires
value-level investigation.
