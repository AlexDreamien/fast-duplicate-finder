#include "dupfinder/finder.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "dupfinder/hashing.hpp"

namespace dupfinder {

namespace {

// Compute a hash for each entry in `idxs` using a thread pool. `hasher` takes
// the entry index and returns its hash; throws on read failure (the entry is
// then silently skipped).
template <typename Hasher>
std::unordered_map<HashValue, std::vector<std::size_t>>
parallel_hash(const std::vector<std::size_t>& idxs, std::size_t threads, Hasher hasher) {
    std::unordered_map<HashValue, std::vector<std::size_t>> result;
    std::mutex mu;
    std::atomic<std::size_t> cursor{0};

    if (threads == 0) threads = std::thread::hardware_concurrency();
    if (threads == 0) threads = 1;
    threads = std::min(threads, idxs.size());
    if (threads <= 1) {
        for (auto idx : idxs) {
            HashValue h = 0;
            try {
                h = hasher(idx);
            } catch (const std::runtime_error&) {
                continue;
            }
            result[h].push_back(idx);
        }
        return result;
    }

    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (std::size_t t = 0; t < threads; ++t) {
        workers.emplace_back([&]() {
            for (;;) {
                const std::size_t i = cursor.fetch_add(1, std::memory_order_relaxed);
                if (i >= idxs.size()) return;
                const std::size_t idx = idxs[i];
                HashValue h = 0;
                try {
                    h = hasher(idx);
                } catch (const std::runtime_error&) {
                    continue;
                }
                std::lock_guard<std::mutex> lock(mu);
                result[h].push_back(idx);
            }
        });
    }
    for (auto& w : workers) w.join();
    return result;
}

void emit_groups(const std::vector<FileEntry>& entries,
                 const std::unordered_map<HashValue, std::vector<std::size_t>>& by_hash,
                 FileSize size,
                 std::vector<DuplicateGroup>& out) {
    for (const auto& [h, idxs] : by_hash) {
        if (idxs.size() < 2) continue;
        DuplicateGroup g;
        g.size = size;
        g.hash = h;
        g.paths.reserve(idxs.size());
        for (auto idx : idxs) g.paths.push_back(entries[idx].path);
        out.push_back(std::move(g));
    }
}

}  // namespace

std::vector<DuplicateGroup> find_duplicates_v3(const std::vector<FileEntry>& entries,
                                               std::size_t threads) {
    std::unordered_map<FileSize, std::vector<std::size_t>> by_size;
    by_size.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        by_size[entries[i].size].push_back(i);
    }

    std::vector<DuplicateGroup> groups;
    for (auto& [size, idxs] : by_size) {
        if (idxs.size() < 2) continue;

        if (size == 0) {
            DuplicateGroup g;
            g.size = 0;
            g.hash = 0;
            g.paths.reserve(idxs.size());
            for (auto idx : idxs) g.paths.push_back(entries[idx].path);
            groups.push_back(std::move(g));
            continue;
        }

        if (size <= PARTIAL_HASH_BYTES) {
            const auto by_hash = parallel_hash(idxs, threads, [&](std::size_t idx) {
                return hash_file(entries[idx].path);
            });
            emit_groups(entries, by_hash, size, groups);
            continue;
        }

        const auto by_partial = parallel_hash(idxs, threads, [&](std::size_t idx) {
            return hash_file_prefix(entries[idx].path, PARTIAL_HASH_BYTES);
        });
        for (const auto& [ph, pidxs] : by_partial) {
            if (pidxs.size() < 2) continue;
            const auto by_full = parallel_hash(pidxs, threads, [&](std::size_t idx) {
                return hash_file(entries[idx].path);
            });
            emit_groups(entries, by_full, size, groups);
        }
    }
    return groups;
}

}  // namespace dupfinder
