#!/usr/bin/env python3

import argparse
import json
import os
import re
import shutil
import statistics
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[2]
HASHCAT_BIN = REPO_ROOT / "hashcat"
TARGET_KERNEL = REPO_ROOT / "OpenCL" / "m00000_a0-optimized.cl"
MODULE_REFERENCE = REPO_ROOT / "src" / "modules" / "module_00000.c"

DEFAULT_BASELINE_JSON = Path(os.environ.get("HASHCAT_MD5_MODE0_BASELINE_JSON", "/tmp/hashcat-md5-mode0-baseline.json"))
DEFAULT_WORDLIST_SIZE = 50000

SPEED_RE = re.compile(r"Speed\.#\d+.*?:\s*([0-9][0-9.,]*)\s*([kKMGT]?H/s)")
ST_PASS_RE = re.compile(r'static const char \*ST_PASS\s*=\s*"([^"]+)";')
ST_HASH_RE = re.compile(r'static const char \*ST_HASH\s*=\s*"([^"]+)";')

SPEED_UNITS = {
  "H/s": 1.0,
  "kH/s": 1_000.0,
  "KH/s": 1_000.0,
  "MH/s": 1_000_000.0,
  "GH/s": 1_000_000_000.0,
  "TH/s": 1_000_000_000_000.0,
}


@dataclass
class CommandResult:
  rc: int
  stdout: str
  stderr: str


def parse_reference_pair() -> tuple[str, str]:
  module_text = MODULE_REFERENCE.read_text(encoding = "utf-8")

  st_pass_match = ST_PASS_RE.search(module_text)
  st_hash_match = ST_HASH_RE.search(module_text)

  if st_pass_match is None or st_hash_match is None:
    raise RuntimeError(f"Failed to parse MD5 self-test reference from {MODULE_REFERENCE}")

  return st_hash_match.group(1), st_pass_match.group(1)


def run_cmd(cmd: list[str], env: dict[str, str], cwd: Path) -> CommandResult:
  proc = subprocess.run(
    cmd,
    cwd = str(cwd),
    env = env,
    capture_output = True,
    text = True,
    check = False)

  return CommandResult(proc.returncode, proc.stdout, proc.stderr)


def make_eval_env(workspace: Path) -> dict[str, str]:
  env = os.environ.copy()

  xdg_cache_home = workspace / "xdg-cache"
  xdg_data_home  = workspace / "xdg-data"
  home_dir       = workspace / "home"

  xdg_cache_home.mkdir(parents = True, exist_ok = True)
  xdg_data_home.mkdir(parents = True, exist_ok = True)
  home_dir.mkdir(parents = True, exist_ok = True)

  env["HOME"] = str(home_dir)
  env["XDG_CACHE_HOME"] = str(xdg_cache_home)
  env["XDG_DATA_HOME"] = str(xdg_data_home)

  return env


def hashcat_base_cmd(workspace: Path) -> list[str]:
  return [
    str(HASHCAT_BIN),
    "-m", "0",
    "-a", "0",
    "-D", "1",
    "-d", "1",
    "-O",
    "--backend-vector-width", "1",
    "--backend-ignore-cuda",
    "--backend-ignore-hip",
    "--quiet",
    "--potfile-disable",
    "--logfile-disable",
    "--force",
    "--session", f"openevolve-md5-mode0-{workspace.name}",
  ]


def build_wordlist(path: Path, reference_plaintext: str, count: int) -> None:
  with path.open("w", encoding = "utf-8") as fp:
    for idx in range(count):
      fp.write(f"hashcat-openevolve-{idx:05d}\n")

    fp.write(f"{reference_plaintext}\n")


def parse_speed(output: str) -> float:
  match = SPEED_RE.search(output)

  if match is None:
    raise RuntimeError(f"Could not parse hashcat speed from output:\n{output}")

  value = float(match.group(1).replace(",", ""))
  unit  = match.group(2)

  return value * SPEED_UNITS[unit]


def ensure_hashcat_built(env: dict[str, str]) -> CommandResult:
  return run_cmd(["make", "-s", "PRODUCTION=1"], env, REPO_ROOT)


