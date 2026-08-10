# D32 isolate genuine light ID 2: result

## Result

The square artifact remained when genuine light ID 2 selected record 2 and
every other list entry selected the known-safe record 1. No iteration took the
zero/sentinel skip path.

Record 2 is therefore sufficient to reproduce the visible grid in this
controlled record-1/record-2 mixture. This is a causal record-selection result,
not yet proof that the record data itself is valid or that any particular
operation inside the record-2 path is wrong.

## Run verification

- Run: `D32-isolate-light-id2-r1`
- Compatibility tool: `IL2-Korea-D25-LightAtomicCompat-84c87c83`
- Log: `/tmp/il2-D32-isolate-light-id2-r1/steam-247970.log`
- Log size: 6,232,977 bytes
- Log SHA-256:
  `5c8f39229aad1ae4aebbc1514b643f4c0615803127340aaa9a6082724b534c97`
- Visual result reported by the user: squares present
- Game and Steam were closed after classification

Each intended override loaded exactly once:

```text
Overriding shader hash df0bd777fd1bb89d .../d32-isolate-light-id2-overrides/df0bd777fd1bb89d.spv
Overriding shader hash a2d104d5c813322e .../d32-isolate-light-id2-overrides/a2d104d5c813322e.spv
```

The inspected log contains no shader-module creation failure, Vulkan device
loss, GPU fault, or validation-error signature.

## Causal boundary

D32 keeps the real tile count/start, list traversal, and number of complete
per-light evaluations. Its only selection rule is:

```c
filtered_id = light_id == 2 ? 2 : 1;
```

Together with clean D27, this establishes:

- common loop traversal and repeated accumulation are not sufficient;
- record 1 is safe in the controlled path;
- zero/sentinel handling is not required;
- record 2 is sufficient.

It does not distinguish an invalid or undersized `t7` view from malformed
record data, fragile application math, DXIL-to-SPIR-V semantics, or driver
execution.

## Next evidence gate

Do not continue with record-field visual bisections. Resolve the exact live
`t7` SRV contract first: resource and view size, format, first element, element
count, root-signature bounds policy, backing allocation, and captured record
contents. Only after that evidence should one targeted behavior change be
tested.

D32 is diagnostic evidence, not a fix or upload candidate.
