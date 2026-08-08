#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_root=$(cd -- "$script_dir/.." && pwd)
renderdoc_root="$project_root/build/renderdoc-1.45-local"
renderdoc_library="$renderdoc_root/package/usr/lib/librenderdoc.so"
renderdoc_layer_dir="$renderdoc_root/xdg/vulkan/implicit_layer.d"
steam_cmd="${XDG_DATA_HOME:-${HOME:?HOME is required}/.local/share}/Steam/steam.sh"
run_id="${1:-D20-renderdoc-light-values-r1}"

if [[ ! "$run_id" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
    printf 'Invalid run ID: %s\n' "$run_id" >&2
    exit 2
fi
if [[ ! -f "$renderdoc_library" ]]; then
    printf 'Local RenderDoc library is missing: %s\n' "$renderdoc_library" >&2
    exit 1
fi
if [[ ! -f "$renderdoc_layer_dir/renderdoc_capture.json" ]]; then
    printf 'Local RenderDoc layer manifest is missing: %s\n' "$renderdoc_layer_dir" >&2
    exit 1
fi
if [[ ! -x "$steam_cmd" ]]; then
    printf 'Steam launcher is missing: %s\n' "$steam_cmd" >&2
    exit 1
fi
if pgrep -ax steam >/dev/null 2>&1; then
    printf 'Exit Steam completely before starting the D20 capture session.\n' >&2
    exit 1
fi

capture_dir="$project_root/captures/runs/$run_id/renderdoc"
mkdir -p -- "$capture_dir"

export ENABLE_VULKAN_RENDERDOC_CAPTURE=1
export VKD3D_AUTO_CAPTURE_SHADER=df0bd777fd1bb89d
export VKD3D_AUTO_CAPTURE_COUNTS=0
export VK_ADD_IMPLICIT_LAYER_PATH="$renderdoc_layer_dir${VK_ADD_IMPLICIT_LAYER_PATH:+:$VK_ADD_IMPLICIT_LAYER_PATH}"
export RENDERDOC_CAPFILE="$capture_dir/il2-d20"

printf '%s\n' \
    "Starting Steam with the local RenderDoc Vulkan layer available to its children." \
    "Target shader: df0bd777fd1bb89d; matching submission: 0." \
    "Capture directory: $capture_dir" \
    "Select IL2-Korea-D20-RenderDoc-5735f64f and start AppID 247970."

exec "$steam_cmd"
