#pragma once

#include <string>
#include <format>

namespace catalyst {
    
constexpr struct {
    static constexpr int major = 0, minor = 0, patch = 1;

    static std::string string() {
        return std::format("{}.{}.{}", version.major, version.minor, version.patch);
    }
} version;

}
