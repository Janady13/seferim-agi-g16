#!/usr/bin/env bash
set -euo pipefail

ROOT="${HOME}/SEFERIM_AGI_G16"
source "${ROOT}/config/tuned_weights_v2.env"
export G16_WEIGHTS G16_GAMMA_MIN G16_LAMBDA SEFERIM_TARGET_COH SEFERIM_LR_BASE SEFERIM_LR_GAIN
export AGI_INGEST_GATE AGI_INGEST_MAX AGI_INGEST_INDEX

TICKS="${1:-1000}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG="${ROOT}/data/bridge_v2_run_${TIMESTAMP}.log"

export SEFERIM_SAVE_DIR="${SEFERIM_SNAPSHOT_BASE}/${TIMESTAMP}"
mkdir -p "${SEFERIM_SAVE_DIR}" "${ROOT}/data"

echo "Running agi_bridge_v2 for ${TICKS} ticks (Fibonacci+Spiral+Resonance) ..."
"${ROOT}/build/agi_bridge_v2" "${TICKS}" | tee "${LOG}"

echo ""
echo "Snapshot: ${SEFERIM_SAVE_DIR}"
echo "Log: ${LOG}"
