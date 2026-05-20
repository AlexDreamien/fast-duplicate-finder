#include "dupfinder/finder.hpp"

#include <unordered_map>

#include "dupfinder/hashing.hpp"

namespace dupfinder {

std::vector<DuplicateGroup> find_duplicates_v1(const std::vector<FileEntry>& entries) {
    std::unordered_map<FileSize, std::vector<std::size_t>> by_size;
    by_size.reserve(entries.size());
    for (std::size_t i = 0; i < entries.size(); ++i) {
        by_size[entries[i].size].push_back(i);
    }

    std::vector<DuplicateGroup> groups;
    for (auto& [size, idxs] : by_size) {
        if (idxs.size() < 2) continue;

        // Empty files: all duplicates without hashing.
        if (size == 0) {
            DuplicateGroup g;
            g.size = 0;
            g.hash = 0;
            g.paths.reserve(idxs.size());
            for (auto idx : idxs) g.paths.push_back(entries[idx].path);
            groups.push_back(std::move(g));
            continue;
        }

        std::unordered_map<HashValue, std::vector<std::size_t>> by_hash;
        for (auto idx : idxs) {
            HashValue h;
            try {
                h = hash_file(entries[idx].path);
            } catch (...) {
                continue;
            }
            by_hash[h].push_back(idx);
        }
        for (auto& [h, hidxs] : by_hash) {
            if (hidxs.size() < 2) continue;
            DuplicateGroup g;
            g.size = size;
            g.hash = h;
            g.paths.reserve(hidxs.size());
            for (auto idx : hidxs) g.paths.push_back(entries[idx].path);
            groups.push_back(std::move(g));
        }
    }
    return groups;
}

}  // namespace dupfinder
