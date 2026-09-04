#!/usr/bin/env bash
# benchmark.sh - Compare D3D12 vs Vulkan performance
#
# Usage: ./tools/benchmark.sh [-d Duration] [-s SkipWarmup] [-b BuildDir]
#
# Runs the game with both backends and compares performance metrics.
# Equivalent to benchmark.ps1 for cross-platform use.

set -euo pipefail

# Defaults (matching PowerShell defaults)
Duration=60
SkipWarmup=10
BuildDir=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--duration) Duration="$2"; shift 2 ;;
        -s|--skip-warmup) SkipWarmup="$2"; shift 2 ;;
        -b|--build-dir) BuildDir="$2"; shift 2 ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

# Project root is one level up from tools/
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Default build dir if not specified
if [[ -z "$BuildDir" ]]; then
    BuildDir="$PROJECT_ROOT/out/build/linux-amd64-relwithdebinfo"
fi

# Detect executable (Linux/macOS: .bin, Windows: .exe)
if [[ -f "$BuildDir/perfectdarkremasterrecomp.bin" ]]; then
    ExePath="$BuildDir/perfectdarkremasterrecomp.bin"
elif [[ -f "$BuildDir/perfectdarkremasterrecomp.exe" ]]; then
    ExePath="$BuildDir/perfectdarkremasterrecomp.exe"
else
    ExePath="$BuildDir/perfectdarkremasterrecomp"
fi

echo "=== ReXGlue Performance Benchmark ==="
echo "Duration: $Duration seconds, Warmup skip: $SkipWarmup frames"
echo ""

# Check if build dir and executable exist
if [[ ! -d "$BuildDir" ]]; then
    echo "Build dir not found: $BuildDir" >&2
    exit 1
fi
if [[ ! -f "$ExePath" ]]; then
    echo "Executable not found: $ExePath" >&2
    exit 1
fi

# Run D3D12 benchmark
echo "Running D3D12 benchmark ($Duration seconds)..."
d3d12_log="$SCRIPT_DIR/benchmark_d3d12_${USER:-unknown}.csv"

"$ExePath" --graphics_backend=d3d12 --perf_log_csv="$d3d12_log" &
process_pid=$!
sleep "$Duration"
if kill -0 "$process_pid" 2>/dev/null; then
    echo "Stopping D3D12 benchmark..."
    kill -9 "$process_pid" 2>/dev/null || true
    sleep 2
fi
echo "D3D12 benchmark complete"

# Run Vulkan benchmark
echo "Running Vulkan benchmark ($Duration seconds)..."
vulkan_log="$SCRIPT_DIR/benchmark_vulkan_${USER:-unknown}.csv"

"$ExePath" --graphics_backend=vulkan --perf_log_csv="$vulkan_log" &
process_pid=$!
sleep "$Duration"
if kill -0 "$process_pid" 2>/dev/null; then
    echo "Stopping Vulkan benchmark..."
    kill -9 "$process_pid" 2>/dev/null || true
    sleep 2
fi
echo "Vulkan benchmark complete"

# Analyze results using awk for accurate CSV parsing
# Equivalent to Analyze-Log in benchmark.ps1
analyze_log() {
    local path="$1" skip="$2"

    if [[ ! -f "$path" ]]; then
        return 1
    fi

    awk -F',' -v skip="$skip" '
        NR == 1 {
            for (i = 1; i <= NF; i++) {
                cols[i] = $i
                if ($i == "fps") fps_idx = i
                else if ($i == "frame_time_us") ft_idx = i
                else if ($i == "draw_calls") dc_idx = i
                else if ($i == "vertices_processed") v_idx = i
            }
            next
        }
        NR > skip + 1 {
            fps = $(fps_idx)
            ft = $(ft_idx)
            dc = $(dc_idx)
            v = $(v_idx)
            if (fps != 0) { fps_sum += fps; fps_count++ }
            ft_sum += ft; ft_count++
            dc_sum += dc; dc_count++
            v_sum += v; v_count++
        }
        END {
            avg_fps = (fps_count > 0) ? fps_sum / fps_count : 0
            avg_ft = (ft_count > 0) ? ft_sum / ft_count : 0
            avg_dc = (dc_count > 0) ? dc_sum / dc_count : 0
            avg_v = (v_count > 0) ? v_sum / v_count : 0
            printf "%.2f,%.2f,%.2f,%.2f\n", avg_fps, avg_ft, avg_dc, avg_v
        }
    ' "$path"
}

d3d12_avg="$(analyze_log "$d3d12_log" "$SkipWarmup")" || { echo "D3D12 log missing" >&2; exit 1; }
vulkan_avg="$(analyze_log "$vulkan_log" "$SkipWarmup")" || { echo "Vulkan log missing" >&2; exit 1; }

IFS=',' read -ra d3d12_parts <<< "$d3d12_avg"
IFS=',' read -ra vulkan_parts <<< "$vulkan_avg"

d3d12_fps="${d3d12_parts[0]:-0}"
d3d12_ft="${d3d12_parts[1]:-0}"
d3d12_dc="${d3d12_parts[2]:-0}"
d3d12_v="${d3d12_parts[3]:-0}"

vulkan_fps="${vulkan_parts[0]:-0}"
vulkan_ft="${vulkan_parts[1]:-0}"
vulkan_dc="${vulkan_parts[2]:-0}"
vulkan_v="${vulkan_parts[3]:-0}"

# Display comparison
echo ""
echo "=== Performance Comparison ==="
echo ""
printf "%-20s %10s %10s %10s\n" "Metric" "D3D12" "Vulkan" "Diff"
printf "%-20s %10s %10s %10s\n" "--------" "-----" "------" "----"

# Calculate diff and display (matching PowerShell format)
calc_diff() {
    local d3d="$1" vul="$2"
    if [[ "$d3d" != "0" ]]; then
        awk "BEGIN { printf \"%.1f\", ($vul - $d3d) / $d3d * 100 }"
    else
        echo "N/A"
    fi
}

printf "%-20s %10s %10s %10s%%\n" "Avg FPS" "$d3d12_fps" "$vulkan_fps" "$(calc_diff "$d3d12_fps" "$vulkan_fps")"
printf "%-20s %10s %10s %10s%%\n" "Avg Frame Time (us)" "$d3d12_ft" "$vulkan_ft" "$(calc_diff "$d3d12_ft" "$vulkan_ft")"
printf "%-20s %10s %10s %10s%%\n" "Avg Draw Calls" "$d3d12_dc" "$vulkan_dc" "$(calc_diff "$d3d12_dc" "$vulkan_dc")"
printf "%-20s %10s %10s %10s%%\n" "Avg Vertices" "$d3d12_v" "$vulkan_v" "$(calc_diff "$d3d12_v" "$vulkan_v")"

echo ""
echo "Logs saved:"
echo "  D3D12:  $d3d12_log"
echo "  Vulkan: $vulkan_log"
