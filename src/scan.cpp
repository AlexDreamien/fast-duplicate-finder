#include "dupfinder/scan.hpp"

#include <system_error>

namespace dupfinder {

namespace fs = std::filesystem;

ScanResult scan_directory(const fs::path& root, FileSize min_size) {
    ScanResult result;
    std::error_code ec;

    auto opts = fs::directory_options::skip_permission_denied;
    fs::recursive_directory_iterator it(root, opts, ec);
    if (ec) {
        result.errors = 1;
        return result;
    }

    const fs::recursive_directory_iterator end;
    while (it != end) {
        auto status = it->status(ec);
        if (ec) {
            ++result.errors;
            ec.clear();
            it.increment(ec);
            ec.clear();
            continue;
        }
        if (fs::is_regular_file(status)) {
            const auto size = it->file_size(ec);
            if (ec) {
                ++result.errors;
                ec.clear();
            } else if (size >= min_size) {
                result.entries.push_back({it->path(), size});
            }
        }
        it.increment(ec);
        if (ec) {
            ++result.errors;
            ec.clear();
        }
    }

    return result;
}

}  // namespace dupfinder
