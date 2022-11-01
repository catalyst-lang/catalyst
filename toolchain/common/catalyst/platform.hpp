#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define CATALYST_PLATFORM_WINDOWS 1
#elif defined(unix) || defined(__unix) || defined(__unix__)
#define CATALYST_PLATFORM_UNIX 1
#elif defined(__APPLE__) || defined(__MACH__)
#define CATALYST_PLATFORM_MACOS 1
#else
#define CATALYST_PLATFORM_UNKNOWN 1
#endif

#ifdef _POSIX_VERSION
#define CATALYST_PLATFORM_POSIX 1
#else
#define CATALYST_PLATFORM_POSIX 0
#endif

namespace catalyst {

enum class Platform { 
    Windows,
    MacOS,
    Unix,
    Unknown
 };

#if defined(_WIN32) || defined(_WIN64)
constexpr Platform current_platform = Platform::Windows;
#elif defined(unix) || defined(__unix) || defined(__unix__)
constexpr Platform current_platform = Platform::Unix;
#elif defined(__APPLE__) || defined(__MACH__)
constexpr Platform current_platform = Platform::MacOS;
#else
constexpr Platform current_platform = Platform::Unknown;
#endif

constexpr bool is_posix_compliant(Platform platform) {
    return platform == Platform::Unix || platform == Platform::MacOS;
}

constexpr bool is_current_posix_compliant() {
    return CATALYST_PLATFORM_POSIX;
}

}
