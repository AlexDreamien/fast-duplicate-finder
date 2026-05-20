#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "dupfinder/types.hpp"

namespace dupfinder {

enum class OutputFormat { Text, Json };

OutputFormat parse_format(const std::string& s);

void write_groups(std::ostream& out,
                  const std::vector<DuplicateGroup>& groups,
                  OutputFormat fmt);

// Human-readable size: "1.23 MB", "456 B".
std::string format_size(FileSize bytes);

}  // namespace dupfinder
