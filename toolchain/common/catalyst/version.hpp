#pragma once

#include <string>
#include <sstream>

#define _CATALYST_STRINGIFY(x) #x
#define CATALYST_TOSTRING(x) _CATALYST_STRINGIFY(x)
#define CATALYST_AT __FILE__ ":" CATALYST_TOSTRING(__LINE__)

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
