#pragma once

#include <string>
#include <sstream>

#include "../common/version.hpp"

namespace catalyst::parser {
    
using char_type = char8_t;

constexpr struct {
    int major = 0, minor = 0, patch = 1;

    std::string string() const {
        std::stringstream ss;
        ss << major << '.' << minor << '.' << patch;
        return ss.str();
    }
} version;

}
