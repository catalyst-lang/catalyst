// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "llvm/Support/TargetSelect.h"

#include "jit.hpp"

namespace catalyst::compiler {

using namespace llvm;
using namespace llvm::orc;

int64_t run_jit(codegen::state &state) {
	ExitOnError ExitOnErr;

	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
	std::unique_ptr<KaleidoscopeJIT> TheJIT = ExitOnErr(KaleidoscopeJIT::Create());

	state.TheModule->setDataLayout(TheJIT->getDataLayout());

	 ExitOnErr(TheJIT->addModule(
			ThreadSafeModule(std::move(state.TheModule), std::move(state.TheContext))));

	for(const auto& [key, value] : state.runtime->functions)
        TheJIT->define_symbol(key.c_str(), value);

	/*if (auto ret = (*TheJIT)->addModule(
			ThreadSafeModule(std::move(state.TheModule), std::move(state.TheContext)));
	    !ret) {
		codegen::state::report_message(codegen::report_type::error,
		                               "Problem while initializing JIT");
		codegen::state::report_message(codegen::report_type::info,
		                               "Could not load compiled code");
	}*/

	auto ExprSymbol = TheJIT->lookup("main");
	if (!ExprSymbol) {
		auto message = llvm::toString(ExprSymbol.takeError());
		state.report_message(codegen::report_type::error, "Entry function not found");
		state.report_message(codegen::report_type::info, message);
		return -1;
	}

	// Get the symbol's address and cast it to the right type (takes no
	// arguments, returns a double) so we can call it as a native function.
	auto (*FP)() = (int64_t(*)())(*ExprSymbol).getAddress();
	return FP();
}

} // namespace catalyst::compiler
