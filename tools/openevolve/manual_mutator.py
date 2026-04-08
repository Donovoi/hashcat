#!/usr/bin/env python3

import argparse
import json
import random
import re
import time
from pathlib import Path


CURRENT_PROGRAM_RE = re.compile(r"# Current Program\s+```[^\n]*\n(.*?)\n```", re.S)


def extract_current_program (task_payload: dict) -> str:
  display_prompt = str(task_payload.get("display_prompt", ""))

  match = CURRENT_PROGRAM_RE.search(display_prompt)

  if match is not None:
    return match.group(1)

  messages = task_payload.get("messages", [])

  for message in reversed(messages):
    content = str(message.get("content", ""))
    match = CURRENT_PROGRAM_RE.search(content)

    if match is not None:
      return match.group(1)

  raise RuntimeError("Unable to extract current program from OpenEvolve manual-mode task")


def format_diff (search: str, replace: str) -> str:
  return f"<<<<<<< SEARCH\n{search}\n=======\n{replace}\n>>>>>>> REPLACE"


def candidate_mutations (code: str) -> list[tuple[str, str]]:
  mutations = []

  pairs = [
    ("w3[2] = out_len * 8;", "w3[2] = out_len << 3;"),
    ("w3[2] = out_len << 3;", "w3[2] = out_len * 8;"),
    ("const u32 pw_len = pws[gid].pw_len & 63;", "const u32 pw_len = pws[gid].pw_len & 63u;"),
    ("const u32 pw_len = pws[gid].pw_len & 63u;", "const u32 pw_len = pws[gid].pw_len & 63;"),
    ("const u32x out_len = apply_rules_vect_optimized (pw_buf0, pw_buf1, pw_len, rules_buf, il_pos, w0, w1);", "u32x out_len = apply_rules_vect_optimized (pw_buf0, pw_buf1, pw_len, rules_buf, il_pos, w0, w1);"),
    ("u32x out_len = apply_rules_vect_optimized (pw_buf0, pw_buf1, pw_len, rules_buf, il_pos, w0, w1);", "const u32x out_len = apply_rules_vect_optimized (pw_buf0, pw_buf1, pw_len, rules_buf, il_pos, w0, w1);"),
    ("u32x w0[4] = { 0 };", "u32x w0[4] = { 0, 0, 0, 0 };"),
    ("u32x w0[4] = { 0, 0, 0, 0 };", "u32x w0[4] = { 0 };"),
    ("u32x w1[4] = { 0 };", "u32x w1[4] = { 0, 0, 0, 0 };"),
    ("u32x w1[4] = { 0, 0, 0, 0 };", "u32x w1[4] = { 0 };"),
    ("u32x w2[4] = { 0 };", "u32x w2[4] = { 0, 0, 0, 0 };"),
    ("u32x w2[4] = { 0, 0, 0, 0 };", "u32x w2[4] = { 0 };"),
    ("u32x w3[4] = { 0 };", "u32x w3[4] = { 0, 0, 0, 0 };"),
    ("u32x w3[4] = { 0, 0, 0, 0 };", "u32x w3[4] = { 0 };"),
    ("w3[3] = 0;", "w3[3] = 0u;"),
    ("w3[3] = 0u;", "w3[3] = 0;"),
  ]

  for search, replace in pairs:
    if search in code:
      mutations.append((search, replace))

  return mutations


def build_answer (task_payload: dict, rng: random.Random) -> str:
  code = extract_current_program(task_payload)
  mutations = candidate_mutations(code)

  if len(mutations) == 0:
    raise RuntimeError("No bounded MD5 mode 0 mutations matched the current program")

  rng.shuffle(mutations)

  selected = mutations[: max(1, min(2, len(mutations)))]

  return "\n\n".join(format_diff(search, replace) for search, replace in selected)


def process_task (task_path: Path, rng: random.Random) -> None:
  answer_path = task_path.with_suffix(".answer.json")

  if answer_path.exists():
    return

  task_payload = json.loads(task_path.read_text(encoding = "utf-8"))
  answer = build_answer(task_payload, rng)

  answer_path.write_text(json.dumps({"answer": answer}, indent = 2) + "\n", encoding = "utf-8")


def main () -> int:
  parser = argparse.ArgumentParser(description = "Automatic bounded mutator for OpenEvolve manual mode")
  parser.add_argument("--queue-dir", required = True)
  parser.add_argument("--seed", type = int, default = 42)
  parser.add_argument("--poll-interval", type = float, default = 0.5)

  args = parser.parse_args()

  queue_dir = Path(args.queue_dir)
  rng = random.Random(args.seed)

  while True:
    queue_dir.mkdir(parents = True, exist_ok = True)

    for task_path in sorted(queue_dir.glob("*.json")):
      if task_path.name.endswith(".answer.json"):
        continue

      process_task(task_path, rng)

    time.sleep(args.poll_interval)


if __name__ == "__main__":
  raise SystemExit(main())
