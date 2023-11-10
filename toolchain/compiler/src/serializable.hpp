// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <iostream>
#include <fstream>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <functional>

namespace catalyst::compiler::serializable {

class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void serialize(std::ostream& out) const = 0;

    template<class T>
    static void write_binary(std::ostream& out, const T& value) {
        out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

    template<typename T>
    static void read_binary(std::istream& in, T& value) {
        in.read(reinterpret_cast<char*>(&value), sizeof(T));
    }

    static bool read_boolean(std::istream& in) {
        bool value;
        read_binary(in, value);
        return value;
    }

    template<typename T>
    static void write_vector(std::ostream& out, const std::vector<T>& vec, std::function<void(std::ostream&, const T&)> element_fn) {
        write_binary(out, (int32_t)vec.size());
        for (const auto& elem : vec) {
            element_fn(out, elem);
        }
    }

    template<typename T>
    static void write_unordered_set(std::ostream& out, const std::unordered_set<T>& set, std::function<void(std::ostream&, const T&)> element_fn) {
        write_binary(out, (int32_t)set.size());
        for (const auto& elem : set) {
            element_fn(out, elem);
        }
    }

    template<typename T>
    static void read_unordered_set(std::istream& in, std::unordered_set<T>& set, std::function<T(std::istream&)> element_fn) {
        int32_t size;
        read_binary(in, size);
        for (size_t i = 0; i < size; i++) {
            set.insert(element_fn(in));
        }
    }

    template<typename T>
    static std::vector<T> read_vector(std::istream& in, std::function<T(std::istream&)> element_fn) {
        int32_t size;
        read_binary(in, size);
        std::vector<T> vec;
        vec.reserve(size);
        for (size_t i = 0; i < size; i++) {
            vec.push_back(element_fn(in));
        }
        return vec;
    }

    static std::string read_string(std::istream& in) {
        std::string str;
        std::getline(in, str, '\0');
        return str;
    }
};

} // namespace catalyst::compiler
