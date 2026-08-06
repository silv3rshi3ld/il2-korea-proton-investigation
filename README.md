# IL-2 Korea Proton compatibility investigation

This repository tracks a controlled investigation of **Korea. IL-2 Series**
(Steam AppID 247970). It deliberately separates the startup/NUMA failure from
the D3D12 rendering corruption. No application override has been added.

## Current status

The launch-option matrix E00-E02 is complete as of 2026-08-06. The
investigation has now moved to source-level diagnosis; no application override
or behavior-changing source patch has been added. The verified environment is:

- Launch executable: `bin/game/IL2Series.exe`
- Steam library: `/home/silv3rshi3ld/.local/share/Steam`
- Game directory: `/home/silv3rshi3ld/.local/share/Steam/steamapps/common/IL2Series`
- Prefix: `/home/silv3rshi3ld/.local/share/Steam/steamapps/compatdata/247970`
- Game build ID: `24596901` (Steam auto-update on 2026-08-06; prior controlled
  build was `24577563`)
- Selected compatibility tool: Proton Experimental
  `experimental-11.0-20260724c` (`11.0-100` prefix)
- VKD3D-Proton commit: `3dfc6f07d0953b1e8b41705275c2c59cc7374fc5`
- DXVK commit: `1a5919b7edd111887648d1e8bf0c32733e2e00d3`
- Mesa/RADV: `26.1.6` (`Mesa 26.1.6-arch3.1`)

`IL2Series.exe` imports the game's `dxBackend12.dll`. That backend imports
`d3d12.dll` and `dxgi.dll`; the prefix copies hash-identically to the selected
Proton build's 64-bit VKD3D-Proton D3D12 DLLs and DXVK DXGI DLL. A DXVK
`d3d11.dll` is installed in the prefix but is not statically imported by the
D3D12 backend. The completed controlled runtime logs with module evidence load
DXGI, D3D12, D3D12Core, and `dxBackend12.dll`, with no D3D11 module load.

The currently required startup mitigation is:

```text
OMP_NUM_THREADS=16 KMP_AFFINITY=disabled %command%
```

This is recorded as a mitigation, not a root-cause fix.

Controlled baseline E00 is complete across two runs and reproduces both
reported graphics signatures: block artifacts on the rendered menu aircraft
and severe rectangular-page terrain loss with magenta edges in flight. Runtime logging
confirms the D3D12 path and active host-visible upload, descriptor-buffer, and
multi-queue paths. No causal path has been isolated. See
[`docs/evidence-e00-baseline.md`](docs/evidence-e00-baseline.md).

E01 (`no_upload_hvv`) was collected across two runs, but its apparent vegetation
improvement is confounded by altitude. The corruption is consistently worse
near 5,000 m and more low-fidelity content appears near 1,500 m across multiple
configurations. E01 is therefore inconclusive pending matched-altitude A/B.

E02 (`single_queue`) is complete and unchanged across two runs. Disabling
asynchronous compute/transfer queues does not remove either defect.

E03 (descriptor buffer disabled) and E04 (`no_upload_hvv,single_queue`) were
not run. They remain explicitly inconclusive rather than being counted as
unchanged. The user reports that below roughly 1,500 m some low-fidelity assets
begin to load; around 5,000 m the failure is much more severe. Current logs do
not expose the relevant altitude, mip, tile-mapping, or residency state.

The current root-cause confidence is **low**, but D02 has narrowed the next
step. During a valid corrupted run, 2,355 multi-mip compressed textures received
geometrically complete buffer uploads, no partial mip chain was found, every
logged SRV used a zero minimum-LOD clamp, and no logged operation followed
resource destruction. A separate class of 433 placed BC3 textures received an
SRV but no logged incoming upload/copy. D03 will first determine whether those
resources overlap live buffers/textures or participate in explicit alias
barriers. Actual descriptor use is a follow-up only if that result requires it. See
[`docs/evidence-d02-ordinary-texture-trace.md`](docs/evidence-d02-ordinary-texture-trace.md)
and [`patches/README.md`](patches/README.md).

D03 is prepared at diagnostic commit `cfca234e`. A separate custom Proton tool
will correlate those same resource cookies with placed buffer/texture heap
ranges, lifetimes, and explicit alias barriers. It remains telemetry-only; no
descriptor or aliasing workaround has been enabled. See
[`docs/evidence-d03-preparation.md`](docs/evidence-d03-preparation.md).

An unmodified development build of the exact installed VKD3D-Proton commit was
built successfully, but D00 did not validate it: stock Proton recopies its own
packaged D3D12 DLLs into the prefix during every launch. Steam then auto-updated the game from build
`24577563` to `24596901` and restored the prefix's Proton-supplied D3D12 DLLs
before D01 ran. U00 confirms the updated game has the same corruption and the
same Linux D3D12 path. A later D01 prefix-install attempt was also overwritten;
its absent trace marker and post-run DLL hashes prove the method ineffective.
The verified `IL2-Korea-Diagnostic-3dfc6f07` custom Proton tool is now created,
and D01b has completed successfully. The trace marker and post-run hashes prove
that the diagnostic DLLs ran. Zero reserved-resource or tile-mapping calls were
made, excluding D3D12 sparse/tiled resources from the failing path. Read-only
inspection of the compiled game/backend binaries independently points to the
game's ordinary committed/placed textures, async `UpdateSubresource`, mip SRVs,
and copy operations. D02 telemetry at local commit `54797ad3` has now completed
in its separately named custom Proton tool. The trace is valid and the visual
failure remains at 1,385 m; its findings select a focused heap/alias/descriptor
D03 trace rather than an application override. See
[`docs/development-build.md`](docs/development-build.md).

