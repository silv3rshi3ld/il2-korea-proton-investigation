# D40 packed-mask-only tiled-light membership

D40 keeps valid geometric light intervals and the packed logarithmic depth
mask, but removes the direct scalar tile min/max overlap which D39 proves can
restore the original blocks.

```text
original = geometry && packed_mask && scalar_overlap
D38     = geometry
D39     = geometry && scalar_overlap
D40     = geometry && packed_mask
```

Both producer passes use the same predicate. No light ID, consumer, record,
shadow operation, or resource binding changes.

Build and validate with:

```sh
make -C probes/d40-packed-mask-only
```

D40 is a local diagnostic until its visual result is known. Nothing is to be
posted or uploaded without explicit user confirmation.
