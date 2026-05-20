"""Generate a reproducible test dataset for fast-duplicate-finder benchmarks.

The dataset mixes unique files with duplicate groups across a spread of file
sizes, so each optimisation layer (size-bucketing, partial-hash, parallel
hashing) actually has work to skip or share.

Run:
    python benchmarks/generate.py --out benchmarks/data --seed 42

Re-running with the same seed produces byte-identical content, so the
recorded measurements stay reproducible.
"""

from __future__ import annotations

import argparse
import random
import shutil
import sys
from pathlib import Path


SIZE_BUCKETS = [
    # (bytes, weight) — distribution of file sizes.
    (256, 6),         # tiny: <4 KiB partial-hash threshold
    (8 * 1024, 5),    # small
    (128 * 1024, 4),  # medium
    (2 * 1024 * 1024, 2),  # large
    (16 * 1024 * 1024, 1),  # very large
]


def pick_size(rng: random.Random) -> int:
    weights = [w for _, w in SIZE_BUCKETS]
    sizes = [s for s, _ in SIZE_BUCKETS]
    return rng.choices(sizes, weights=weights, k=1)[0]


def random_bytes(rng: random.Random, n: int) -> bytes:
    return rng.randbytes(n)


def build(out: Path, total_files: int, duplicate_ratio: float, seed: int) -> None:
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    rng = random.Random(seed)
    duplicate_files = int(total_files * duplicate_ratio)
    unique_files = total_files - duplicate_files

    # Decide group sizes (2..5 files per duplicate set)
    duplicate_groups = []
    remaining = duplicate_files
    while remaining >= 2:
        n = rng.randint(2, min(5, remaining))
        duplicate_groups.append(n)
        remaining -= n
    unique_files += remaining  # leftover singletons

    total_bytes = 0
    fileno = 0

    for gid, group_size in enumerate(duplicate_groups):
        size = pick_size(rng)
        payload = random_bytes(rng, size)
        for i in range(group_size):
            subdir = out / f"group_{gid % 10}" / f"sub_{(gid // 10) % 10}"
            subdir.mkdir(parents=True, exist_ok=True)
            p = subdir / f"dup_{gid:05d}_{i}.bin"
            p.write_bytes(payload)
            fileno += 1
            total_bytes += size

    for _ in range(unique_files):
        size = pick_size(rng)
        payload = random_bytes(rng, size)
        subdir = out / f"unique_{fileno % 100 // 10}"
        subdir.mkdir(parents=True, exist_ok=True)
        p = subdir / f"uniq_{fileno:06d}.bin"
        p.write_bytes(payload)
        fileno += 1
        total_bytes += size

    print(
        f"Generated {fileno} files "
        f"({len(duplicate_groups)} duplicate groups, "
        f"{total_bytes / (1024 * 1024):.1f} MB) at {out}",
        file=sys.stderr,
    )


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--out", type=Path, default=Path("benchmarks/data"))
    p.add_argument("--files", type=int, default=2000, help="Approximate file count.")
    p.add_argument("--duplicate-ratio", type=float, default=0.3)
    p.add_argument("--seed", type=int, default=42)
    args = p.parse_args()
    build(args.out, args.files, args.duplicate_ratio, args.seed)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