The user also confirms that the game renders correctly on native Windows. This
establishes Windows as the functional control and confines reproduction to the
Linux compatibility/driver path. It does not by itself prove that every D3D12
operation issued by the game is specification-valid, because native drivers may
tolerate undefined application behavior.

The same missing, tile-shaped terrain and menu-square corruption is documented
in the open [VKD3D-Proton issue #3134](https://github.com/HansKristian-Work/vkd3d-proton/issues/3134)
on an RX 9070 XT with Mesa 26.1.3. Its screenshot and large Proton log were
inspected without adding them to this repository. See
[`docs/external-evidence.md`](docs/external-evidence.md) for checksums, filtered
facts, and the consequences for the controlled experiments.

## Reproducing or resuming

Do not alter the prefix until a backup exists and Steam and the game are fully
stopped.

```bash
./scripts/collect-system-info.sh
./scripts/backup-prefix.sh
./scripts/collect-proton-log.sh prepare E00-baseline-r1 baseline
```

Paste the printed launch option into Steam, perform the fixed reproduction in
[`docs/reproduction.md`](docs/reproduction.md), exit the game, then ingest the
log:

```bash
./scripts/collect-proton-log.sh collect E00-baseline-r1
```

For new work, use the altitude-controlled procedure in
[`docs/reproduction.md`](docs/reproduction.md) and the order recorded in
[`docs/experiment-matrix.md`](docs/experiment-matrix.md).
Compare collected runs with:

```bash
./scripts/compare-logs.py \
  captures/runs/E00-baseline-r1 \
  captures/runs/E00-baseline-r2
```

## Safety and evidence rules

- Full prefix backups default to
  `${XDG_STATE_HOME:-$HOME/.local/state}/il2-korea-proton-investigation/` and
  are not committed here.
- Restore and DLL-install operations require an explicit `--yes`, refuse to
  run while IL-2/Proton is active, and preserve the displaced files.
- Generated content under `captures/` is ignored. Its tracked README inventories
  the local evidence; curate only reviewed, redacted artifacts for handoff.
- Game assets, Steam configuration databases, credentials, unfiltered giant
  traces, and shader caches must not be committed.
- A Proton update changes the baseline. Re-run system collection after every
  Proton, VKD3D-Proton, Mesa, kernel, firmware, or game update.

## Documentation

- [`docs/environment.md`](docs/environment.md): verified local environment
- [`docs/external-evidence.md`](docs/external-evidence.md): issue #3134 artifact
  and log analysis
- [`docs/evidence-e00-baseline.md`](docs/evidence-e00-baseline.md): two-run
  controlled local baseline
- [`docs/evidence-e01-no-upload-hvv.md`](docs/evidence-e01-no-upload-hvv.md):
  altitude-confounded host-visible-upload result
- [`docs/altitude-observation.md`](docs/altitude-observation.md): screenshot
  evidence and revised altitude-controlled protocol
- [`docs/reproduction.md`](docs/reproduction.md): fixed visual reproductions
- [`docs/hypotheses.md`](docs/hypotheses.md): ranked hypotheses and prior art
- [`docs/experiment-matrix.md`](docs/experiment-matrix.md): one-variable test plan
- [`docs/development-build.md`](docs/development-build.md): exact local build and
  source-level investigation status
- [`docs/evidence-d00-local-build.md`](docs/evidence-d00-local-build.md): why the
  attempted local-build parity control is invalid
- [`docs/evidence-d01-invalid-prefix-install.md`](docs/evidence-d01-invalid-prefix-install.md):
  proof that stock Proton overwrote the trace DLLs
- [`docs/evidence-d01b-sparse-trace.md`](docs/evidence-d01b-sparse-trace.md):
  valid trace excluding the D3D12 reserved/tiled-resource path
- [`docs/evidence-d02-preparation.md`](docs/evidence-d02-preparation.md): D02
  build and isolated custom-tool verification
- [`docs/evidence-d02-ordinary-texture-trace.md`](docs/evidence-d02-ordinary-texture-trace.md):
  valid D02 runtime result and next diagnostic discriminator
- [`docs/evidence-d03-preparation.md`](docs/evidence-d03-preparation.md): D03
  build, isolated custom tool, exact launch option, and rollback
- [`docs/game-binary-inspection.md`](docs/game-binary-inspection.md): read-only
  import, symbol, and diagnostic-string evidence from the compiled game files
- [`docs/evidence-u00-game-update.md`](docs/evidence-u00-game-update.md): updated
  game-build baseline result
- [`docs/findings.md`](docs/findings.md): evidence ledger and root-cause status
- [`docs/upstream-drafts.md`](docs/upstream-drafts.md): review-only issue drafts
- [`patches/README.md`](patches/README.md): why no patch is justified

## Rollback

Restore a full backup with:

```bash
./scripts/restore-prefix.sh --archive /absolute/path/to/backup.tar.zst --yes
```

The restore script moves the displaced prefix to a timestamped
`247970.pre-restore-*` directory instead of deleting it. Custom VKD3D DLLs can
also be reverted using the backup directory printed by
`install-vkd3d-build.sh`; see the detailed rollback section in
[`docs/reproduction.md`](docs/reproduction.md).
