# D27 fixed-light-ID override generator

D27 keeps the original per-tile `t9` light count and start offset, executes the
original number of loop iterations, and preserves all per-light calculations.
It changes only the scalar ID extracted from each `t10` fetch to `1`, the first
non-sentinel light ID used by the game's index producer.

This distinguishes the real `t10` list contents and ID selection from the
subsequent per-light data/math and count-dependent accumulation:

- grid disappears: actual `t10` IDs/list interpretation is required;
- grid remains: the ID list itself is not required, selecting the per-light
  data/math or repeated count-dependent contribution;
- unclassifiable lighting: refine the test rather than infer a fix.

The binary patch replaces one five-word `OpCompositeExtract` with a five-word
`OpBitwiseOr(1, 0)`. It verifies the complete original instruction signature,
requires exactly one match, and preserves all other words. The override is a
local VKD3D developer diagnostic, not a game mod or final compatibility fix.

Generate and validate the ignored outputs from the exact local D25 shader dump:

```text
make -C probes/d27-fixed-light-id-override
```
