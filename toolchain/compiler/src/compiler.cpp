//
// Created by basdu on 4-11-2022.
//
#include <iostream>
#include "compiler.hpp"
#include "../../parser/src/parser.hpp"
#include "codegen/codegen.hpp"
#include "codegen/expr.hpp"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include "jit.hpp"

using namespace catalyst;

namespace catalyst::compiler {

compile_result compile(catalyst::ast::translation_unit &tu, options options) {
	auto state = std::make_shared<codegen::state>();
	state->translation_unit = &tu;
	state->options = options;
	state->TheModule = std::make_unique<llvm::Module>(tu.parser_state->filename, *state->TheContext);
	state->FPM = std::make_unique<llvm::legacy::FunctionPassManager>(state->TheModule.get());

	if (options.optimizer_level >= 1) {
		state->FPM->add(llvm::createPromoteMemoryToRegisterPass());
		// Do simple "peephole" optimizations and bit-twiddling optimizations.
		state->FPM->add(llvm::createInstructionCombiningPass());
		// Re-associate expressions.
		state->FPM->add(llvm::createReassociatePass());
	}
	if (options.optimizer_level >= 2) {
		// Eliminate Common SubExpressions.
		state->FPM->add(llvm::createGVNPass());
		// Simplify the control flow graph (deleting unreachable blocks, etc).
		state->FPM->add(llvm::createCFGSimplificationPass());
	}

	state->FPM->doInitialization();

	auto v = codegen::codegen(*state);
	printf("Read module:\n");
	state->TheModule->print(llvm::outs(), nullptr);
	printf("\n");

	compile_result result;
	result.state = std::move(state);
	result.is_successful = true;
	return result;
}

compile_result compile(const std::string &filename, options options) {
	auto ast = parser::parse_filename(filename);
	if (ast.has_value()) {
		return compile(ast.value(), options);
	} else {
		return compile_result::create_failed();
	}
}

uint64_t run(compile_result &result) {
	return run_jit(*std::static_pointer_cast<codegen::state>(result.state));
}

} // namespace catalyst::compiler