def run_correctness_check(env: dict[str, str], workspace: Path) -> dict[str, Any]:
  target_hash, reference_plaintext = parse_reference_pair()

  hash_file = workspace / "md5.hash"
  word_file = workspace / "md5.wordlist"
  out_file  = workspace / "md5.out"

  hash_file.write_text(f"{target_hash}\n", encoding = "utf-8")
  build_wordlist(word_file, reference_plaintext, 32)

  cmd = hashcat_base_cmd(workspace)
  cmd.extend([
    "--outfile-autohex-disable",
    "--outfile", str(out_file),
    "--outfile-format", "2",
    str(hash_file),
    str(word_file),
  ])

  result = run_cmd(cmd, env, REPO_ROOT)

  recovered_plaintext = ""

  if out_file.exists():
    recovered_plaintext = out_file.read_text(encoding = "utf-8").strip()

  cracked_ok = (result.rc == 0 and recovered_plaintext == reference_plaintext)

  return {
    "correctness_ok": float(cracked_ok),
    "correctness_stdout": result.stdout,
    "correctness_stderr": result.stderr,
    "correctness_plaintext": recovered_plaintext,
    "correctness_expected_plaintext": reference_plaintext,
    "correctness_rc": result.rc,
  }


def run_speed_check(env: dict[str, str], workspace: Path, samples: int, warmup: int) -> dict[str, Any]:
  target_hash, reference_plaintext = parse_reference_pair()

  hash_file = workspace / "bench.hash"
  word_file = workspace / "bench.wordlist"

  hash_file.write_text(f"{target_hash}\n", encoding = "utf-8")
  build_wordlist(word_file, reference_plaintext, DEFAULT_WORDLIST_SIZE)

  cmd = hashcat_base_cmd(workspace)
  cmd.extend([
    "--speed-only",
    str(hash_file),
    str(word_file),
  ])

  warmup_outputs = []
  measured = []
  measured_outputs = []

  for _ in range(warmup):
    result = run_cmd(cmd, env, REPO_ROOT)

    if result.rc != 0:
      return {
        "benchmark_ok": 0.0,
        "benchmark_stdout": result.stdout,
        "benchmark_stderr": result.stderr,
        "benchmark_rc": result.rc,
        "speed_samples": [],
        "speed_hs": 0.0,
        "speed_spread": 1.0,
      }

    warmup_outputs.append(result.stdout)

  for _ in range(samples):
    result = run_cmd(cmd, env, REPO_ROOT)

    if result.rc != 0:
      return {
        "benchmark_ok": 0.0,
        "benchmark_stdout": result.stdout,
        "benchmark_stderr": result.stderr,
        "benchmark_rc": result.rc,
        "speed_samples": [],
        "speed_hs": 0.0,
        "speed_spread": 1.0,
      }

    measured.append(parse_speed(result.stdout))
    measured_outputs.append(result.stdout)

  median_speed = statistics.median(measured)
  spread = 0.0

  if median_speed > 0:
    spread = (max(measured) - min(measured)) / median_speed

  return {
    "benchmark_ok": 1.0,
    "benchmark_stdout": (
      "=== warmup ===\n"
      + ("\n--- warmup run ---\n".join(warmup_outputs) if warmup_outputs else "")
      + "\n=== measured ===\n"
      + ("\n--- measured run ---\n".join(measured_outputs) if measured_outputs else "")
    ),
    "benchmark_stderr": "",
    "benchmark_rc": 0,
    "speed_samples": measured,
    "speed_hs": median_speed,
    "speed_spread": spread,
  }


def load_baseline(path: Path) -> dict[str, Any]:
  return json.loads(path.read_text(encoding = "utf-8"))


def write_baseline(path: Path, data: dict[str, Any]) -> None:
  path.parent.mkdir(parents = True, exist_ok = True)
  path.write_text(json.dumps(data, indent = 2, sort_keys = True) + "\n", encoding = "utf-8")


