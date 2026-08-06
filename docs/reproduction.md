# Reproduction and controlled-run procedure

## Preconditions

1. Exit IL-2 and wait for its Proton processes to terminate.
2. Do not change Proton, Mesa, kernel, game files, firmware, prefix, display
   server, or graphics settings between repetitions.
3. Create and verify a prefix backup with `scripts/backup-prefix.sh`.
4. Run `scripts/collect-system-info.sh` and attach its output to the run batch.
5. Record the game build ID, Proton version, VKD3D commit, display resolution,
   graphics preset, anti-aliasing/upscaler settings, V-Sync state, and frame cap.
6. Keep the existing Steam/VKD3D shader caches unchanged for the initial
   matrix. Record if Steam performs shader pre-caching between runs.

The OpenMP variables remain constant for all graphics experiments. This keeps
the startup issue out of the graphics comparison.

## Fixed visual checks

Use the same menu state, aircraft, mission, weather, time of day, altitude,
heading, and camera for every comparison run. The completed E00-E02 batch did
not record the exact graphics preset and mission identifier, so it establishes
repeatability of the symptom and control results but is not a pixel-matched
reproduction. Record those missing values before any resumed A/B testing.

### R1: menu aircraft corruption

1. Start the game and wait 30 seconds after the main menu becomes interactive.
2. Leave the same aircraft selected; the existing evidence uses the menu's
   currently selected jet.
3. Do not rotate the menu camera after choosing the reference view.
4. Capture the entire frame and a crop covering the aircraft.
5. Record missing geometry, incorrect material, black/pink regions, or unstable
   squares as present/absent, with a short severity note.

### R2: shimmering or flickering squares

1. At the same menu camera position, observe for 30 seconds.
2. If possible, record a short video at the monitor's normal refresh rate.
3. Count whether artifacts are continuous, intermittent, or absent and note
   whether they affect aircraft, shadow, background, or UI.
4. Do not use a still image alone to classify a temporal improvement.

### R3: terrain texture loading failure

1. Load the same flight used for the existing evidence, in the Singo-dong map
   area, or record a new fixed mission identifier before starting a new batch.
2. Record aircraft, weather, time, initial position, altitude, and heading in
   the run observations.
3. Keep the external/cockpit camera and field of view unchanged.
4. Capture at T+0, T+30 s, and T+60 s after entering the mission.
5. Record the approximate distance at which textured terrain first becomes
   visible and whether low-resolution/absent chunks later resolve.
6. Capture a paused external view near 1,500 m and another near 5,000 m at the
   same approximate landmark, heading, and field of view.

### R4: distant terrain black, hollow, or absent

1. Continue the same mission without changing graphics settings.
2. Point the fixed camera toward the recorded Singo-dong landmark and heading.
3. Capture horizon, middle distance, and near terrain in one frame.
4. Record black or hollow areas, missing geometry, pink/purple chunk seams, and
   whether the boundary follows terrain chunks.

Altitude is now a controlled variable rather than incidental metadata. See
[`altitude-observation.md`](altitude-observation.md).

## Result classification

Use only these final outcome values in the matrix:

- **fixed**: all relevant artifacts disappear in both runs;
- **improved**: a repeatable material reduction, with at least one defined
  metric or matching before/after capture;
- **unchanged**: no repeatable visual or performance difference;
- **regressed**: a repeatable new/worse artifact, crash, hang, or material
  performance loss;
- **inconclusive**: runs disagree, evidence is incomplete, or another variable
  changed.

## Preparing and collecting a run

For each repetition, prepare a unique run ID and paste the printed line into
Steam Launch Options:

```bash
./scripts/collect-proton-log.sh prepare E00-baseline-r1 baseline
```

After exiting the game:

```bash
./scripts/collect-proton-log.sh collect E00-baseline-r1
```

The script keeps the full compressed Proton log under ignored `captures/`,
extracts a filtered diagnostic log, records checksums and selected component
versions, and reports the D3D/VKD3D/queue lines it found. Do not commit the full
log without reviewing it for local paths and size.

## Experiment variants

The completed batch stopped after E02. If testing resumes, run two repetitions
of each single-variable variant before any combined test:

```bash
./scripts/collect-proton-log.sh prepare E00-baseline-r1 baseline
./scripts/collect-proton-log.sh prepare E01-no-upload-hvv-r1 no-upload-hvv
./scripts/collect-proton-log.sh prepare E02-single-queue-r1 single-queue
./scripts/collect-proton-log.sh prepare E03-no-descriptor-buffer-r1 no-descriptor-buffer
./scripts/collect-proton-log.sh prepare E04-no-upload-hvv-single-queue-r1 no-upload-hvv-single-queue
```

