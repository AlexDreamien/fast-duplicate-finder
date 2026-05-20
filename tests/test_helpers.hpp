#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace dupfinder::test {

namespace fs = std::filesystem;

inline fs::path make_temp_dir(const std::string& prefix = "dupfinder_test_") {
    static std::atomic<std::uint64_t> counter{0};
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    fs::path p = fs::temp_directory_path() /
                 (prefix + std::to_string(stamp) + "_" + std::to_string(counter.fetch_add(1)));
    fs::create_directories(p);
    return p;
}

inline void write_file(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

struct TempDir {
    fs::path path;
    TempDir() : path(make_temp_dir()) {}
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
};

}  // namespace dupfinder::test
