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
