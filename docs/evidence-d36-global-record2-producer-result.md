# D36 global record-2 producer membership: result

## Result

D36 is a valid visual negative. The original square blocks returned even
though record 2 was conservatively included in every valid tile by both
producer passes.

This rejects the narrow hypothesis that false-negative producer membership of
record 2 alone explains the artifact. It also clarifies why D35 looked clean:
D35 did more than remove record-2 tile boundaries. It replaced the complete
real list with one record-2 evaluation per pixel. D36 removed record 2's
producer boundary while restoring every other genuine light and the original
consumer/shadow behavior, and the blocks returned.

The remaining boundary is therefore real light-record diversity, the tiled
placement of another nontrivial record, or an interaction between real
records. It is not useful to repeat the earlier ID-by-ID consumer filters:
D28-D32 already establish that record 2 is sufficient while tile-masked, but
they do not prove the other records harmless after record 2 becomes global.
The next work is offline analysis of the producer's shared culling logic by
light class.

## Runtime verification

- Run: `D36-global-record2-producer-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D36-global-record2-producer-r1/steam-247970.log`
- Log size: 6,237,503 bytes, 75,048 lines
- Log SHA-256:
  `713023656305260747cd7fff371b6b36ce95e222c3ad6771d2b476e8f2ab42d6`
- Visual result reported by the user: artifacts/blocks returned
- Shader/GPU failure signatures: zero

The log contains exactly one replacement for each intended producer:

```text
651194bd0a21772e  ComputeLightsCount
11e32439a86036ba  ComputeLightsIndices
```

It contains no D35 replacement for consumer hashes `df0bd777fd1bb89d` or
`a2d104d5c813322e`. The initially reported clean observation came from a
separate launch which the log proved was still using D35; it is not counted as
D36 evidence.

## Fine-grained film grain

The user subsequently received confirmation that the fine sandy/film-grain
lighting visible on the fuselage also occurs in the Windows version. This is a
user-relayed native-parity report rather than a matched capture, but it removes
the effect from the Proton-defect scope unless contradictory Windows evidence
appears. Only the square blocks remain under investigation.

D36 is diagnostic evidence, not a compatibility fix or upload candidate.
