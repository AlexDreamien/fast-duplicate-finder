"""Run dupfinder against a generated dataset across all four algorithms.

Reports the best of N runs per algorithm (best is more stable than mean
under filesystem-cache noise) plus its relative speedup over the v0 baseline.

Example:
    python benchmarks/generate.py --out benchmarks/data
    python benchmarks/run.py --binary build/Release/dupfinder.exe --data benchmarks/data

Print a Markdown table that you can drop straight into README.md.
"""

from __future__ import annotations

import argparse
import re
import statistics
import subprocess
import sys
from pathlib import Path

TIMING_RE = re.compile(r"search \(\w+\): (\d+) ms")


def time_algorithm(binary: Path, data: Path, algo: str, runs: int) -> list[int]:
    timings: list[int] = []
    for _ in range(runs):
        proc = subprocess.run(
            [
                str(binary),
                str(data),
                "--algorithm",
                algo,
                "--timing",
                "--format",
                "json",
            ],
            capture_output=True,
            text=True,
            check=True,
        )
        m = TIMING_RE.search(proc.stderr)
        if not m:
            raise RuntimeError(f"Could not parse timing from stderr: {proc.stderr!r}")
        timings.append(int(m.group(1)))
    return timings


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--data", type=Path, required=True)
    p.add_argument("--runs", type=int, default=3)
    p.add_argument(
        "--algorithms",
        nargs="+",
        default=["v0", "v1", "v2", "v3"],
        help="Algorithms to benchmark.",
    )
    args = p.parse_args()

    if not args.binary.exists():
        print(f"Binary not found: {args.binary}", file=sys.stderr)
        return 1
    if not args.data.exists():
        print(f"Dataset not found: {args.data}", file=sys.stderr)
        return 1

    results: dict[str, list[int]] = {}
    for algo in args.algorithms:
        print(f"Running {algo}...", file=sys.stderr)
        results[algo] = time_algorithm(args.binary, args.data, algo, args.runs)
        print(
            f"  {algo}: best={min(results[algo])} ms  "
            f"median={int(statistics.median(results[algo]))} ms  "
            f"all={results[algo]}",
            file=sys.stderr,
        )

    baseline = min(results[args.algorithms[0]]) if args.algorithms else 0
    print()
    print("| Algorithm | Best (ms) | Median (ms) | Speedup vs v0 |")
    print("| --------- | --------- | ----------- | ------------- |")
    for algo in args.algorithms:
        best = min(results[algo])
        median = int(statistics.median(results[algo]))
        speedup = baseline / best if best > 0 else float("inf")
        print(f"| {algo:<9} | {best:>9} | {median:>11} | {speedup:>13.2f}x |")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
