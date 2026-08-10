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
        "  $0 prepare RUN_ID VARIANT [--no-openmp-override]" \
        "  $0 collect RUN_ID [--source PATH] [--notes TEXT] [--no-system-info]" \
        "" \
        "Variants:" \
        "  baseline" \
        "  local-vkd3d-baseline" \
        "  resource-trace" \
        "  texture-trace" \
        "  alias-trace" \
        "  bc3-border-normalization" \
        "  bc3-page-normalization" \
        "  baked-cache-trace" \
        "  menu-pass-trace" \
        "  light-trace" \
        "  descriptor-trace" \
        "  radv-fullsync-descriptor-trace" \
        "  radv-nodcc-descriptor-trace" \
        "  aco-force-waitcnt-descriptor-trace" \
        "  renderdoc-light-values" \
        "  shader-dump" \
        "  no-upload-hvv" \
        "  single-queue" \
        "  no-descriptor-buffer" \
        "  no-fragment-shading-rate" \
        "  no-upload-hvv-single-queue"
}

validate_run_id() {
    if [[ ! "$1" =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]; then
        printf 'Invalid run ID %q; use letters, digits, dot, underscore, or hyphen.\n' "$1" >&2
        exit 2
    fi
}

variant_environment() {
    local variant=$1
    local run_id=${2:-}

    case "$variant" in
        baseline|local-vkd3d-baseline)
            printf '%s' ''
            ;;
        resource-trace)
            printf '%s' 'VKD3D_IL2_RESOURCE_TRACE=1 '
            ;;
        texture-trace)
            printf '%s' 'VKD3D_IL2_TEXTURE_TRACE=1 '
            ;;
        alias-trace)
            printf '%s' 'VKD3D_IL2_TEXTURE_TRACE=1 VKD3D_IL2_ALIAS_TRACE=1 '
            ;;
        bc3-border-normalization)
            printf '%s' 'VKD3D_IL2_BC3_BORDER_COPY=1 '
            ;;
        bc3-page-normalization)
            printf '%s' 'VKD3D_IL2_BC3_PAGE_COPY=1 '
            ;;
        baked-cache-trace)
            printf '%s' 'VKD3D_IL2_BAKED_CACHE_TRACE=1 '
            ;;
        menu-pass-trace)
            printf '%s' 'VKD3D_IL2_MENU_TRACE=1 '
            ;;
        light-trace)
            printf '%s' 'VKD3D_IL2_LIGHT_TRACE=1 '
            ;;
        descriptor-trace)
            printf '%s' 'VKD3D_IL2_DESCRIPTOR_TRACE=1 '
            ;;
        radv-fullsync-descriptor-trace)
            printf '%s' 'RADV_DEBUG=startup,fullsync VKD3D_IL2_DESCRIPTOR_TRACE=1 '
            ;;
        radv-nodcc-descriptor-trace)
            printf '%s' 'RADV_DEBUG=startup,nodcc VKD3D_IL2_DESCRIPTOR_TRACE=1 '
            ;;
        aco-force-waitcnt-descriptor-trace)
            printf '%s' 'RADV_DEBUG=startup ACO_DEBUG=force-waitcnt VKD3D_IL2_DESCRIPTOR_TRACE=1 '
            ;;
        renderdoc-light-values)
            # The RenderDoc wrapper owns capture variables so that they are
            # inherited by the complete Steam process tree.
            printf '%s' ''
            ;;
        shader-dump)
            if [[ -z "$run_id" ]]; then
                printf 'shader-dump requires a run ID\n' >&2
                exit 2
            fi
            printf 'VKD3D_SHADER_DUMP_PATH=/tmp/il2-%s/shaders ' "$run_id"
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
        no-fragment-shading-rate)
            printf '%s' 'VKD3D_DISABLE_EXTENSIONS=VK_KHR_fragment_shading_rate '
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
    shift 2
    local run_dir="$runs_dir/$run_id"
    local extra launch_options quoted_dir short_log_dir
    local include_openmp_override=1

    while (($#)); do
        case "$1" in
            --no-openmp-override)
                include_openmp_override=0
                shift
                ;;
            *)
                printf 'Unknown prepare argument: %s\n' "$1" >&2
                exit 2
                ;;
        esac
    done

    validate_run_id "$run_id"
    extra=$(variant_environment "$variant" "$run_id")
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
    if [[ "$variant" == shader-dump ]]; then
        mkdir -p -- "$run_dir/shaders"
    fi
    printf -v quoted_dir '%q' "$short_log_dir"
    if ((include_openmp_override)); then
        launch_options="PROTON_LOG=1 PROTON_LOG_DIR=$quoted_dir OMP_NUM_THREADS=16 KMP_AFFINITY=disabled ${extra}%command%"
    else
        launch_options="PROTON_LOG=1 PROTON_LOG_DIR=$quoted_dir ${extra}%command%"
    fi

    printf '%s\n' "$variant" >"$run_dir/variant.txt"
    printf '%s\n' "$launch_options" >"$run_dir/launch-options.txt"
    {
        printf 'run_id=%s\n' "$run_id"
        printf 'variant=%s\n' "$variant"
        printf 'app_id=%s\n' "$app_id"
        printf 'openmp_override=%s\n' "$([[ $include_openmp_override -eq 1 ]] && printf enabled || printf disabled)"
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
        /IL2TRACE|IL2TEX|IL2ALIAS|IL2BCCOPY|IL2CACHE|IL2MENU|IL2LIGHT|vkd3d|d3d12|dxgi|d3d11|vulkan|radv|amdgpu|queue|descriptor|fragment.shading|shading.rate|VRS|sparse|residen|barrier|image layout|upload|host.visible|memory (heap|type|budget)|GetNuma|NUMA|OpenMP|KMP_|err:|warn:/ { print }
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
        printf 'fragment_shading_rate_lines=%s\n' "$(count_matches 'fragment.shading|shading.rate|VK_KHR_fragment_shading_rate' "$source_log")"
        printf 'fragment_shading_rate_disabled_count=%s\n' "$(count_matches 'Extension .*VK_KHR_fragment_shading_rate.* is disabled' "$source_log")"
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
        printf 'il2alias_enabled_count=%s\n' "$(count_matches 'IL2ALIAS enabled:' "$source_log")"
        printf 'alias_resource_create_count=%s\n' "$(count_matches 'IL2ALIAS create ' "$source_log")"
        printf 'alias_resource_destroy_count=%s\n' "$(count_matches 'IL2ALIAS destroy ' "$source_log")"
        printf 'alias_barrier_count=%s\n' "$(count_matches 'IL2ALIAS barrier ' "$source_log")"
        printf 'alias_trace_suppressed_count=%s\n' "$(count_matches 'IL2ALIAS suppressed ' "$source_log")"
        printf 'bc3_border_enabled_count=%s\n' "$(count_matches 'IL2BCCOPY enabled ' "$source_log")"
        printf 'bc3_border_adjustment_count=%s\n' "$(count_matches 'IL2BCCOPY adjust ' "$source_log")"
        printf 'bc3_border_log_limit_count=%s\n' "$(count_matches 'IL2BCCOPY adjustment log limit' "$source_log")"
        printf 'baked_cache_enabled_count=%s\n' "$(count_matches 'IL2CACHE enabled:' "$source_log")"
        printf 'baked_cache_create_count=%s\n' "$(count_matches 'IL2CACHE create ' "$source_log")"
        printf 'baked_cache_srv_count=%s\n' "$(count_matches 'IL2CACHE srv ' "$source_log")"
        printf 'baked_cache_copy_count=%s\n' "$(count_matches 'IL2CACHE copy_(image|buffer_image|resource)' "$source_log")"
        printf 'baked_cache_barrier_count=%s\n' "$(count_matches 'IL2CACHE (enhanced_)?barrier ' "$source_log")"
        printf 'baked_cache_destroy_count=%s\n' "$(count_matches 'IL2CACHE destroy ' "$source_log")"
        printf 'baked_cache_suppressed_count=%s\n' "$(count_matches 'IL2CACHE suppressed ' "$source_log")"
        printf 'il2menu_resource_trace_enabled_count=%s\n' "$(count_matches 'IL2MENU resource-name trace enabled' "$source_log")"
        printf 'il2menu_pix_trace_enabled_count=%s\n' "$(count_matches 'IL2MENU PIX-event trace enabled' "$source_log")"
        printf 'il2menu_resource_name_count=%s\n' "$(count_matches 'IL2MENU resource_name ' "$source_log")"
        printf 'il2menu_pix_event_count=%s\n' "$(count_matches 'IL2MENU pix_event ' "$source_log")"
        printf 'il2menu_reflection_name_count=%s\n' "$(count_matches 'IL2MENU.*(SSR|reflection|reflect)' "$source_log")"
        printf 'il2menu_usage_trace_enabled_count=%s\n' "$(count_matches 'IL2MENU usage trace enabled' "$source_log")"
        printf 'il2menu_usage_event_count=%s\n' "$(count_matches 'IL2MENU usage sequence=' "$source_log")"
        printf 'il2menu_barrier_count=%s\n' "$(count_matches 'IL2MENU usage .*op=(barrier|uav_barrier|alias_barrier)' "$source_log")"
        printf 'il2menu_rtv_bind_count=%s\n' "$(count_matches 'IL2MENU usage .*op=rtv_bind' "$source_log")"
        printf 'il2menu_rtv_clear_count=%s\n' "$(count_matches 'IL2MENU usage .*op=rtv_clear' "$source_log")"
        printf 'il2menu_draw_count=%s\n' "$(count_matches 'IL2MENU usage .*op=draw(_indexed)? ' "$source_log")"
        printf 'il2menu_copy_count=%s\n' "$(count_matches 'IL2MENU usage .*op=copy_(texture|resource)' "$source_log")"
        printf 'il2menu_execute_count=%s\n' "$(count_matches 'IL2MENU usage .*op=execute' "$source_log")"
        printf 'il2menu_trace_suppressed_count=%s\n' "$(count_matches 'IL2MENU .*suppressed after limit' "$source_log")"
        printf 'il2light_resource_trace_enabled_count=%s\n' "$(count_matches 'IL2LIGHT resource-name trace enabled' "$source_log")"
        printf 'il2light_usage_trace_enabled_count=%s\n' "$(count_matches 'IL2LIGHT usage trace enabled' "$source_log")"
        printf 'il2light_resource_name_count=%s\n' "$(count_matches 'IL2LIGHT resource_name ' "$source_log")"
        printf 'il2light_usage_event_count=%s\n' "$(count_matches 'IL2LIGHT usage sequence=' "$source_log")"
        printf 'il2light_barrier_count=%s\n' "$(count_matches 'IL2LIGHT usage .*op=(barrier|enhanced_barrier|uav_barrier|alias_barrier)' "$source_log")"
        printf 'il2light_rtv_bind_count=%s\n' "$(count_matches 'IL2LIGHT usage .*op=rtv_bind' "$source_log")"
        printf 'il2light_clear_count=%s\n' "$(count_matches 'IL2LIGHT usage .*op=(rtv_clear|uav_clear_uint|uav_clear_float)' "$source_log")"
        printf 'il2light_draw_count=%s\n' "$(count_matches 'IL2LIGHT usage .*op=draw(_indexed)? ' "$source_log")"
        printf 'il2light_dispatch_count=%s\n' "$(count_matches 'IL2LIGHT usage .*op=dispatch ' "$source_log")"
        printf 'il2light_copy_count=%s\n' "$(count_matches 'IL2LIGHT usage .*op=copy_(texture|resource)' "$source_log")"
        printf 'il2light_execute_count=%s\n' "$(count_matches 'IL2LIGHT usage .*op=execute' "$source_log")"
        printf 'il2light_trace_suppressed_count=%s\n' "$(count_matches 'IL2LIGHT .*suppressed after limit' "$source_log")"
        printf 'il2descriptor_enabled_count=%s\n' "$(count_matches 'IL2DESC descriptor sidecar trace enabled' "$source_log")"
        printf 'il2descriptor_heap_count=%s\n' "$(count_matches 'IL2DESC heap=' "$source_log")"
        printf 'il2descriptor_resource_name_count=%s\n' "$(count_matches 'IL2DESC resource_name ' "$source_log")"
        printf 'il2descriptor_draw_event_count=%s\n' "$(count_matches 'IL2DESC sequence=' "$source_log")"
        printf 'il2descriptor_resolved_count=%s\n' "$(count_matches 'IL2DESC sequence=.*status=resolved' "$source_log")"
        printf 'il2descriptor_t9_resolved_count=%s\n' "$(count_matches 'IL2DESC sequence=.*register=t9 status=resolved' "$source_log")"
        printf 'il2descriptor_t10_resolved_count=%s\n' "$(count_matches 'IL2DESC sequence=.*register=t10 status=resolved' "$source_log")"
        printf 'il2descriptor_failure_count=%s\n' "$(count_matches 'IL2DESC sequence=.*status=(no_|heap_oob)' "$source_log")"
        printf 'il2descriptor_trace_suppressed_count=%s\n' "$(count_matches 'IL2DESC draw events suppressed after limit' "$source_log")"
    } >"$run_dir/summary.txt"

    if grep -q -- 'IL2TEX enabled:' "$source_log"; then
        "$script_dir/analyze-texture-trace.py" "$source_log" \
            --output "$run_dir/texture-trace-analysis.md"
    fi

    if grep -q -- 'IL2ALIAS enabled:' "$source_log"; then
        "$script_dir/analyze-alias-trace.py" "$source_log" \
            --output "$run_dir/alias-trace-analysis.md"
    fi

    if grep -q -- 'IL2BCCOPY enabled ' "$source_log"; then
        if ! "$script_dir/analyze-bc3-border-copy.py" "$source_log" \
                --output "$run_dir/bc3-border-copy-analysis.md"; then
            printf 'BC3 diagnostic validation failed; preserving the run as invalid evidence.\n' >&2
        fi
    fi

    if grep -q -- 'IL2CACHE enabled:' "$source_log"; then
        "$script_dir/analyze-baked-cache-trace.py" "$source_log" \
            --output "$run_dir/baked-cache-analysis.md"
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
    if [[ -f "$run_dir/alias-trace-analysis.md" ]]; then
        printf '  alias analysis:   %s\n' "$run_dir/alias-trace-analysis.md"
    fi
    if [[ -f "$run_dir/bc3-border-copy-analysis.md" ]]; then
        printf '  BC3 analysis:     %s\n' "$run_dir/bc3-border-copy-analysis.md"
    fi
    if [[ -f "$run_dir/baked-cache-analysis.md" ]]; then
        printf '  baked-cache analysis: %s\n' "$run_dir/baked-cache-analysis.md"
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
        if (($# < 3 || $# > 4)); then
            usage >&2
            exit 2
        fi
        run_id=$2
        variant=$3
        shift 3
        prepare_run "$run_id" "$variant" "$@"
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
