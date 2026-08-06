#!/usr/bin/env bash
set -euo pipefail

app_id="247970"
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
project_dir=$(cd -- "$script_dir/.." && pwd -P)
runs_dir="$project_dir/captures/runs"

usage() {
    printf '%s\n' \
        "Usage:" \
        "  $0 list-variants" \
        "  $0 prepare RUN_ID VARIANT" \
        "  $0 collect RUN_ID [--source PATH] [--notes TEXT] [--no-system-info]" \
        "" \
        "Variants:" \
        "  baseline" \
        "  local-vkd3d-baseline" \
        "  resource-trace" \
        "  texture-trace" \
        "  no-upload-hvv" \
        "  single-queue" \
        "  no-descriptor-buffer" \
        "  no-upload-hvv-single-queue"
}

validate_run_id() {
    if [[ ! "$1" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
        printf 'Invalid run ID %q; use letters, digits, dot, underscore, or hyphen.\n' "$1" >&2
        exit 2
    fi
}

variant_environment() {
    case "$1" in
        baseline|local-vkd3d-baseline)
            printf '%s' ''
            ;;
        resource-trace)
            printf '%s' 'VKD3D_IL2_RESOURCE_TRACE=1 '
            ;;
        texture-trace)
            printf '%s' 'VKD3D_IL2_TEXTURE_TRACE=1 '
            ;;
        no-upload-hvv)
            printf '%s' 'VKD3D_CONFIG=no_upload_hvv '
            ;;
        single-queue)
            printf '%s' 'VKD3D_CONFIG=single_queue '
            ;;
        no-descriptor-buffer)
            printf '%s' 'VKD3D_DISABLE_EXTENSIONS=VK_EXT_descriptor_buffer '
            ;;
        no-upload-hvv-single-queue)
            printf '%s' 'VKD3D_CONFIG=no_upload_hvv,single_queue '
            ;;
        *)
            printf 'Unknown variant: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
}

game_processes() {
    pgrep -af 'IL2Series\.exe|IL2Series/Launcher\.exe|SteamLaunch AppId=247970' 2>/dev/null || true
}

prepare_run() {
    local run_id=$1
    local variant=$2
    local run_dir="$runs_dir/$run_id"
    local extra launch_options quoted_dir short_log_dir

    validate_run_id "$run_id"
    extra=$(variant_environment "$variant")
    if [[ -e "$run_dir" ]]; then
        printf 'Run directory already exists; choose a unique ID: %s\n' "$run_dir" >&2
        exit 1
    fi

    short_log_dir="/tmp/il2-$run_id"
    if [[ -e "$short_log_dir" || -L "$short_log_dir" ]]; then
        printf 'Short log path already exists; remove or choose another run ID: %s\n' "$short_log_dir" >&2
        exit 1
    fi

    mkdir -p -- "$run_dir"
    ln -s -- "$run_dir" "$short_log_dir"
    printf -v quoted_dir '%q' "$short_log_dir"
    launch_options="PROTON_LOG=1 PROTON_LOG_DIR=$quoted_dir OMP_NUM_THREADS=16 KMP_AFFINITY=disabled ${extra}%command%"

    printf '%s\n' "$variant" >"$run_dir/variant.txt"
    printf '%s\n' "$launch_options" >"$run_dir/launch-options.txt"
    {
        printf 'run_id=%s\n' "$run_id"
        printf 'variant=%s\n' "$variant"
        printf 'app_id=%s\n' "$app_id"
        printf 'prepared_utc=%s\n' "$(date -u --iso-8601=seconds)"
        printf 'status=prepared\n'
    } >"$run_dir/metadata.txt"
    {
        printf 'Run ID: %s\n' "$run_id"
        printf 'UTC start/end:\n'
        printf 'Game build ID:\n'
        printf 'Proton version:\n'
        printf 'VKD3D-Proton commit:\n'
        printf 'DXVK commit:\n'
        printf 'Mesa/RADV version:\n'
        printf 'Kernel:\n'
        printf 'Launch options: %s\n' "$launch_options"
        printf 'Prefix backup ID:\n'
        printf 'Graphics settings and resolution:\n'
        printf 'Mission/camera protocol:\n'
        printf 'Started (yes/no):\n'
        printf 'Menu aircraft (fixed/improved/unchanged/regressed/inconclusive):\n'
        printf 'Flickering squares (fixed/improved/unchanged/regressed/inconclusive):\n'
        printf 'Terrain loading (fixed/improved/unchanged/regressed/inconclusive):\n'
        printf 'Distant terrain (fixed/improved/unchanged/regressed/inconclusive):\n'
        printf 'Menu/in-mission FPS or frame-time notes:\n'
        printf 'GPU hang/reset (yes/no):\n'
        printf 'New warnings/errors:\n'
        printf 'Screenshot/video filenames and SHA-256:\n'
        printf 'Notes:\n'
    } >"$run_dir/observations.md"

    printf 'Steam Launch Options for %s:\n\n%s\n\n' "$run_id" "$launch_options"
    printf 'After the game exits, run:\n  %q collect %q\n' "$0" "$run_id"
    printf 'Record visual results in:\n  %s\n' "$run_dir/observations.md"
}

