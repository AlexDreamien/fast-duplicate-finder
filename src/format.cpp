#include "dupfinder/format.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace dupfinder {

OutputFormat parse_format(const std::string& s) {
    std::string lower;
    lower.reserve(s.size());
    for (char c : s) lower.push_back(static_cast<char>(std::tolower(c)));
    if (lower == "text") return OutputFormat::Text;
    if (lower == "json") return OutputFormat::Json;
    throw std::invalid_argument("Unknown output format: " + s);
}

std::string format_size(FileSize bytes) {
    constexpr double UNIT = 1024.0;
    static const char* const units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    double value = static_cast<double>(bytes);
    std::size_t i = 0;
    while (value >= UNIT && i + 1 < sizeof(units) / sizeof(units[0])) {
        value /= UNIT;
        ++i;
    }
    std::ostringstream out;
    if (i == 0) {
        out << bytes << ' ' << units[0];
    } else {
        out << std::fixed << std::setprecision(2) << value << ' ' << units[i];
    }
    return out.str();
}

namespace {

void write_text(std::ostream& out, const std::vector<DuplicateGroup>& groups) {
    if (groups.empty()) {
        out << "No duplicates found.\n";
        return;
    }
    FileSize total_wasted = 0;
    out << groups.size() << " duplicate group(s)\n\n";
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const auto& g = groups[i];
        const FileSize wasted = g.size * (g.paths.size() - 1);
        total_wasted += wasted;
        out << "Group " << (i + 1) << ": " << g.paths.size()
            << " files, each " << format_size(g.size)
            << " (wasted " << format_size(wasted) << ")\n";
        for (const auto& p : g.paths) {
            out << "  " << p.string() << '\n';
        }
        out << '\n';
    }
    out << "Total reclaimable: " << format_size(total_wasted) << '\n';
}

void write_json_string(std::ostream& out, const std::string& s) {
    out << '"';
    for (char c : s) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c) << std::dec << std::setfill(' ');
                } else {
                    out << c;
                }
        }
    }
    out << '"';
}

void write_json(std::ostream& out, const std::vector<DuplicateGroup>& groups) {
    out << "[";
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const auto& g = groups[i];
        if (i > 0) out << ",";
        out << "{\"size\":" << g.size << ",\"hash\":" << g.hash << ",\"paths\":[";
        for (std::size_t j = 0; j < g.paths.size(); ++j) {
            if (j > 0) out << ",";
            write_json_string(out, g.paths[j].string());
        }
        out << "]}";
    }
    out << "]\n";
}

}  // namespace

void write_groups(std::ostream& out,
                  const std::vector<DuplicateGroup>& groups,
                  OutputFormat fmt) {
    switch (fmt) {
        case OutputFormat::Text: write_text(out, groups); break;
        case OutputFormat::Json: write_json(out, groups); break;
    }
}

}  // namespace dupfinder
