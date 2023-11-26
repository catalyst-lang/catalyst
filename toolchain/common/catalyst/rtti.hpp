#pragma once

#include <memory>

namespace catalyst {
    
template <typename T, typename I> bool isa(const std::shared_ptr<I> &ptr) {
    return dynamic_cast<T*>(ptr.get()) != nullptr;
}

template <typename T, typename I> bool isa(I *obj) {
    return dynamic_cast<T*>(obj) != nullptr;
}

template <typename T, typename I> bool isa(const I &obj) {
    return dynamic_cast<const T*>(&obj) != nullptr;
}

template <typename T> bool isa(const void* ptr) {
    return dynamic_cast<T*>(ptr) != nullptr;
}

}
