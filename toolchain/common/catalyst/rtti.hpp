#pragma once

#include <memory>

namespace catalyst {
    
template <typename T, typename I> inline bool isa(const std::shared_ptr<I> &ptr) {
    return dynamic_cast<T*>(ptr.get()) != nullptr;
}

template <typename T, typename I> inline bool isa(const I &obj) {
    return dynamic_cast<const T*>(&obj) != nullptr;
}

template <typename T> inline bool isa(const void* ptr) {
    return dynamic_cast<T*>(ptr) != nullptr;
}

}
