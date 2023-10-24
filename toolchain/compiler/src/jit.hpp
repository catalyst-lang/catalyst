// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4624)
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#pragma warning(pop)
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "runtime.hpp"
#include "codegen/codegen.hpp"

namespace catalyst::compiler {

using namespace llvm;
using namespace llvm::orc;

class KaleidoscopeJIT;

class KaleidoscopeJIT {
  private:
	std::unique_ptr<ExecutionSession> ES;

	DataLayout DL;
	MangleAndInterner Mangle;

	RTDyldObjectLinkingLayer ObjectLayer;
	IRCompileLayer CompileLayer;

	JITDylib &MainJD;

  public:
	KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES, JITTargetMachineBuilder JTMB,
	                DataLayout DL)
		: ES(std::move(ES)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
		  ObjectLayer(*this->ES, []() { return std::make_unique<SectionMemoryManager>(); }),
		  CompileLayer(*this->ES, ObjectLayer,
	                   std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
		  MainJD(this->ES->createBareJITDylib("<main>")) {
		MainJD.addGenerator(
			cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));

		if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
			ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
			ObjectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
		}
	}

	~KaleidoscopeJIT() {
		if (auto Err = ES->endSession())
			ES->reportError(std::move(Err));
	}

	static Expected<std::unique_ptr<KaleidoscopeJIT>> Create() {
		auto EPC = SelfExecutorProcessControl::Create();
		if (!EPC)
			return EPC.takeError();

		auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

		JITTargetMachineBuilder JTMB(ES->getExecutorProcessControl().getTargetTriple());

		auto DL = JTMB.getDefaultDataLayoutForTarget();
		if (!DL)
			return DL.takeError();

		return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(JTMB), std::move(*DL));
	}

	const DataLayout &getDataLayout() const { return DL; }

	JITDylib &getMainJITDylib() { return MainJD; }

	Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
		if (!RT)
			RT = MainJD.getDefaultResourceTracker();
		auto ret = CompileLayer.add(RT, std::move(TSM));

		return ret;
	}

	Expected<ExecutorSymbolDef> lookup(StringRef Name) {
		return ES->lookup({&MainJD}, Mangle(Name.str()));
	}

	void define_symbol(const char* name, llvm::JITTargetAddress addr) {
		SymbolMap M;
		// Register every symbol that can be accessed from the JIT'ed code.
		// Register every symbol that can be accessed from the JIT'ed code.
		M[Mangle(name)] = ExecutorSymbolDef(llvm::orc::ExecutorAddr(addr), JITSymbolFlags());
		cantFail(MainJD.define(absoluteSymbols(M)));
	}
};

template<typename T>
T run_jit(codegen::state &state, const std::string &symbol_name) {
	ExitOnError ExitOnErr;

	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
	std::unique_ptr<KaleidoscopeJIT> TheJIT = ExitOnErr(KaleidoscopeJIT::Create());

	state.TheModule->setDataLayout(TheJIT->getDataLayout());

	 ExitOnErr(TheJIT->addModule(
			ThreadSafeModule(std::move(state.TheModule), std::move(state.TheContext))));

	auto rt = (runtime*)state.target;

	for(const auto& [key, value] : rt->functions)
        TheJIT->define_symbol(key.c_str(), value);

	auto ExprSymbol = TheJIT->lookup(symbol_name);
	if (!ExprSymbol) {
		auto message = llvm::toString(ExprSymbol.takeError());
		state.report_message(codegen::report_type::error, "Entry function not found");
		state.report_message(codegen::report_type::info, message);
		return 0;
	}

	// Get the symbol's address and cast it to the right type (takes no
	// arguments, returns a double) so we can call it as a native function.
	auto (*FP)() = ExprSymbol->getAddress().toPtr<T (*)()>();
	return FP();
}

} // namespace catalyst::compiler
