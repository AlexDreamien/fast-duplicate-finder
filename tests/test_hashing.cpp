#include "doctest.h"

#include "dupfinder/hashing.hpp"
#include "test_helpers.hpp"

using namespace dupfinder;
using namespace dupfinder::test;

TEST_CASE("hash_file is deterministic") {
    TempDir tmp;
    write_file(tmp.path / "a.bin", "hello world");
    const auto h1 = hash_file(tmp.path / "a.bin");
    const auto h2 = hash_file(tmp.path / "a.bin");
    CHECK(h1 == h2);
}

TEST_CASE("identical contents produce identical hashes") {
    TempDir tmp;
    write_file(tmp.path / "a.bin", "identical content here");
    write_file(tmp.path / "b.bin", "identical content here");
    CHECK(hash_file(tmp.path / "a.bin") == hash_file(tmp.path / "b.bin"));
}

TEST_CASE("different contents produce different hashes") {
    TempDir tmp;
    write_file(tmp.path / "a.bin", "content one");
    write_file(tmp.path / "b.bin", "content two");
    CHECK(hash_file(tmp.path / "a.bin") != hash_file(tmp.path / "b.bin"));
}

TEST_CASE("empty file hashes successfully") {
    TempDir tmp;
    write_file(tmp.path / "empty.bin", "");
    const auto h1 = hash_file(tmp.path / "empty.bin");
    write_file(tmp.path / "empty2.bin", "");
    const auto h2 = hash_file(tmp.path / "empty2.bin");
    CHECK(h1 == h2);
}

TEST_CASE("large file hashes consistently") {
    TempDir tmp;
    std::string content(256 * 1024, 'A');  // 256 KiB, exceeds single read buffer
    for (std::size_t i = 0; i < content.size(); ++i) {
        content[i] = static_cast<char>(i & 0xFF);
    }
    write_file(tmp.path / "big.bin", content);
    write_file(tmp.path / "big2.bin", content);
    CHECK(hash_file(tmp.path / "big.bin") == hash_file(tmp.path / "big2.bin"));
}

TEST_CASE("hash_file throws on missing file") {
    CHECK_THROWS_AS(hash_file("does_not_exist__"), std::runtime_error);
}

TEST_CASE("partial hash equals full hash for files smaller than the prefix") {
    TempDir tmp;
    const std::string small_content = "tiny";
    write_file(tmp.path / "tiny.bin", small_content);
    const auto full = hash_file(tmp.path / "tiny.bin");
    const auto partial = hash_file_prefix(tmp.path / "tiny.bin", 4096);
    CHECK(full == partial);
}

TEST_CASE("partial hash distinguishes files with different prefixes") {
    TempDir tmp;
    write_file(tmp.path / "a.bin", "different start here..." + std::string(8192, 'x'));
    write_file(tmp.path / "b.bin", "other start here......." + std::string(8192, 'x'));
    CHECK(hash_file_prefix(tmp.path / "a.bin", 16) !=
          hash_file_prefix(tmp.path / "b.bin", 16));
}
