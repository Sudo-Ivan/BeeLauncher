#!/bin/bash
# Startup bench: runs the launcher N times headless and prints median and min
# time to first frame for classic and shell modes.
# Requires BEE_STARTUP_PROFILE log output from the app (first frame milestone).
# Usage: startup_bench.sh [binary] [N]   defaults: /tmp/bee-build/beelauncher, 7
set -e
BIN=${1:-/tmp/bee-build/beelauncher}
N=${2:-7}
DATA=${BEE_BENCH_DATA:-/tmp/bee-bench-data}
mkdir -p "$DATA"

run_mode() {
    local mode="$1"; shift
    local times=()
    for i in $(seq 1 "$N"); do
        log=$(mktemp)
        env BEE_STARTUP_PROFILE=1 BEE_SHELL_GRAB=/tmp/bee-bench-grab.png QT_QPA_PLATFORM=offscreen QSG_RHI_BACKEND=opengl \
            timeout 6 "$BIN" -d "$DATA" "$@" > "$log" 2>&1 || true
        out=$(grep -m1 "first frame" "$log" || true)
        rm -f "$log"
        # expected log line shape: [profile] first frame: NNN ms
        ms=$(echo "$out" | grep -oE '[0-9]+' | head -1 || true)
        [ -n "$ms" ] && times+=("$ms")
    done
    if [ ${#times[@]} -eq 0 ]; then
        echo "$mode: no profile output"
        return
    fi
    sorted=$(printf '%s\n' "${times[@]}" | sort -n)
    min=$(echo "$sorted" | head -1)
    count=${#times[@]}
    mid=$((count / 2))
    if ((count % 2)); then median=$(echo "$sorted" | sed -n "$((mid+1))p"); else
        a=$(echo "$sorted" | sed -n "${mid}p"); b=$(echo "$sorted" | sed -n "$((mid+1))p")
        median=$(( (a + b) / 2 ))
    fi
    echo "$mode: min=${min}ms median=${median}ms over ${count} runs (times: $(echo $sorted | tr '\n' ' '))"
}

run_mode classic --classic-ui
run_mode shell
