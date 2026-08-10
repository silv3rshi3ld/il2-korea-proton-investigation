#!/usr/bin/env bash
set -euo pipefail

tool_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
project_root="${IL2_INVESTIGATION_ROOT:?IL2_INVESTIGATION_ROOT is required}"
renderdoc_root="$project_root/build/renderdoc-1.45-local"
renderdoc_layer_dir="$renderdoc_root/xdg/vulkan/implicit_layer.d"
trigger_library="$renderdoc_root/libd44_trigger_capture.so"
capture_dir="$project_root/captures/runs/D44-consecutive-light-values-r1/renderdoc"
trigger_file="/tmp/il2-d44-trigger-${USER:?USER is required}"

if [[ ! -x "$tool_dir/proton.d44-base" ]]; then
    printf 'D44 base Proton launcher is missing: %s\n' "$tool_dir/proton.d44-base" >&2
    exit 1
fi
if [[ ! -f "$trigger_library" || ! -f "$renderdoc_layer_dir/renderdoc_capture.json" ]]; then
    printf 'D44 local RenderDoc support is incomplete under %s\n' "$renderdoc_root" >&2
    exit 1
fi

mkdir -p -- "$capture_dir"

export ENABLE_VULKAN_RENDERDOC_CAPTURE=1
export VK_ADD_IMPLICIT_LAYER_PATH="$renderdoc_layer_dir${VK_ADD_IMPLICIT_LAYER_PATH:+:$VK_ADD_IMPLICIT_LAYER_PATH}"
export VK_IMPLICIT_LAYER_PATH="$renderdoc_layer_dir${VK_IMPLICIT_LAYER_PATH:+:$VK_IMPLICIT_LAYER_PATH}"
export VK_LAYER_PATH="$renderdoc_layer_dir${VK_LAYER_PATH:+:$VK_LAYER_PATH}"
export VK_INSTANCE_LAYERS="VK_LAYER_RENDERDOC_Capture${VK_INSTANCE_LAYERS:+:$VK_INSTANCE_LAYERS}"
export RENDERDOC_CAPFILE="$capture_dir/il2-d44"
export IL2_RENDERDOC_CAPTURE_PATH="$capture_dir/il2-d44"
export IL2_RENDERDOC_TRIGGER_FILE="$trigger_file"
export IL2_RENDERDOC_FRAME_COUNT=3
export IL2_RENDERDOC_TARGET_COMM=IL2Series.exe
export LD_PRELOAD="$trigger_library${LD_PRELOAD:+:$LD_PRELOAD}"

exec "$tool_dir/proton.d44-base" "$@"
