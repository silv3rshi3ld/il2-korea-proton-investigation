# D35 constant one-record-2 evaluation

D35 builds on accepted D34 and removes variable tiled-list behavior from the
two target consumers:

- the packed `t9` value becomes start 0, count 1 for every target pixel;
- both outcomes of D32's ID selection become record 2;
- record 2 is therefore evaluated exactly once, independent of real tile
  count, start offset, membership, or multiplicity;
- D34's fully-visible shadow result remains active;
- the complete ordinary record-2 distance, cone, material, and accumulation
  math remains active.

The `t10` fetch instruction remains but its extracted value cannot select a
different record. This is a causal diagnostic, not a final compatibility fix.
