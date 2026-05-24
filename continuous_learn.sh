#!/bin/bash
cd "$(cd "$(dirname "$0")" && pwd)"

# iCloud-first storage - no local data accumulation
ICLOUD_AGI="${HOME}/Library/Mobile Documents/com~apple~CloudDocs/AGI_Data/seferim"
export AGI_INGEST_INDEX="${SEFERIM_HOME:-$PWD}/data/multi_api_stream.jsonl"
LOG_FILE="/tmp/seferim_live.log"  # Ephemeral log in /tmp

echo "Starting continuous learning loop..."
echo "Dashboard: http://localhost:9306/"
echo "Storage: iCloud (${ICLOUD_AGI})"
echo "Press Ctrl+C to stop"
echo ""

# Ensure iCloud dirs exist
mkdir -p "${ICLOUD_AGI}/snapshots" "${ICLOUD_AGI}/substrate"

# Clear ephemeral log
> "$LOG_FILE"

ROUND=1
while true; do
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)

    # Snapshots go to iCloud, compressed
    SNAP_DIR="${ICLOUD_AGI}/snapshots/${TIMESTAMP}_r${ROUND}"
    export SEFERIM_SAVE_DIR="/tmp/seferim_snap_${ROUND}"
    mkdir -p "$SEFERIM_SAVE_DIR"

    echo "=== Round $ROUND starting at $(date) ===" >> "$LOG_FILE"

    # Run 5000 ticks (substrate count)
    ./build/agi_bridge_v2 5000 >> "$LOG_FILE" 2>&1

    # Compress and move snapshot to iCloud
    if [ -d "$SEFERIM_SAVE_DIR" ] && [ "$(ls -A $SEFERIM_SAVE_DIR 2>/dev/null)" ]; then
        mkdir -p "$SNAP_DIR"
        cd "$SEFERIM_SAVE_DIR"
        for f in *; do
            [ -f "$f" ] && zstd -19 -f "$f" -o "${SNAP_DIR}/${f}.zst" 2>/dev/null
        done
        cd "$(cd "$(dirname "$0")" && pwd)"
        rm -rf "$SEFERIM_SAVE_DIR"  # Clear local temp
    fi

    # Keep only last 10 snapshots in iCloud to save space
    ls -dt "${ICLOUD_AGI}/snapshots"/*/ 2>/dev/null | tail -n +11 | xargs rm -rf 2>/dev/null

    echo "Round $ROUND complete, sleeping 2s..."
    sleep 2

    ROUND=$((ROUND + 1))
done
