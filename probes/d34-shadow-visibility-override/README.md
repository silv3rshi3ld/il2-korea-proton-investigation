# D34 record-2 shadow-visibility discriminator

D34 is built on the D32 control. Genuine light ID 2 still selects record 2;
every other list entry still selects known-safe record 1. It preserves record
2's position, color, range, cone, diffuse/specular math, and accumulation, but
replaces the final shadow visibility operand with `1.0` (fully visible).

The seven comparison-sampling instructions remain in the module, although a
later driver optimization may remove their now-unused result. The generator
requires the exact D32 ID-selection instructions, the expected visibility
multiply, one float-one constant, and seven comparison samples before changing
one existing operand. Module size and ID bound do not change.

Interpretation:

- squares disappear while record-2 lighting remains: shadow
  projection/comparison is causal;
- squares remain: ordinary record-2 spotlight math is causal and the shadow
  result is not required;
- record-2 lighting disappears or rendering breaks: the test is invalid.

This is a local diagnostic shader override, not a mod and not a final fix.
