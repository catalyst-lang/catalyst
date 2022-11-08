//
// Created by basdu on 4-11-2022.
//
#include <iostream>
#include "compiler.hpp"
#include "../../parser/src/parser.hpp"
#include "codegen/codegen.hpp"
#include "codegen/expr.hpp"

#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

using namespace catalyst;

namespace catalyst::compiler {

bool compile(catalyst::ast::translation_unit &tu) {
	codegen::state state;
	state.translation_unit = &tu;
	state.TheModule = std::make_unique<llvm::Module>(tu.parser_state->filename, state.TheContext);
	state.FPM = std::make_unique<llvm::legacy::FunctionPassManager>(state.TheModule.get());

	// Do simple "peephole" optimizations and bit-twiddling optimizations.
	state.FPM->add(llvm::createInstructionCombiningPass());
	// Re-associate expressions.
	state.FPM->add(llvm::createReassociatePass());
	// Eliminate Common SubExpressions.
	state.FPM->add(llvm::createGVNPass());
	// Simplify the control flow graph (deleting unreachable blocks, etc).
	state.FPM->add(llvm::createCFGSimplificationPass());

	state.FPM->doInitialization();

	auto v = codegen::codegen(state);
	printf("Read module:\n");
	state.TheModule->print(llvm::outs(), nullptr);
	printf("\n");

	return false;
}

bool compile(const std::string &filename) {
	auto ast = parser::parse_filename(filename);
	if (ast.has_value()) {
		return compile(ast.value());
	} else {
		return false;
	}
}

} // namespace catalyst::compiler