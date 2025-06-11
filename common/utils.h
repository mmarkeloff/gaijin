#pragma once

#include <vector>
#include <string_view>

std::vector<std::string_view> split(std::string_view s, const char *delim) noexcept;
