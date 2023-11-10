// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <iostream>
#include "compiler.hpp"
#include "../../parser/src/parser.hpp"
#include "codegen/codegen.hpp"
#include "codegen/expr.hpp"
#include "codegen/type.hpp"
#include "catalyst/rtti.hpp"
#include "codegen/metadata.hpp"
#include "object_file.hpp"

#pragma warning( push )
#pragma warning( disable : 4244 )
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/TargetParser/Host.h"
#include "jit.hpp"
#pragma warning( pop )

using namespace catalyst;

namespace catalyst::compiler {

void initialize_state(compile_session& session, options options) {
	auto state = std::make_shared<codegen::state>();
	session.state = state;
	//auto module_name = tu.parser_state->filename;
	std::string module_name = "module";
	state->TheModule = std::make_unique<llvm::Module>(module_name, *state->TheContext);
	state->TheFPM = std::make_unique<FunctionPassManager>();
	state->TheFAM = std::make_unique<FunctionAnalysisManager>();
	state->TheMAM = std::make_unique<ModuleAnalysisManager>();
	state->ThePIC = std::make_unique<PassInstrumentationCallbacks>();
	state->TheSI = std::make_unique<StandardInstrumentations>(*state->TheContext, /*DebugLogging*/ false);
	state->TheSI->registerCallbacks(*state->ThePIC, state->TheMAM.get());

	state->target->register_symbols();

	options.optimizer_level = 2;

	if (options.optimizer_level >= 1) {
		// // Standard mem2reg pass to construct SSA form from alloca's and stores.
		// state->FPM->add(llvm::createPromoteMemoryToRegisterPass());
		// // Do simple "peephole" optimizations and bit-twiddling optimizations.
		// state->FPM->add(llvm::createInstructionCombiningPass());
		// // Re-associate expressions.
		// state->FPM->add(llvm::createReassociatePass());

		// Add transform passes.
		// Do simple "peephole" optimizations and bit-twiddling optzns.
		state->TheFPM->addPass(llvm::InstCombinePass());
		// Reassociate expressions.
		state->TheFPM->addPass(llvm::ReassociatePass());
		// Eliminate Common SubExpressions.
		state->TheFPM->addPass(llvm::GVNPass());
		// Simplify the control flow graph (deleting unreachable blocks, etc).
		state->TheFPM->addPass(llvm::SimplifyCFGPass());

		// Register analysis passes used in these transform passes.
		state->TheFAM->registerPass([&] { return llvm::AAManager(); });
		state->TheFAM->registerPass([&] { return llvm::AssumptionAnalysis(); });
		state->TheFAM->registerPass([&] { return llvm::DominatorTreeAnalysis(); });
		state->TheFAM->registerPass([&] { return llvm::LoopAnalysis(); });
		state->TheFAM->registerPass([&] { return llvm::MemoryDependenceAnalysis(); });
		state->TheFAM->registerPass([&] { return llvm::MemorySSAAnalysis(); });
		state->TheFAM->registerPass([&] { return llvm::OptimizationRemarkEmitterAnalysis(); });
		state->TheFAM->registerPass([&] {
			return llvm::OuterAnalysisManagerProxy<ModuleAnalysisManager, Function>(*state->TheMAM);
		});
		state->TheFAM->registerPass(
			[&] { return llvm::PassInstrumentationAnalysis(state->ThePIC.get()); });
		state->TheFAM->registerPass([&] { return llvm::TargetIRAnalysis(); });
		state->TheFAM->registerPass([&] { return llvm::TargetLibraryAnalysis(); });

		state->TheMAM->registerPass([&] { return llvm::ProfileSummaryAnalysis(); });
	}
	if (options.optimizer_level >= 2) {
		// // Eliminate Common SubExpressions.
		// state->FPM->add(llvm::createGVNPass());
		// state->FPM->add(llvm::createSinkingPass());
		// state->FPM->add(llvm::createGVNHoistPass());
		// state->FPM->add(llvm::createGVNSinkPass());
		// // Simplify the control flow graph (deleting unreachable blocks, etc).
		// state->FPM->add(llvm::createCFGSimplificationPass());
		// state->FPM->add(llvm::createSROAPass());

		// state->FPM->add(llvm::createIndVarSimplifyPass());
		// state->FPM->add(llvm::createAggressiveInstCombinerPass());
		// state->FPM->add(llvm::createPartiallyInlineLibCallsPass());
		// //state->FPM->add(llvm::createAggressiveDCEPass());
		// state->FPM->add(llvm::createMemCpyOptPass());
		// state->FPM->add(llvm::createConstantHoistingPass());
		
		// state->FPM->add(llvm::createDeadCodeEliminationPass());
		// //state->FPM->add(llvm::createDeadArgEliminationPass());
		// state->FPM->add(llvm::createDeadStoreEliminationPass());
		// //state->FPM->add(llvm::createInferFunctionAttrsLegacyPass());

		// //state->FPM->add(llvm::createAlwaysInlinerLegacyPass());
		// //state->FPM->add(llvm::createVectorCombinePass());
		// //state->FPM->add(llvm::createLoadStoreVectorizerPass());
		// //state->FPM->add(llvm::createSLPVectorizerPass());
		// //state->FPM->add(llvm::createLoopVectorizePass());
	}
}

bool compile(compile_session& session, catalyst::ast::translation_unit &tu, options options) {
	if (session.state == nullptr) {
		initialize_state(session, options);
	}

	auto state = std::static_pointer_cast<codegen::state>(session.state);
	state->translation_unit = &tu;
	state->options = options;

	codegen::codegen(*state);

	session.is_successful = state->num_errors == 0;

	std::string main = "main";
	if (!state->global_namespace.empty())
		main = state->global_namespace + "." + main;

	if (session.is_successful && state->symbol_table.contains(main)) {
		auto sym_type = state->symbol_table[main].type;
		if (catalyst::isa<codegen::type_function>(sym_type.get())) {
			auto sym_fun = (codegen::type_function*)sym_type.get();
			session.result_type_name = sym_fun->return_type->get_fqn();
			session.is_runnable = true;
		} else {
			session.is_runnable = false;
		}
	}
	
	return state->num_errors == 0;
}

bool compile_string(compile_session& session, const std::string &string, options options) {
	auto ast = parser::parse_string(string);
	if (ast.has_value()) {
		return compile(session, ast.value(), options);
	} else {
		session.is_successful = false;
		session.is_runnable = false;
		return false;
	}
}

compile_session compile_string(const std::string &string, options options) {
	auto ast = parser::parse_string(string);
	if (ast.has_value()) {
		compile_session session;
		if (compile(session, ast.value(), options)) {
			return session;
		} else {
			session.is_successful = false;
			return session;
		}
	} else {
		return compile_session::create_failed();
	}
}

bool compile_file(compile_session& session, const std::string &filename, options options) {
	auto ast = parser::parse_filename(filename);
	if (ast.has_value()) {
		return compile(session, ast.value(), options);
	} else {
		session.is_successful = false;
		session.is_runnable = false;
		return false;
	}
}

void compiler_import_bundle(compile_session& session, const std::string &filename, options options) {
	if (session.state == nullptr) {
		initialize_state(session, options);
	}

	session.is_successful = read_bundle_file(filename, session, get_default_target_triple());
}

void compiler_debug_print(compile_session &result) {
	auto state = std::static_pointer_cast<codegen::state>(result.state);
	if (state != nullptr && state->TheModule) {
		printf("Read module:\n");
		state->TheModule->print(llvm::outs(), nullptr);
		printf("\n");
	}
}

codegen::state &get_state(const compile_session &result) {
	return *std::static_pointer_cast<codegen::state>(result.state);
}

std::string get_default_target_triple() {
	return llvm::sys::getDefaultTargetTriple();
}

template<typename T>
T run(const compile_session &result) {
	auto &state = get_state(result);
	std::string main = "main";

	if (!state.global_namespace.empty())
		main = state.global_namespace + "." + main;
	return run_jit<T>(state, main);
}

bool create_meta(const compile_session &result, std::ostream& out) {
	codegen::create_meta(get_state(result), out);
	return true;
}

template int8_t run(const compile_session &);
template int16_t run(const compile_session &);
template int32_t run(const compile_session &);
template int64_t run(const compile_session &);
template uint8_t run(const compile_session &);
template uint16_t run(const compile_session &);
template uint32_t run(const compile_session &);
template uint64_t run(const compile_session &);
template void* run(const compile_session &);
template float run(const compile_session &);
template double run(const compile_session &);
template bool run(const compile_session &);

} // namespace catalyst::compiler
