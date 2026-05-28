#include "dupfinder/hashing.hpp"

#include <array>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "xxhash.h"

namespace dupfinder {

namespace {
constexpr std::size_t READ_BUF_SIZE = 64 * 1024;
}

HashValue hash_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    auto state = std::unique_ptr<XXH3_state_t, decltype(&XXH3_freeState)>(
        XXH3_createState(), XXH3_freeState);
    if (!state) {
        throw std::runtime_error("XXH3_createState failed");
    }
    XXH3_64bits_reset(state.get());

    std::array<char, READ_BUF_SIZE> buf{};
    while (file.read(buf.data(), buf.size()) || file.gcount() > 0) {
        const auto n = static_cast<std::size_t>(file.gcount());
        XXH3_64bits_update(state.get(), buf.data(), n);
    }

    return XXH3_64bits_digest(state.get());
}

HashValue hash_file_prefix(const std::filesystem::path& path, std::size_t bytes) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    std::vector<char> buf(bytes);
    file.read(buf.data(), static_cast<std::streamsize>(buf.size()));
    const auto read = static_cast<std::size_t>(file.gcount());

    return XXH3_64bits(buf.data(), read);
}

}  // namespace dupfinder
