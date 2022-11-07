// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once
#pragma warning(disable : 4624)

#include <memory>
#include <map>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

namespace catalyst::compiler::codegen {

struct state {
	llvm::LLVMContext TheContext;
	llvm::IRBuilder<> Builder;
	std::unique_ptr<llvm::Module> TheModule;
	std::map<std::string, llvm::Value *> NamedValues;

	state(): Builder(TheContext) {

	}
};

} // namespace catalyst::compiler::codegen
