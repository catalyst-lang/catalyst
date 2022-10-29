#pragma once

#include <string>
#include <sstream>

namespace catalyst {
    
constexpr struct {
    int major = 0, minor = 0, patch = 1;

    std::string string() const {
        std::stringstream ss;
        ss << major << '.' << minor << '.' << patch;
        return ss.str();
    }
} version;

}
