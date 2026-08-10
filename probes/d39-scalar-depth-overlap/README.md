# D39 scalar-depth-only tiled-light membership

D39 keeps the valid geometric interval and the producer's direct scalar
near/far overlap test, but removes the redundant packed logarithmic depth-mask
requirement.

Conceptually:

```text
original = valid_geometry && packed_mask_overlap && scalar_depth_overlap
D38     = valid_geometry
D39     = valid_geometry && scalar_depth_overlap
```

The scalar comparisons are reconstructed from the exact original operations
and operands immediately before the final branch. Both the count and index
producers receive the same predicate. No light IDs, consumers, shadows,
records, or resource bindings change.

Build and validate with:

```sh
make -C probes/d39-scalar-depth-overlap
```

D39 is a local diagnostic until its visual result is known. Nothing is to be
posted or uploaded without explicit user confirmation.
