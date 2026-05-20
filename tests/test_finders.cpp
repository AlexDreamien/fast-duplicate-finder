#include "doctest.h"

#include <algorithm>
#include <set>
#include <string>

#include "dupfinder/finder.hpp"
#include "dupfinder/scan.hpp"
#include "test_helpers.hpp"

using namespace dupfinder;
using namespace dupfinder::test;

namespace {

// Returns the set of duplicate paths reported, sorted, regardless of grouping.
std::set<std::set<std::string>> normalise(const std::vector<DuplicateGroup>& groups) {
    std::set<std::set<std::string>> out;
    for (const auto& g : groups) {
        std::set<std::string> paths;
        for (const auto& p : g.paths) paths.insert(p.string());
        out.insert(std::move(paths));
    }
    return out;
}

struct Fixture {
    TempDir tmp;
    std::vector<FileEntry> entries;

    Fixture() {
        // Two duplicate pairs and three uniques.
        write_file(tmp.path / "dup_a_1.bin", "same content one");
        write_file(tmp.path / "sub" / "dup_a_2.bin", "same content one");
        write_file(tmp.path / "dup_b_1.bin", std::string(8192, 'X'));
        write_file(tmp.path / "deep" / "nested" / "dup_b_2.bin", std::string(8192, 'X'));
        write_file(tmp.path / "unique_small.bin", "alpha");
        write_file(tmp.path / "unique_med.bin", std::string(8192, 'Y'));
        write_file(tmp.path / "unique_big.bin", std::string(16384, 'Z'));

        entries = scan_directory(tmp.path).entries;
    }
};

}  // namespace

TEST_CASE_FIXTURE(Fixture, "v0 finds exactly the two duplicate pairs") {
    const auto groups = find_duplicates_v0(entries);
    REQUIRE(groups.size() == 2);
    for (const auto& g : groups) {
        CHECK(g.paths.size() == 2);
    }
}

TEST_CASE_FIXTURE(Fixture, "all four finders report the same duplicate groups") {
    const auto v0 = normalise(find_duplicates_v0(entries));
    const auto v1 = normalise(find_duplicates_v1(entries));
    const auto v2 = normalise(find_duplicates_v2(entries));
    const auto v3 = normalise(find_duplicates_v3(entries, /*threads=*/4));

    CHECK(v0 == v1);
    CHECK(v0 == v2);
    CHECK(v0 == v3);
}

TEST_CASE("finders handle empty input") {
    const std::vector<FileEntry> empty;
    CHECK(find_duplicates_v0(empty).empty());
    CHECK(find_duplicates_v1(empty).empty());
    CHECK(find_duplicates_v2(empty).empty());
    CHECK(find_duplicates_v3(empty).empty());
}

TEST_CASE("v1 returns no groups when every file is unique") {
    TempDir tmp;
    write_file(tmp.path / "a.bin", "alpha");
    write_file(tmp.path / "b.bin", "beta");
    write_file(tmp.path / "c.bin", "gamma");

    const auto entries = scan_directory(tmp.path).entries;
    CHECK(find_duplicates_v0(entries).empty());
    CHECK(find_duplicates_v1(entries).empty());
    CHECK(find_duplicates_v2(entries).empty());
    CHECK(find_duplicates_v3(entries).empty());
}

TEST_CASE("empty files are reported as duplicates of each other") {
    TempDir tmp;
    write_file(tmp.path / "a.empty", "");
    write_file(tmp.path / "b.empty", "");
    write_file(tmp.path / "c.empty", "");
    const auto entries = scan_directory(tmp.path).entries;

    for (auto fn : {&find_duplicates_v0, &find_duplicates_v1, &find_duplicates_v2}) {
        const auto groups = fn(entries);
        REQUIRE(groups.size() == 1);
        CHECK(groups[0].paths.size() == 3);
        CHECK(groups[0].size == 0);
    }
    const auto v3 = find_duplicates_v3(entries);
    REQUIRE(v3.size() == 1);
    CHECK(v3[0].paths.size() == 3);
}

TEST_CASE("v2 still detects duplicates when files share the first 4 KiB but differ later") {
    TempDir tmp;
    std::string prefix(PARTIAL_HASH_BYTES + 10, 'P');
    write_file(tmp.path / "a.bin", prefix + "tail_alpha" + std::string(2048, 'A'));
    write_file(tmp.path / "b.bin", prefix + "tail_beta_" + std::string(2048, 'A'));
    // Same content in c/d so that v2's partial-hash filter would let them through.
    write_file(tmp.path / "c.bin", prefix + "tail_gamma" + std::string(2048, 'G'));
    write_file(tmp.path / "d.bin", prefix + "tail_gamma" + std::string(2048, 'G'));

    const auto entries = scan_directory(tmp.path).entries;
    const auto groups = find_duplicates_v2(entries);
    REQUIRE(groups.size() == 1);
    CHECK(groups[0].paths.size() == 2);
}
