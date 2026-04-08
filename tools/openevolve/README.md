# OpenEvolve MD5 mode 0 harness

This folder contains a bounded OpenEvolve integration for `OpenCL/m00000_a0-optimized.cl`.

## Scope

- target kernel: `OpenCL/m00000_a0-optimized.cl`
- correctness reference: `src/modules/module_00000.c`
- fixed backend policy: OpenCL only, device type CPU, device id 1, vector width 1
- benchmark method: mode 0 `--speed-only`

## Files

- `hashcat_md5_mode0_harness.py`: baseline creation plus candidate evaluation
- `hashcat_md5_mode0_evaluate.py`: OpenEvolve evaluator entrypoint
- `manual_mutator.py`: unattended bounded mutator for OpenEvolve manual mode
- `md5_mode0_openevolve.yaml`: reproducible OpenEvolve configuration
- `run_md5_mode0_openevolve.sh`: end-to-end runner
- `requirements.txt`: pinned OpenEvolve dependency

## Local setup

Install a usable OpenCL runtime before benchmarking. A CPU fallback such as `pocl-opencl-icd` works for local smoke tests.

## Baseline only

```bash
cd /home/runner/work/hashcat/hashcat
python3 tools/openevolve/hashcat_md5_mode0_harness.py baseline \
  --baseline-json /tmp/hashcat-openevolve-output/md5_mode0_baseline.json
```

## Run OpenEvolve

```bash
cd /home/runner/work/hashcat/hashcat
ITERATIONS=5 OUTPUT_DIR=/tmp/hashcat-openevolve-output \
  tools/openevolve/run_md5_mode0_openevolve.sh
```

The runner keeps OpenEvolve output outside the source tree and restores the target kernel after each evaluation. The bounded mutator only emits local SEARCH/REPLACE edits for the MD5 fast-path kernel body and directly related expressions.
