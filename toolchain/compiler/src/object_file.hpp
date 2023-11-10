// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <string>

#include "compiler.hpp"

namespace catalyst::compiler {

// bool write_object_file(const std::string &filename, const compile_session &result, const std::string &target_triple);
bool write_bundle_file(const std::string &filename, const compile_session &result, const std::string &target_triple);
bool read_bundle_file(const std::string &filename, const compile_session &, const std::string &target_triple);

}