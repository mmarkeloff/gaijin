#include <cstring>

#include "common/utils.h"

std::vector<std::string_view> split(std::string_view s, const char *delim) noexcept
{
    std::vector<std::string_view> tokens;
    auto pos = s.npos;
    while (s.npos != (pos = s.find(delim)))
    {
        auto token = s.substr(0, pos);
        if (!token.empty())
        {
            tokens.push_back(token);
        }
        s = s.substr(pos + strlen(delim));
    }
    if (!s.empty())
    {
        tokens.push_back(s);
    }
    return tokens;
}