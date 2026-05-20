#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "dupfinder/types.hpp"

namespace dupfinder {

struct ScanResult {
    std::vector<FileEntry> entries;
    std::size_t errors{0};  // files we couldn't stat (permission, broken link, etc.)
};

// Recursively walks `root`, returning every regular file >= min_size.
// Inaccessible entries are counted in `errors` and otherwise skipped.
ScanResult scan_directory(const std::filesystem::path& root, FileSize min_size = 0);

}  // namespace dupfinder
