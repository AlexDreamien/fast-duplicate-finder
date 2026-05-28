# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Content-based duplicate finder, documented as an optimization case study (v0 naive → v3 parallel). C++17 + CMake, XXH3-64. See `README.md` for the case study and measurements.

## Build & test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
ctest --test-dir build -R <regex>           # single test / subset
# benchmarks:
python benchmarks/generate.py --out benchmarks/data
python benchmarks/run.py --binary build/Release/dupfinder --data benchmarks/data
```

CMake 3.20+, C++17. xxHash and doctest vendored in `third_party/` — no external deps.

## Architecture invariant

Each finder version (`finder_v0..v3.cpp`) is its own translation unit so the README can point at one file per optimization step, and so they can be cross-checked. v1 buckets by size; v2 adds a 4 KiB partial-hash filter; v3 runs v2's algorithm on a thread pool.

## Gotchas — do not regress

- **v0–v3 must report identical duplicate groups on the same input.** There is a cross-version agreement test — any change to one algorithm must keep all four in agreement.
- **XXH3 state is RAII.** `hashing.cpp` wraps `XXH3_state_t` in a `unique_ptr` with `XXH3_freeState` deleter. Do not go back to manual `createState`/`freeState` — a throw between them leaks.
- **v3 worker catches only `std::runtime_error`** (the type `hash_file` throws on I/O failure → skip that file). Do **not** widen to `catch (...)`: that would swallow `std::bad_alloc` and other fatal exceptions and continue in a degraded state. The hash accumulator is initialized (`HashValue h = 0;`) so a skipped file never contributes garbage.
- **`PARTIAL_HASH_BYTES = 4096`.** Files at or below this size skip the partial step (partial hash would equal the full hash).
- **v3 concurrency model:** a single `std::atomic` cursor hands out work; a mutex guards only writes to the shared result map. Keep this shape — don't add shared mutable state outside the lock.
- XXH3 is non-cryptographic — fine for content equality, not for security comparisons.
