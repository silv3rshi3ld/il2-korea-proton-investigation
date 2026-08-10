#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_root=$(cd -- "$script_dir/.." && pwd)
renderdoc_root="$project_root/build/renderdoc-1.45-local"
steam_cmd="${XDG_DATA_HOME:-${HOME:?HOME is required}/.local/share}/Steam/steam.sh"
run_id="${1:-D44-consecutive-light-values-r1}"
trigger_file="/tmp/il2-d44-trigger-${USER:?USER is required}"
tool_dir="${HOME:?HOME is required}/.local/share/Steam/compatibilitytools.d/IL2-Korea-D44-ConsecutiveCapture"

if [[ ! "$run_id" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
    printf 'Invalid run ID: %s\n' "$run_id" >&2
    exit 2
fi
if [[ ! -x "$tool_dir/proton.d44-base" ]] || ! rg -q 'IL2_RENDERDOC_FRAME_COUNT=3' "$tool_dir/proton"; then
    printf 'The game-only D44 Proton capture wrapper is not installed in %s\n' "$tool_dir" >&2
    exit 1
fi
if [[ ! -x "$steam_cmd" ]]; then
    printf 'Steam launcher is missing: %s\n' "$steam_cmd" >&2
    exit 1
fi
if pgrep -ax steam >/dev/null 2>&1; then
    printf 'Exit Steam completely before starting the D44 capture session.\n' >&2
    exit 1
fi

capture_dir="$project_root/captures/runs/$run_id/renderdoc"
mkdir -p -- "$capture_dir"
rm -f -- "$trigger_file"
export IL2_INVESTIGATION_ROOT="$project_root"

printf '%s\n' \
    "Starting Steam normally; the D44 capture environment is scoped to its custom Proton launcher." \
    "Capture directory: $capture_dir" \
    "Select IL2-Korea-D44-ConsecutiveCapture and start AppID 247970." \
    "When the affected menu is visible, create the trigger with:" \
    "  touch $trigger_file"

exec "$steam_cmd"
