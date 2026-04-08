#!/usr/bin/env python3

from pathlib import Path

from openevolve.evaluation_result import EvaluationResult

from hashcat_md5_mode0_harness import DEFAULT_BASELINE_JSON
from hashcat_md5_mode0_harness import evaluate_candidate


def evaluate(candidate_path: str) -> EvaluationResult:
  result = evaluate_candidate(Path(candidate_path), Path(DEFAULT_BASELINE_JSON), write_baseline_if_missing = True)

  metrics = {
    "combined_score": result["combined_score"],
    "speed_hs": result["speed_hs"],
    "baseline_hs": result["baseline_hs"],
    "speed_ratio": result["speed_ratio"],
    "speed_spread": result["speed_spread"],
    "build_ok": result["build_ok"],
    "correctness_ok": result["correctness_ok"],
    "benchmark_ok": result["benchmark_ok"],
  }

  artifacts = {
    "build_stdout.txt": result.get("build_stdout", ""),
    "build_stderr.txt": result.get("build_stderr", ""),
    "correctness_stdout.txt": result.get("correctness_stdout", ""),
    "correctness_stderr.txt": result.get("correctness_stderr", ""),
    "benchmark_stdout.txt": result.get("benchmark_stdout", ""),
    "benchmark_stderr.txt": result.get("benchmark_stderr", ""),
    "summary.txt": (
      f"combined_score={result['combined_score']}\n"
      f"speed_hs={result['speed_hs']}\n"
      f"baseline_hs={result['baseline_hs']}\n"
      f"speed_ratio={result['speed_ratio']}\n"
      f"speed_spread={result['speed_spread']}\n"
      f"build_ok={result['build_ok']}\n"
      f"correctness_ok={result['correctness_ok']}\n"
      f"benchmark_ok={result['benchmark_ok']}\n"
      f"speed_samples={result.get('speed_samples', [])}\n"
      f"correctness_plaintext={result.get('correctness_plaintext', '')}\n"
      f"correctness_expected_plaintext={result.get('correctness_expected_plaintext', '')}\n"
    ),
  }

  return EvaluationResult(metrics = metrics, artifacts = artifacts)
