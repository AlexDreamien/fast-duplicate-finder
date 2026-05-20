#include "dupfinder/finder.hpp"

#include <unordered_map>

#include "dupfinder/hashing.hpp"

namespace dupfinder {

namespace {

void emit_full_hash_groups(const std::vector<FileEntry>& entries,
                           const std::vector<std::size_t>& idxs,
                           FileSize size,
                           std::vector<DuplicateGroup>& out) {
    std::unordered_map<HashValue, std::vector<std::size_t>> by_full;
    for (auto idx : idxs) {
        HashValue h;
        try {
            h = hash_file(entries[idx].path);
        } catch (...) {
            continue;
        }
        by_full[h].push_back(idx);
    }
    for (auto& [h, hidxs] : by_full) {
        if (hidxs.size() < 2) continue;
        DuplicateGroup g;
        g.size = size;
        g.hash = h;
        g.paths.reserve(hidxs.size());
        for (auto idx : hidxs) g.paths.push_back(entries[idx].path);
        out.push_back(std::move(g));
    }
}

}  // namespace

std::vector<DuplicateGroup> find_duplicates_v2(const std::vector<FileEntry>& entries) {
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

        // For small files, the partial hash equals the full hash, so skip it.
        if (size <= PARTIAL_HASH_BYTES) {
            emit_full_hash_groups(entries, idxs, size, groups);
            continue;
        }

        // Partial-hash filter
        std::unordered_map<HashValue, std::vector<std::size_t>> by_partial;
        for (auto idx : idxs) {
            HashValue h;
            try {
                h = hash_file_prefix(entries[idx].path, PARTIAL_HASH_BYTES);
            } catch (...) {
                continue;
            }
            by_partial[h].push_back(idx);
        }
        for (auto& [ph, pidxs] : by_partial) {
            if (pidxs.size() < 2) continue;
            emit_full_hash_groups(entries, pidxs, size, groups);
        }
    }
    return groups;
}

}  // namespace dupfinder
