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
#include "jit.hpp"

using namespace catalyst;

namespace catalyst::compiler {

bool compile(catalyst::ast::translation_unit &tu, options options) {
	codegen::state state;
	state.translation_unit = &tu;
	state.options = options;
	state.TheModule = std::make_unique<llvm::Module>(tu.parser_state->filename, state.TheContext);
	state.FPM = std::make_unique<llvm::legacy::FunctionPassManager>(state.TheModule.get());

	switch (options.optimizer_level) {
	case 2:
		// Do simple "peephole" optimizations and bit-twiddling optimizations.
		state.FPM->add(llvm::createInstructionCombiningPass());
		// Re-associate expressions.
		state.FPM->add(llvm::createReassociatePass());
	case 1:
		// Eliminate Common SubExpressions.
		state.FPM->add(llvm::createGVNPass());
		// Simplify the control flow graph (deleting unreachable blocks, etc).
		state.FPM->add(llvm::createCFGSimplificationPass());
	case 0:
	default:
		break;
	}

	state.FPM->doInitialization();

	auto v = codegen::codegen(state);
	printf("Read module:\n");
	state.TheModule->print(llvm::outs(), nullptr);
	printf("\n");

	bool run_jit = true;
	if (run_jit) {
		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
		llvm::InitializeNativeTargetAsmParser();
		auto TheJIT = KaleidoscopeJIT::Create();
		if (TheJIT.takeError()) {
			state.report_error("Problem while initializing JIT");
			return false;
		}
		state.TheModule->setDataLayout((*TheJIT)->getDataLayout());

	}

	return false;
}

bool compile(const std::string &filename, options options) {
	auto ast = parser::parse_filename(filename);
	if (ast.has_value()) {
		return compile(ast.value(), options);
	} else {
		return false;
	}
}

} // namespace catalyst::compiler