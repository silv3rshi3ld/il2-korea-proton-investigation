# D37 finite-`rsqrt` tiled-light producer control: preparation

## Question

D37 asks one new question:

> Do the large square lighting artifacts disappear when the exact tiled-light
> producer shaders retain all original culling logic but prevent reciprocal-
> square-root infinities from contaminating that logic with NaN?

This is not another light-ID filter. It is a normal VKD3D-Proton hashed shader
quirk using behavior already present upstream as
`FIXUP_RSQRT_INF_NAN`.

## Why this follows from D27-D36

CPU-only decoding of the retained D33 `t7` buffer establishes the active
producer classes:

| Record | Type field | Producer branch |
| --- | ---: | --- |
| 0 | 1 | non-local/sentinel path |
| 1 | 3 | cone/spotlight culling |
| 2 | 3 | cone/spotlight culling |
| 3 | 3 | cone/spotlight culling |
| 4 | 2 | spherical culling |

D30 proves that genuine tile placement of record 1 alone can reproduce the
blocks. D32 proves the same for record 2 in a controlled mixture. D27 and D35
become clean only when their genuine sparse membership is replaced. D36 fixes
only record 2's membership and therefore leaves the other type-3 records able
to reproduce the boundary. D28 excludes record 4 as necessary.

The shared type-3 producer path normalizes several constructed vectors. Both
exact producer DXIL modules contain 12 fast FP32 reciprocal-square-roots.
Vulkan `InverseSqrt(0)` may yield infinity; multiplying that result by a zero
vector component can yield NaN. Ordered geometric comparisons then reject the
light from a tile, even though later per-pixel attenuation remains valid. A
tile-level false negative produces exactly the observed block boundary.

The existing quirk changes only this exceptional arithmetic behavior. It
clamps each `rsqrt` result to finite `FLT_MAX`, matching a behavior that some
Windows driver stacks tolerate for invalid or degenerate game math. This is
the conventional compatibility approach: ideally the game would avoid
normalizing a zero-length vector, but in practice a narrowly scoped runtime
allowance can reproduce the behavior on which the Windows version relies.

## Static contract

The local quirk table contains only:

```text
651194bd0a21772e FIXUP_RSQRT_INF_NAN
11e32439a86036ba FIXUP_RSQRT_INF_NAN
```

Static translation of the retained exact DXIL with quirk index 10:

- succeeds for both producer modules;
- preserves 12 `InverseSqrt` operations per module;
- increases `NMin` operations from 15 to 27 per module, exactly one clamp per
  reciprocal square root;
- validates both results for Vulkan 1.3.

The retained static outputs are:

| Hash | Bytes | SHA-256 |
| --- | ---: | --- |
| `651194bd0a21772e` | 37,440 | `c465d952a4a08fe301b8dd36520b387e8cec676c70825227f2e38acad87ead92` |
| `11e32439a86036ba` | 37,968 | `28d2434b31eeced00426049d7760992d01dcd9683fd36de17dd800684fc684e6` |

The two-line runtime quirk file has SHA-256
`f9b3bbdbdf304eb5c1482faa3ab73ed9a954f4b14b1181d8f4bba63021c20100`.

The test changes no descriptor, light ID, membership predicate, resource
format, consumer shader, shadow behavior, or render setting.

## Runtime control

Select `IL2-Korea-D25-LightAtomicCompat-84c87c83`, ensure no old
`VKD3D_SHADER_OVERRIDE` remains, and use:

```text
PROTON_LOG=1 PROTON_LOG_DIR=/tmp/il2-D37-rsqrt-compat-r1 VKD3D_SHADER_QUIRKS=/home/USER/Documents/Codespace/Protondb/il2-korea-proton-investigation/probes/d37-rsqrt-compat/il2-rsqrt-quirks.conf VKD3D_SHADER_DUMP_PATH=/tmp/il2-D37-rsqrt-compat-r1/shaders %command%
```

The decisive observations are:

- whether the original large square/block artifact is absent;
- whether ordinary dynamic lights and shadows remain present;
- whether the log selects `FIXUP_RSQRT_INF_NAN` for exactly the two producer
  hashes and contains no shader or device failure.

The fine animated film-grain/sandy lighting is excluded from the success
criterion because the user received confirmation that it also occurs on
Windows.
