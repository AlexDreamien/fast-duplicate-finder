#include "dupfinder/finder.hpp"

#include <unordered_map>

#include "dupfinder/hashing.hpp"

namespace dupfinder {

std::vector<DuplicateGroup> find_duplicates_v0(const std::vector<FileEntry>& entries) {
    std::unordered_map<HashValue, std::vector<std::size_t>> by_hash;

    for (std::size_t i = 0; i < entries.size(); ++i) {
        HashValue h;
        try {
            h = hash_file(entries[i].path);
        } catch (...) {
            continue;
        }
        by_hash[h].push_back(i);
    }

    std::vector<DuplicateGroup> groups;
    groups.reserve(by_hash.size());
    for (auto& [h, idxs] : by_hash) {
        if (idxs.size() < 2) continue;
        DuplicateGroup g;
        g.hash = h;
        g.size = entries[idxs.front()].size;
        g.paths.reserve(idxs.size());
        for (auto idx : idxs) g.paths.push_back(entries[idx].path);
        groups.push_back(std::move(g));
    }
    return groups;
}

}  // namespace dupfinder
