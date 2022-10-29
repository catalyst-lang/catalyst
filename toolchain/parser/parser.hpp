#pragma once

#include <string>
#include <format>

#include "../common/version.hpp"

namespace catalyst::parser {
    
using char_type = char8_t;

constexpr struct {
    static constexpr int major = 0, minor = 0, patch = 1;

    static std::string string() {
        return std::format("{}.{}.{}", version.major, version.minor, version.patch);
    }
} version;

}
