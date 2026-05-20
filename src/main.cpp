#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "dupfinder/finder.hpp"
#include "dupfinder/format.hpp"
#include "dupfinder/scan.hpp"

namespace fs = std::filesystem;
using namespace dupfinder;

namespace {

constexpr const char* USAGE =
    "Usage: dupfinder <path> [options]\n"
    "\n"
    "Options:\n"
    "  --algorithm {v0,v1,v2,v3}  Which finder algorithm to use (default: v3).\n"
    "  --min-size BYTES            Skip files smaller than this many bytes.\n"
    "  --format {text,json}        Output format (default: text).\n"
    "  --threads N                 Worker thread count for v3 (default: hw concurrency).\n"
    "  --timing                    Print scan and search timings to stderr.\n"
    "  -h, --help                  Show this help and exit.\n";

struct Args {
    fs::path root;
    std::string algorithm = "v3";
    FileSize min_size = 0;
    OutputFormat format = OutputFormat::Text;
    std::size_t threads = 0;
    bool timing = false;
};

std::string_view require_value(int argc, char** argv, int& i) {
    if (i + 1 >= argc) {
        throw std::invalid_argument(std::string("Missing value for ") + argv[i]);
    }
    return argv[++i];
}

Args parse_args(int argc, char** argv) {
    Args args;
    bool root_set = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view a = argv[i];
        if (a == "-h" || a == "--help") {
            std::cout << USAGE;
            std::exit(0);
        } else if (a == "--algorithm") {
            args.algorithm = std::string(require_value(argc, argv, i));
        } else if (a == "--min-size") {
            args.min_size = std::stoull(std::string(require_value(argc, argv, i)));
        } else if (a == "--format") {
            args.format = parse_format(std::string(require_value(argc, argv, i)));
        } else if (a == "--threads") {
            args.threads = std::stoull(std::string(require_value(argc, argv, i)));
        } else if (a == "--timing") {
            args.timing = true;
        } else if (!a.empty() && a[0] == '-') {
            throw std::invalid_argument("Unknown option: " + std::string(a));
        } else if (!root_set) {
            args.root = std::string(a);
            root_set = true;
        } else {
            throw std::invalid_argument("Unexpected positional argument: " + std::string(a));
        }
    }
    if (!root_set) {
        throw std::invalid_argument("Path is required. Run with --help for usage.");
    }
    return args;
}

std::vector<DuplicateGroup> dispatch(const std::string& algorithm,
                                     const std::vector<FileEntry>& entries,
                                     std::size_t threads) {
    if (algorithm == "v0") return find_duplicates_v0(entries);
    if (algorithm == "v1") return find_duplicates_v1(entries);
    if (algorithm == "v2") return find_duplicates_v2(entries);
    if (algorithm == "v3") return find_duplicates_v3(entries, threads);
    throw std::invalid_argument("Unknown algorithm: " + algorithm);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Args args = parse_args(argc, argv);

        using clock = std::chrono::steady_clock;
        auto scan_start = clock::now();
        ScanResult scanned = scan_directory(args.root, args.min_size);
        auto scan_done = clock::now();

        if (args.timing) {
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                scan_done - scan_start)
                                .count();
            std::cerr << "scan: " << scanned.entries.size() << " files in " << ms << " ms";
            if (scanned.errors) std::cerr << " (" << scanned.errors << " skipped)";
            std::cerr << '\n';
        }

        auto search_start = clock::now();
        auto groups = dispatch(args.algorithm, scanned.entries, args.threads);
        auto search_done = clock::now();

        if (args.timing) {
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                search_done - search_start)
                                .count();
            std::cerr << "search (" << args.algorithm << "): " << ms << " ms\n";
        }

        write_groups(std::cout, groups, args.format);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