def evaluate_candidate(candidate_path: Path, baseline_path: Path = DEFAULT_BASELINE_JSON, write_baseline_if_missing: bool = False) -> dict[str, Any]:
  original_text = TARGET_KERNEL.read_text(encoding = "utf-8")
  candidate_text = candidate_path.read_text(encoding = "utf-8")

  build_result = CommandResult(0, "", "")
  correctness = {}
  benchmark = {}

  with tempfile.TemporaryDirectory(prefix = "hashcat-openevolve-eval-") as tmp_dir:
    workspace = Path(tmp_dir)
    env = make_eval_env(workspace)

    try:
      TARGET_KERNEL.write_text(candidate_text, encoding = "utf-8")

      build_result = ensure_hashcat_built(env)

      if build_result.rc == 0:
        correctness = run_correctness_check(env, workspace)

      if build_result.rc == 0 and correctness.get("correctness_ok") == 1.0:
        benchmark = run_speed_check(env, workspace, samples = 3, warmup = 1)
    finally:
      TARGET_KERNEL.write_text(original_text, encoding = "utf-8")

  build_ok = float(build_result.rc == 0)
  correctness_ok = float(correctness.get("correctness_ok", 0.0))
  benchmark_ok = float(benchmark.get("benchmark_ok", 0.0))
  speed_hs = float(benchmark.get("speed_hs", 0.0))
  speed_spread = float(benchmark.get("speed_spread", 1.0))

  if baseline_path.exists():
    baseline = load_baseline(baseline_path)
  elif write_baseline_if_missing:
    baseline = {
      "speed_hs": speed_hs,
      "speed_spread": speed_spread,
      "build_ok": build_ok,
      "correctness_ok": correctness_ok,
      "benchmark_ok": benchmark_ok,
    }
    write_baseline(baseline_path, baseline)
  else:
    raise RuntimeError(f"Baseline file not found: {baseline_path}")

  baseline_hs = float(baseline.get("speed_hs", 0.0))
  speed_ratio = 0.0

  if baseline_hs > 0 and speed_hs > 0:
    speed_ratio = speed_hs / baseline_hs

  stability_penalty = min(speed_spread, 0.25)

  combined_score = 0.0

  if build_ok == 1.0 and correctness_ok == 1.0 and benchmark_ok == 1.0:
    combined_score = max(speed_ratio - stability_penalty, 0.0)

  result = {
    "combined_score": combined_score,
    "build_ok": build_ok,
    "correctness_ok": correctness_ok,
    "benchmark_ok": benchmark_ok,
    "speed_hs": speed_hs,
    "baseline_hs": baseline_hs,
    "speed_ratio": speed_ratio,
    "speed_spread": speed_spread,
    "build_stdout": build_result.stdout,
    "build_stderr": build_result.stderr,
    **correctness,
    **benchmark,
  }

  return result


def baseline_current_kernel(baseline_path: Path) -> dict[str, Any]:
  result = evaluate_candidate(TARGET_KERNEL, baseline_path, write_baseline_if_missing = True)

  baseline = {
    "build_ok": result["build_ok"],
    "correctness_ok": result["correctness_ok"],
    "benchmark_ok": result["benchmark_ok"],
    "speed_hs": result["speed_hs"],
    "speed_spread": result["speed_spread"],
  }

  write_baseline(baseline_path, baseline)

  return result


def make_json_safe(obj: Any) -> Any:
  if isinstance(obj, Path):
    return str(obj)

  if isinstance(obj, dict):
    return {key: make_json_safe(value) for key, value in obj.items()}

  if isinstance(obj, list):
    return [make_json_safe(value) for value in obj]

  return obj


def main() -> int:
  parser = argparse.ArgumentParser(description = "Hashcat MD5 mode 0 OpenEvolve harness")

  subparsers = parser.add_subparsers(dest = "command", required = True)

  baseline_parser = subparsers.add_parser("baseline")
  baseline_parser.add_argument("--baseline-json", default = str(DEFAULT_BASELINE_JSON))

  evaluate_parser = subparsers.add_parser("evaluate")
  evaluate_parser.add_argument("candidate")
  evaluate_parser.add_argument("--baseline-json", default = str(DEFAULT_BASELINE_JSON))

  args = parser.parse_args()

  if args.command == "baseline":
    result = baseline_current_kernel(Path(args.baseline_json))
  else:
    result = evaluate_candidate(Path(args.candidate), Path(args.baseline_json))

  print(json.dumps(make_json_safe(result), indent = 2, sort_keys = True))

  return 0


if __name__ == "__main__":
  raise SystemExit(main())
