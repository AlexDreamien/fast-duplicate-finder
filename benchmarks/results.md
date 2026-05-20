# Benchmark results

Reproduce locally:

```bash
python benchmarks/generate.py --out benchmarks/data --files 2000 --seed 42
python benchmarks/run.py --binary build/Release/dupfinder.exe --data benchmarks/data --runs 3
```

The dataset is 2,000 files totalling ~2.1 GiB with 179 duplicate groups (size
distribution: 256 B / 8 KiB / 128 KiB / 2 MiB / 16 MiB, weights 6/5/4/2/1).

Reported numbers are **best of 3 warm-cache runs** in milliseconds. The first
cold-cache run for each algorithm is noticeably slower (v0's cold run was
12.9 s vs 2.57 s warm) because filesystem reads dominate.

## Hardware

- Windows 10, MSVC 19.44, Release build
- _CPU / disk details to be filled in after running on your reference box._

## Reference measurements (sample)

| Algorithm | Best (ms) | Median (ms) | Speedup vs v0 |
| --------- | --------- | ----------- | ------------- |
| v0        |      2570 |        2653 |          1.00x |
| v1        |      2378 |        2508 |          1.08x |
| v2        |       360 |         484 |          7.14x |
| v3        |       248 |         269 |         10.36x |

### Reading the numbers

- **v0 → v1** is modest here because many unique files share size buckets
  with the duplicate groups (random sizes picked from only 5 buckets), so the
  size filter rejects fewer candidates than it would on a more diverse tree.
- **v1 → v2** is the dramatic step. The 4 KiB partial hash eliminates the
  large-file uniques cheaply — we never read the 2 / 16 MiB tails for files
  that already differ in their first 4 KiB.
- **v2 → v3** adds parallelism. The speedup is bounded by I/O concurrency,
  not by CPU, so it lands well below the ~hardware-concurrency factor.

### Reproducibility note

The same `--seed` produces a byte-identical dataset, so the absolute numbers
depend only on the machine running the benchmark. The shapes (v1 ~ v0, v2
order-of-magnitude faster, v3 a further 30–50% on top of v2) should hold
across hardware unless you change the size distribution.
