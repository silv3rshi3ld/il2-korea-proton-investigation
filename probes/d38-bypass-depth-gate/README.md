# D38 conservative tiled-light depth-gate control

D38 preserves both producers' original light-volume calculations and requires
their merged near depth to be less than their merged far depth. It replaces
only the later membership decision which additionally intersects the light
with the tile's packed scene-depth mask and min/max depth.

Conceptually:

```text
original = valid_geometry && packed_depth_mask_overlap && depth_interval_overlap
D38     = valid_geometry
```

The special non-local-light path remains included because its merged interval
is `[0, 65000]`. Rejected local geometry remains excluded because its merged
interval is `[0, 0]`. No light ID is hard-coded, no light is duplicated, and
the count and index passes receive the identical predicate.

Build and validate with:

```sh
make -C probes/d38-bypass-depth-gate
```

The output directory must contain only the two producer overrides. D38 is a
local diagnostic, not yet an upload candidate.