Use `r2` only after completing and recording `r1`. Do not combine descriptor
buffer disabling with another flag in this first matrix.

## D01 focused resource trace

D01a proved that copying DLLs directly into the prefix is ineffective because
stock Proton restores its packaged copies during launch. Create the dedicated
custom tool using the command in [`development-build.md`](development-build.md),
restart Steam, and explicitly select `IL2-Korea-Diagnostic-3dfc6f07` for this
game. Then prepare a fresh run with:

```bash
./scripts/collect-proton-log.sh prepare D01b-custom-proton-sparse-trace-r1 resource-trace
```

For this diagnostic gate, reach the menu and load the established Singo-dong
mission far enough to reproduce the missing terrain. A new screenshot is not
required unless the rendering unexpectedly changes. Exit normally and collect
the log. The collector reports whether the trace gate was active and counts
reserved-resource creation, tiling queries, tile updates, and tile copies.

This run is not an A/B visual experiment: the DLL behavior is unchanged. Its
first validity gate is an `IL2TRACE enabled` marker; no API result may be
interpreted without it. Its diagnostic question is then whether the failing
path uses D3D12 tiled/reserved resources at all. One representative valid run
is sufficient to answer that binary API-use gate.

## D02 ordinary texture trace

D01b excluded the D3D12 reserved-resource API path. D02 uses a separately
named custom Proton tool containing diagnostic commit `54797ad3`; do not
replace the D01 tool. Select the D02 tool for AppID 247970, then prepare:

```bash
./scripts/collect-proton-log.sh prepare D02-r1 texture-trace
```

The preparation script creates a short `/tmp/il2-D02-r1` symlink so the Steam
launch option is not vulnerable to line wrapping inside a long log path. Paste
the printed launch option exactly.

In-game:

1. Wait at the main menu until the block artifacts are visible on the aircraft.
2. Load the same Singo-dong mission used for D01b.
3. Reach roughly 1,900-2,500 m and bank until the missing rectangular ground
   pages and magenta borders are visible. A higher-altitude pass is useful but
   not required if the defect is already clear.
4. Keep the defective view visible for about 15 seconds, then exit normally.

Collect with:

```bash
./scripts/collect-proton-log.sh collect D02-r1
```

Collection is automatic: it validates/counts the `IL2TEX` markers, compresses
the full log, and writes `texture-trace-analysis.md` with the most common
texture shapes, normalized SRV mip ranges, copy classes, active resource
cookies, and a lifetime-ordering diagnostic. The first validity gate is
exactly one `IL2TEX enabled` marker plus post-run diagnostic DLL hashes. This
is an API census, not a visual A/B fix test; one valid representative run is
enough before narrowing the trace.

D02-r1 is complete. Do not repeat it unless the game, Proton, VKD3D-Proton, or
Mesa baseline changes. Its result and screenshot checksum are recorded in
[`evidence-d02-ordinary-texture-trace.md`](evidence-d02-ordinary-texture-trace.md).

## D03 placed-resource alias trace

Select `IL2-Korea-D03-Alias-Trace-cfca234e` for AppID 247970. `D03-r1` is
already prepared, so paste the launch option stored in
`captures/runs/D03-r1/launch-options.txt` exactly.

In-game:

1. Wait for the menu aircraft artifacts to appear.
2. Load the same Singo-dong mission.
3. Reproduce the rectangular missing ground pages near 1,300-1,500 m.
4. Keep the defective view visible for approximately 15 seconds and take one
   screenshot showing the HUD altitude.
5. Exit the game normally.

Then collect:

```bash
./scripts/collect-proton-log.sh collect D03-r1
```

Collection checks both trace gates and generates `texture-trace-analysis.md`
and `alias-trace-analysis.md`. One valid run is sufficient for this API/range
question; do not repeat it before reading the generated overlap result.

## Performance notes

Record menu FPS, in-mission FPS, and obvious frame-pacing changes from the same
30-second windows. If frame-time capture is introduced later, first capture a
fresh baseline with exactly the same overlay/layer because enabling an overlay
is itself another variable.

## Prefix rollback

Restore only while Steam and the game are stopped:

```bash
./scripts/restore-prefix.sh \
  --archive /absolute/path/il2-korea-247970-prefix-YYYYmmddTHHMMSSZ.tar.zst \
  --yes
```

The old target is retained alongside the restored prefix as
`247970.pre-restore-*`. Inspect it before deleting anything manually.

## Custom VKD3D-Proton DLL rollback

An install prints its backup directory. Restore that exact backup with:

```bash
./scripts/install-vkd3d-build.sh restore \
  --backup-dir /absolute/path/to/vkd3d-dll-backup-* \
  --yes
```

The script backs up the currently installed DLLs again before restoration. It
never overwrites the Proton installation itself.
