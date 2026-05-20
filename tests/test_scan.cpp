#include "doctest.h"

#include <algorithm>

#include "dupfinder/scan.hpp"
#include "test_helpers.hpp"

using namespace dupfinder;
using namespace dupfinder::test;

TEST_CASE("scan finds files recursively") {
    TempDir tmp;
    write_file(tmp.path / "a.txt", "hi");
    write_file(tmp.path / "sub" / "b.txt", "hello");
    write_file(tmp.path / "sub" / "deep" / "c.txt", "deeper");

    const auto result = scan_directory(tmp.path);
    CHECK(result.entries.size() == 3);
    CHECK(result.errors == 0);
}

TEST_CASE("scan ignores files below min_size") {
    TempDir tmp;
    write_file(tmp.path / "tiny.bin", "ab");           // 2 bytes
    write_file(tmp.path / "big.bin", std::string(100, 'x'));  // 100 bytes

    const auto result = scan_directory(tmp.path, 10);
    CHECK(result.entries.size() == 1);
    CHECK(result.entries[0].size == 100);
}

TEST_CASE("scan records file sizes") {
    TempDir tmp;
    write_file(tmp.path / "a.bin", std::string(42, 'x'));
    write_file(tmp.path / "b.bin", std::string(100, 'y'));

    const auto result = scan_directory(tmp.path);
    CHECK(result.entries.size() == 2);
    std::vector<FileSize> sizes;
    for (const auto& e : result.entries) sizes.push_back(e.size);
    std::sort(sizes.begin(), sizes.end());
    CHECK(sizes[0] == 42);
    CHECK(sizes[1] == 100);
}

TEST_CASE("scan of empty directory returns empty result") {
    TempDir tmp;
    const auto result = scan_directory(tmp.path);
    CHECK(result.entries.empty());
    CHECK(result.errors == 0);
}

TEST_CASE("scan of nonexistent path increments errors") {
    const auto result = scan_directory("/definitely/does/not/exist/dupfinder_xyz");
    CHECK(result.entries.empty());
    CHECK(result.errors >= 1);
}