count_matches() {
    local pattern=$1
    local file=$2
    grep -Eic -- "$pattern" "$file" 2>/dev/null || true
}

collect_run() {
    local run_id=$1
    shift
    local run_dir="$runs_dir/$run_id"
    local source_log=""
    local notes=""
    local include_system_info=1
    local process_list

    validate_run_id "$run_id"
    if [[ ! -d "$run_dir" || ! -f "$run_dir/variant.txt" ]]; then
        printf 'Prepared run not found: %s\n' "$run_dir" >&2
        exit 1
    fi

    while (($#)); do
        case "$1" in
            --source)
                source_log="${2:?--source requires a path}"
                shift 2
                ;;
            --notes)
                notes="${2:?--notes requires text}"
                shift 2
                ;;
            --no-system-info)
                include_system_info=0
                shift
                ;;
            *)
                printf 'Unknown collect argument: %s\n' "$1" >&2
                exit 2
                ;;
        esac
    done

    process_list=$(game_processes)
    if [[ -n "$process_list" ]]; then
        printf 'Refusing to collect while an IL-2/Proton process is active:\n%s\n' "$process_list" >&2
        exit 1
    fi

    if [[ -z "$source_log" ]]; then
        source_log="$run_dir/steam-$app_id.log"
        if [[ ! -f "$source_log" && -f "${HOME:-}/steam-$app_id.log" ]]; then
            source_log="${HOME}/steam-$app_id.log"
        fi
    fi
    if [[ ! -f "$source_log" ]]; then
        printf 'Proton log not found: %s\n' "$source_log" >&2
        printf 'Confirm PROTON_LOG_DIR was accepted or pass --source explicitly.\n' >&2
        exit 1
    fi

    if [[ -e "$run_dir/proton.log.gz" ]]; then
        printf 'Collected log already exists for %s; refusing to overwrite it.\n' "$run_id" >&2
        exit 1
    fi

    gzip -n -9 -c -- "$source_log" >"$run_dir/proton.log.gz"
    sha256sum -- "$source_log" >"$run_dir/source-log.sha256"

    awk '
        BEGIN { IGNORECASE = 1 }
        /IL2TRACE|IL2TEX|vkd3d|d3d12|dxgi|d3d11|vulkan|radv|amdgpu|queue|descriptor|sparse|residen|barrier|image layout|upload|host.visible|memory (heap|type|budget)|GetNuma|NUMA|OpenMP|KMP_|err:|warn:/ { print }
    ' "$source_log" >"$run_dir/filtered.log"

    awk '
        BEGIN { IGNORECASE = 1 }
        /loaddll|Loaded L|Loaded .*\.(dll|exe)|d3d12(core)?\.dll|dxgi\.dll|d3d11\.dll|dxBackend12\.dll|IL2Series\.exe/ { print }
    ' "$source_log" >"$run_dir/modules.log"

    {
        printf 'run_id=%s\n' "$run_id"
        printf 'source=%s\n' "$source_log"
        printf 'source_bytes=%s\n' "$(stat -c %s -- "$source_log")"
        printf 'split_end_only_count=%s\n' "$(count_matches 'split barrier.*END_ONLY|END_ONLY.*split barrier' "$source_log")"
        printf 'vkd3d_warning_count=%s\n' "$(count_matches 'warn:.*vkd3d|vkd3d.*warn' "$source_log")"
        printf 'wine_error_count=%s\n' "$(count_matches '(^|:|[[:space:]])err:' "$source_log")"
        printf 'd3d12_module_lines=%s\n' "$(count_matches 'd3d12(core)?\.dll' "$run_dir/modules.log")"
        printf 'dxgi_module_lines=%s\n' "$(count_matches 'dxgi\.dll' "$run_dir/modules.log")"
        printf 'd3d11_module_lines=%s\n' "$(count_matches 'd3d11\.dll' "$run_dir/modules.log")"
        printf 'descriptor_buffer_lines=%s\n' "$(count_matches 'descriptor.buffer|VK_EXT_descriptor_buffer' "$source_log")"
        printf 'queue_lines=%s\n' "$(count_matches 'queue family|queue.*(compute|transfer|graphics)|single.queue' "$source_log")"
        printf 'il2trace_enabled_count=%s\n' "$(count_matches 'IL2TRACE enabled:' "$source_log")"
        printf 'reserved_create_count=%s\n' "$(count_matches 'IL2TRACE reserved_create' "$source_log")"
        printf 'get_tiling_count=%s\n' "$(count_matches 'IL2TRACE get_tiling' "$source_log")"
        printf 'tile_update_count=%s\n' "$(count_matches 'IL2TRACE tile_update cookie=' "$source_log")"
        printf 'tile_update_submit_count=%s\n' "$(count_matches 'IL2TRACE tile_update_submit' "$source_log")"
        printf 'tile_copy_count=%s\n' "$(count_matches 'IL2TRACE tile_copy' "$source_log")"
        printf 'il2tex_enabled_count=%s\n' "$(count_matches 'IL2TEX enabled:' "$source_log")"
        printf 'texture_create_count=%s\n' "$(count_matches 'IL2TEX create ' "$source_log")"
        printf 'texture_srv_count=%s\n' "$(count_matches 'IL2TEX srv ' "$source_log")"
        printf 'texture_copy_region_count=%s\n' "$(count_matches 'IL2TEX copy ' "$source_log")"
        printf 'texture_copy_resource_count=%s\n' "$(count_matches 'IL2TEX copy_resource ' "$source_log")"
        printf 'texture_destroy_count=%s\n' "$(count_matches 'IL2TEX destroy ' "$source_log")"
        printf 'texture_trace_suppressed_count=%s\n' "$(count_matches 'IL2TEX suppressed ' "$source_log")"
    } >"$run_dir/summary.txt"

    if grep -q -- 'IL2TEX enabled:' "$source_log"; then
        "$script_dir/analyze-texture-trace.py" "$source_log" \
            --output "$run_dir/texture-trace-analysis.md"
    fi

    if ((include_system_info)) && [[ ! -f "$run_dir/system-info.txt" ]]; then
        "$script_dir/collect-system-info.sh" --output "$run_dir/system-info.txt"
    fi

    {
        printf 'collected_utc=%s\n' "$(date -u --iso-8601=seconds)"
        printf 'source_log=%s\n' "$source_log"
        printf 'compressed_sha256=%s\n' "$(sha256sum "$run_dir/proton.log.gz" | awk '{ print $1 }')"
        if [[ -n "$notes" ]]; then
            printf 'notes=%s\n' "$notes"
        fi
        printf 'status=collected-awaiting-visual-classification\n'
    } >>"$run_dir/metadata.txt"

    printf 'Collected %s\n' "$run_id"
    printf '  full log: %s\n' "$run_dir/proton.log.gz"
    printf '  filtered: %s\n' "$run_dir/filtered.log"
    printf '  modules:  %s\n' "$run_dir/modules.log"
    printf '  summary:  %s\n' "$run_dir/summary.txt"
    if [[ -f "$run_dir/texture-trace-analysis.md" ]]; then
        printf '  texture analysis: %s\n' "$run_dir/texture-trace-analysis.md"
    fi
    printf '\nKey counts:\n'
    sed -n '1,120p' "$run_dir/summary.txt"
}

command_name=${1:-}
case "$command_name" in
    list-variants)
        usage
        ;;
    prepare)
        if (($# != 3)); then
            usage >&2
            exit 2
        fi
        prepare_run "$2" "$3"
        ;;
    collect)
        if (($# < 2)); then
            usage >&2
            exit 2
        fi
        run_id=$2
        shift 2
        collect_run "$run_id" "$@"
        ;;
    -h|--help|help|'')
        usage
        ;;
    *)
        printf 'Unknown command: %s\n' "$command_name" >&2
        usage >&2
        exit 2
        ;;
esac
