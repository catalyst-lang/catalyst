// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>
#include <memory>
#include <string>

//#include "../type.hpp"

namespace catalyst::compiler::codegen {

struct type_custom;
struct type_virtual;
struct type;
struct state;

namespace object_type_reference_detail {
    std::string get_name_for_type(catalyst::compiler::codegen::state &state, std::shared_ptr<type_custom> type);
    std::shared_ptr<type> get_type_for_name(catalyst::compiler::codegen::state &state, const std::string& name);
}

template <typename T>
struct object_type_reference {    
    static_assert(std::is_base_of_v<type_custom, T>, "T must be a type_custom");

    object_type_reference(catalyst::compiler::codegen::state &state, const std::string &custom_name) : name(custom_name), state(&state) {}

    object_type_reference(catalyst::compiler::codegen::state &state, std::shared_ptr<type_custom> type) : state(&state) {
        name = object_type_reference_detail::get_name_for_type(state, type);
    }

    static object_type_reference<T> unknown(catalyst::compiler::codegen::state &state) {
        static auto u = object_type_reference(state, "<unknown>");
        return u;
    }

    std::shared_ptr<T> get() const {
        return std::dynamic_pointer_cast<T>(object_type_reference_detail::get_type_for_name(*state, name));
    }

    inline std::shared_ptr<T> operator->() const { return get(); }

    inline std::string getName() const { return name; }

    private:
    std::string name;
    catalyst::compiler::codegen::state *state;
};

}