#!/usr/bin/env bash
set -euo pipefail

app_id="247970"
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
project_dir=$(cd -- "$script_dir/.." && pwd -P)
runs_dir="$project_dir/captures/runs"
run_id=${1:-}
game_data_override=${2:-}

usage() {
    printf '%s\n' \
        "Usage: $0 RUN_ID [GAME_DATA_DIRECTORY]" \
        "" \
        "Copies bounded, text-only IL-2 diagnostic logs after the game exits." \
        "Existing evidence is never overwritten."
}

if [[ -z "$run_id" || "$run_id" == "-h" || "$run_id" == "--help" ]]; then
    usage
    [[ -n "$run_id" ]] && exit 0
    exit 2
fi
if [[ ! "$run_id" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
    printf 'Invalid run ID %q.\n' "$run_id" >&2
    exit 2
fi

active=$(pgrep -af 'IL2Series\.exe|IL2Series/Launcher\.exe|SteamLaunch AppId=247970' 2>/dev/null || true)
if [[ -n "$active" ]]; then
    printf 'Refusing to collect while IL-2/Proton is active:\n%s\n' "$active" >&2
    exit 1
fi

run_dir="$runs_dir/$run_id"
if [[ ! -d "$run_dir" ]]; then
    printf 'Prepared run directory not found: %s\n' "$run_dir" >&2
    exit 1
fi

if [[ -n "$game_data_override" ]]; then
    game_data=$(readlink -f -- "$game_data_override")
else
    game_data="${HOME:?HOME is required when GAME_DATA_DIRECTORY is omitted}/.local/share/Steam/steamapps/common/IL2Series/data"
fi
if [[ ! -d "$game_data" ]]; then
    printf 'Game data directory not found: %s\n' "$game_data" >&2
    exit 1
fi

destination="$run_dir/game-logs"
if [[ -e "$destination" ]]; then
    printf 'Game-log evidence already exists; refusing to overwrite: %s\n' "$destination" >&2
    exit 1
fi
mkdir -p -- "$destination"

logs=(tex.log packman.log _gui.log fxerr.log exp.log ActionMapper_error.log)
copied=0
for name in "${logs[@]}"; do
    source="$game_data/$name"
    if [[ ! -f "$source" ]]; then
        continue
    fi
    size=$(stat -c %s -- "$source")
    if ((size > 8388608)); then
        printf 'Skipping %s because it exceeds the 8 MiB evidence limit.\n' "$source" >&2
        continue
    fi
    if [[ "$name" != "fxerr.log" && "$name" != "exp.log" && "$name" != "ActionMapper_error.log" ]] && \
            ! file -b --mime-type "$source" | grep -Eq '^text/|^inode/x-empty$'; then
        printf 'Skipping non-text diagnostic: %s\n' "$source" >&2
        continue
    fi
    cp -a -- "$source" "$destination/$name"
    copied=$((copied + 1))
done

if ((copied == 0)); then
    rmdir -- "$destination"
    printf 'No bounded game diagnostic logs were found under %s.\n' "$game_data" >&2
    exit 1
fi

(
    cd -- "$destination"
    sha256sum -- * >SHA256SUMS
    stat -c '%y %s %n' -- * >FILE-METADATA.txt
)

printf 'Copied %d game diagnostic logs to %s\n' "$copied" "$destination"
printf 'These are copies; the game installation was not modified.\n'
