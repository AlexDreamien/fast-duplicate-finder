#pragma once

#include <cstddef>
#include <filesystem>

#include "dupfinder/types.hpp"

namespace dupfinder {

// Compute XXH3-64 of the entire file contents.
// Throws std::runtime_error if the file cannot be opened.
HashValue hash_file(const std::filesystem::path& path);

// Compute XXH3-64 of the first `bytes` bytes of the file.
// If the file is shorter, hashes what is available.
HashValue hash_file_prefix(const std::filesystem::path& path, std::size_t bytes);

}  // namespace dupfinder
