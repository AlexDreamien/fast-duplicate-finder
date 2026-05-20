# fast-duplicate-finder

[![CI](https://github.com/AlexDreamien/fast-duplicate-finder/actions/workflows/ci.yml/badge.svg)](https://github.com/AlexDreamien/fast-duplicate-finder/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/)

Find duplicate files in a directory tree by content. Documented as an
**optimization case study** — the README walks from a naive baseline through
three optimizations, each with real measurements on a generated dataset.

> _Sample terminal output (placeholder — replace with a real screenshot of
> `dupfinder.exe <path> --timing` once you have one)._
>
> ![Screenshot placeholder](docs/screenshot.png)

## Optimization case study

The goal is a CLI that, given a directory, prints groups of files with
identical contents. The interesting part is **how much work you can avoid**.

### Problem

Naive solution: hash every file, group by hash. Correct but expensive — every
byte of every file is read, even when no duplicate exists.

### v0 — naive baseline

Hash every file with [XXH3-64](https://github.com/Cyan4973/xxHash), group by
hash, report groups with two or more files.

> Source: [`src/finder_v0.cpp`](src/finder_v0.cpp)

### v1 — bucket by size first

Two files of different sizes cannot be duplicates. Bucket by `file_size`
first; hash only inside buckets that contain two or more files. Empty files
short-circuit (every empty file matches every other).

> Source: [`src/finder_v1.cpp`](src/finder_v1.cpp)

### v2 — partial-hash filter

Most non-duplicate files differ in their first few kilobytes. Read only the
first 4 KiB and hash that; if a file is alone in its partial-hash bucket, its
tail cannot match anyone, so we never read it. Full hashes are computed only
when the partial filter leaves more than one candidate. Files at or below the
partial-hash threshold skip the partial step (it would equal the full hash).

> Source: [`src/finder_v2.cpp`](src/finder_v2.cpp), `PARTIAL_HASH_BYTES = 4096`

### v3 — parallel hashing

Hashing is I/O-bound on warm caches but trivially parallelisable. v3 keeps
v2's algorithm and runs the partial-hash and full-hash passes on a thread
pool (`std::thread::hardware_concurrency()` by default). Threads pull work
from a shared atomic cursor.

> Source: [`src/finder_v3.cpp`](src/finder_v3.cpp)

### Measurements

Dataset: 2,000 files, ~2.1 GiB, 179 duplicate groups, sizes distributed
across 256 B / 8 KiB / 128 KiB / 2 MiB / 16 MiB buckets (Windows 10, MSVC
19.44, Release, best of 3 warm-cache runs).

| Algorithm | Best (ms) | Median (ms) | Speedup vs v0 |
| --------- | --------- | ----------- | ------------- |
| v0        |      2570 |        2653 |          1.00x |
| v1        |      2378 |        2508 |          1.08x |
| v2        |       360 |         484 |          7.14x |
| v3        |       248 |         269 |         10.36x |

The **partial-hash filter is the dominant win** (~7x). Parallelism on top of
it adds another ~1.5x; with this dataset I/O concurrency caps further
improvement. v1 is only modestly better than v0 here because the size
distribution has just five buckets — a more diverse tree would amplify it.
See [`benchmarks/results.md`](benchmarks/results.md) for the full notes and
reproduction recipe.

### Honesty disclaimer

Numbers above are real measurements on the dev box. Re-run with:

```bash
python benchmarks/generate.py --out benchmarks/data
python benchmarks/run.py --binary build/Release/dupfinder --data benchmarks/data
```

The seed (`--seed 42`) makes the dataset byte-identical across machines, so
ratios are comparable; absolute times depend on your hardware.

## Building

Requires CMake 3.20+ and a C++17 compiler (MSVC 2019+, GCC 8+, Clang 7+).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

xxHash and doctest are vendored in [`third_party/`](third_party/) — no extra
dependencies.

## Usage

```text
Usage: dupfinder <path> [options]

Options:
  --algorithm {v0,v1,v2,v3}  Which finder algorithm to use (default: v3).
  --min-size BYTES            Skip files smaller than this many bytes.
  --format {text,json}        Output format (default: text).
  --threads N                 Worker thread count for v3 (default: hw concurrency).
  --timing                    Print scan and search timings to stderr.
  -h, --help                  Show this help and exit.
```

Example:

```bash
$ dupfinder ./photos --min-size 1024 --timing
scan: 1834 files in 18 ms
search (v3): 142 ms
12 duplicate group(s)

Group 1: 3 files, each 2.41 MB (wasted 4.82 MB)
  ./photos/2024/06/IMG_2310.JPG
  ./photos/imports/IMG_2310.JPG
  ./photos/backup/IMG_2310.JPG
...
Total reclaimable: 184.50 MB
```

JSON output (`--format json`) is one object per group with `size`, `hash`,
and `paths`.

## Architecture

```
include/dupfinder/
  types.hpp        FileEntry, DuplicateGroup, HashValue/FileSize aliases
  scan.hpp         scan_directory() with error tolerance
  hashing.hpp      hash_file() / hash_file_prefix() (XXH3-64)
  finder.hpp       find_duplicates_v0..v3()
  format.hpp       Text + JSON output, human-readable sizes
src/               Implementations of the above
tests/             doctest suite (hashing, scan, finder correctness +
                   cross-version agreement, edge cases)
benchmarks/        Dataset generator and benchmark runner
third_party/       Vendored xxHash and doctest single-headers
```

Each finder version is its own translation unit so they stay isolated and
the README can point at one file per optimisation step.

## Tests & CI

`tests/` uses [doctest](https://github.com/doctest/doctest):

- Hashing determinism, identical-content equality, large-file streaming,
  partial-hash edge cases
- Scan recursion, min-size filtering, error counting
- Finder correctness on a known fixture + **cross-version agreement** (all
  four algorithms must report the same duplicate groups on the same input)
- Empty input, all-unique input, all-empty-files
- v2's partial-hash boundary case (files that share the first 4 KiB but
  differ afterwards)

GitHub Actions runs `cmake --build` and `ctest` on both Ubuntu and Windows
on every push / PR — see [`.github/workflows/ci.yml`](.github/workflows/ci.yml).

## Out of scope

By design: no auto-deletion (the tool only reports), no GUI, no
cryptographic hashing — XXH3 is a non-cryptographic hash, fine for
content-equality but not for security-sensitive comparisons.

## License

MIT — see [LICENSE](LICENSE). Vendored dependencies keep their upstream
licenses, see [`third_party/README.md`](third_party/README.md).
