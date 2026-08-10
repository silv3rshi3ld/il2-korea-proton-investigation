# D36 global record-2 producer membership

D36 preserves the original tiled-light consumers, shadows, and all real light
IDs. It modifies only the matching decision in the two producer shaders:

```text
inclusive_membership = original_membership || light_id == 2
```

The same predicate is applied to `ComputeLightsCount` and
`ComputeLightsIndices`, so record 2 is added exactly once to every valid tile
without duplicating an existing entry. Other lights retain their original
membership.

The default inputs are the retained D14 producer dumps. D25 changes only
`ComputeLightsFirstRef`; these two producer binaries are therefore the exact
unmodified inputs used by the D25 runtime path. The D25 compatibility tool
must remain selected during the visual test so the enlarged list receives
non-overlapping allocation ranges.

This is a causal diagnostic, not a shipping quirk. The output directory must
contain only the two producer modules; do not combine it with D34 or D35
consumer replacements.
