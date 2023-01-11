//
// Created by basdu on 4-11-2022.
//
#include <iostream>
#include "compiler.hpp"
#include "../../parser/src/parser.hpp"
#include "codegen/codegen.hpp"
#include "codegen/expr.hpp"
#include "codegen/type.hpp"

#pragma warning( push )
#pragma warning( disable : 4244 )
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/IPO/ArgumentPromotion.h"
#include "llvm/Transforms/Vectorize.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/Attributor.h"
#include "llvm/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Transforms/IPO/InferFunctionAttrs.h"
#include "llvm/Analysis/Passes.h"
#include "jit.hpp"
#pragma warning( pop )

using namespace catalyst;

namespace catalyst::compiler {

compile_result compile(catalyst::ast::translation_unit &tu, options options) {
	auto state = std::make_shared<codegen::state>();
	state->translation_unit = &tu;
	state->options = options;
	state->TheModule = std::make_unique<llvm::Module>(tu.parser_state->filename, *state->TheContext);
	state->FPM = std::make_unique<llvm::legacy::FunctionPassManager>(state->TheModule.get());

	state->runtime->register_symbols();

	options.optimizer_level = 2;

	if (options.optimizer_level >= 1) {
		// Standard mem2reg pass to construct SSA form from alloca's and stores.
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

		state->FPM->add(llvm::createMemCpyOptPass());
		state->FPM->add(llvm::createConstantHoistingPass());
		state->FPM->add(llvm::createDeadCodeEliminationPass());
		//state->FPM->add(llvm::createDeadArgEliminationPass());
		state->FPM->add(llvm::createDeadStoreEliminationPass());
		//state->FPM->add(llvm::createInferFunctionAttrsLegacyPass());
		state->FPM->add(llvm::createIndVarSimplifyPass());

		state->FPM->add(llvm::createPartiallyInlineLibCallsPass());
		//state->FPM->add(llvm::createAlwaysInlinerLegacyPass());
		//state->FPM->add(llvm::createVectorCombinePass());
		//state->FPM->add(llvm::createLoadStoreVectorizerPass());
		//state->FPM->add(llvm::createSLPVectorizerPass());
		//state->FPM->add(llvm::createLoopVectorizePass());
	}


	state->FPM->doInitialization();

	codegen::codegen(*state);

	compile_result result;
	result.state = std::move(state);
	result.is_successful = true;
	return result;
}

compile_result compile_string(const std::string &string, options options) {
	auto ast = parser::parse_string(string);
	if (ast.has_value()) {
		return compile(ast.value(), options);
	} else {
		return compile_result::create_failed();
	}
}

compile_result compile_file(const std::string &filename, options options) {
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

void compiler_debug_print(compile_result &result) {
	auto state = std::static_pointer_cast<codegen::state>(result.state);
	printf("Read module:\n");
	state->TheModule->print(llvm::outs(), nullptr);
	printf("\n");
}

} // namespace catalyst::compiler
