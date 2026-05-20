#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace dupfinder {

using HashValue = std::uint64_t;
using FileSize = std::uintmax_t;

struct FileEntry {
    std::filesystem::path path;
    FileSize size{0};
};

struct DuplicateGroup {
    FileSize size{0};
    HashValue hash{0};
    std::vector<std::filesystem::path> paths;
};

}  // namespace dupfinder
