#pragma once

#include <cstddef>
#include <vector>

#include "dupfinder/types.hpp"

namespace dupfinder {

// Number of bytes used by partial-hash filtering (v2/v3).
inline constexpr std::size_t PARTIAL_HASH_BYTES = 4096;

// v0 — naive baseline: full XXH3 hash of every file, group by hash.
std::vector<DuplicateGroup> find_duplicates_v0(const std::vector<FileEntry>& entries);

// v1 — first bucket by file size, then full-hash only files in size-buckets
// containing two or more entries.
std::vector<DuplicateGroup> find_duplicates_v1(const std::vector<FileEntry>& entries);

// v2 — like v1 plus a partial-hash filter (the first PARTIAL_HASH_BYTES bytes)
// before any full hash.
std::vector<DuplicateGroup> find_duplicates_v2(const std::vector<FileEntry>& entries);

// v3 — like v2 but performs all hash work on a thread pool.
// `threads == 0` uses std::thread::hardware_concurrency().
std::vector<DuplicateGroup> find_duplicates_v3(const std::vector<FileEntry>& entries,
                                               std::size_t threads = 0);

}  // namespace dupfinder
