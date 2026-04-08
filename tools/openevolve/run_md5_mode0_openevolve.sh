#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

VENV_DIR="${VENV_DIR:-/tmp/hashcat-openevolve-venv}"
OUTPUT_DIR="${OUTPUT_DIR:-/tmp/hashcat-openevolve-output}"
BASELINE_JSON="${BASELINE_JSON:-${OUTPUT_DIR}/md5_mode0_baseline.json}"
ITERATIONS="${ITERATIONS:-5}"
TARGET_SCORE="${TARGET_SCORE:-1.01}"
SEED="${SEED:-42}"

python3 -m venv "${VENV_DIR}"
. "${VENV_DIR}/bin/activate"

pip install --upgrade pip >/dev/null
pip install -r "${SCRIPT_DIR}/requirements.txt" >/dev/null

mkdir -p "${OUTPUT_DIR}"

cd "${REPO_ROOT}"

make -s PRODUCTION=1
./hashcat -I >/dev/null

python3 "${SCRIPT_DIR}/hashcat_md5_mode0_harness.py" baseline --baseline-json "${BASELINE_JSON}"

export HASHCAT_MD5_MODE0_BASELINE_JSON="${BASELINE_JSON}"

python3 "${SCRIPT_DIR}/manual_mutator.py" \
  --queue-dir "${OUTPUT_DIR}/manual_tasks_queue" \
  --seed "${SEED}" &
MUTATOR_PID=$!

cleanup() {
  kill "${MUTATOR_PID}" >/dev/null 2>&1 || true
}

trap cleanup EXIT

openevolve-run \
  "${REPO_ROOT}/OpenCL/m00000_a0-optimized.cl" \
  "${SCRIPT_DIR}/hashcat_md5_mode0_evaluate.py" \
  --config "${SCRIPT_DIR}/md5_mode0_openevolve.yaml" \
  --output "${OUTPUT_DIR}" \
  --iterations "${ITERATIONS}" \
  --target-score "${TARGET_SCORE}" \
  --log-level INFO
