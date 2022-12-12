// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

#pragma warning( push )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4624 )
#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#pragma warning( pop )
#include <memory>

#include "codegen/codegen.hpp"

namespace catalyst::compiler {

using namespace llvm;
using namespace llvm::orc;

int64_t run_jit(codegen::state &state);

//class KaleidoscopeASTLayer;
class KaleidoscopeJIT;

// class KaleidoscopeASTMaterializationUnit : public MaterializationUnit {
//   public:
// 	KaleidoscopeASTMaterializationUnit(KaleidoscopeASTLayer &L, ast::decl_fn *F);

// 	StringRef getName() const override { return "KaleidoscopeASTMaterializationUnit"; }

// 	void materialize(std::unique_ptr<MaterializationResponsibility> R) override;

//   private:
// 	void discard(const JITDylib &JD, const SymbolStringPtr &Sym) override {
// 		llvm_unreachable("Kaleidoscope functions are not overridable");
// 	}

// 	KaleidoscopeASTLayer &L;
// 	ast::decl_fn *F;
// };

// class KaleidoscopeASTLayer {
//   public:
// 	KaleidoscopeASTLayer(IRLayer &BaseLayer, const DataLayout &DL) : BaseLayer(BaseLayer), DL(DL) {}

// 	Error add(ResourceTrackerSP RT, ast::decl_fn *F) {
// 		return RT->getJITDylib().define(
// 			std::make_unique<KaleidoscopeASTMaterializationUnit>(*this, F), RT);
// 	}

// 	void emit(std::unique_ptr<MaterializationResponsibility> MR, ast::decl_fn *F) {
// 		BaseLayer.emit(std::move(MR), irgenAndTakeOwnership(F, ""));
// 	}

// 	MaterializationUnit::Interface getInterface(ast::decl_fn *F) {
// 		MangleAndInterner Mangle(BaseLayer.getExecutionSession(), DL);
// 		SymbolFlagsMap Symbols;
// 		Symbols[Mangle(F->ident.name)] =
// 			JITSymbolFlags(JITSymbolFlags::Exported | JITSymbolFlags::Callable);
// 		return MaterializationUnit::Interface(std::move(Symbols), nullptr);
// 	}

//   private:
// 	IRLayer &BaseLayer;
// 	const DataLayout &DL;
// };

// KaleidoscopeASTMaterializationUnit::KaleidoscopeASTMaterializationUnit(KaleidoscopeASTLayer &L,
//                                                                        ast::decl_fn *F)
// 	: MaterializationUnit(L.getInterface(F)), L(L), F(F) {}

// void KaleidoscopeASTMaterializationUnit::materialize(
// 	std::unique_ptr<MaterializationResponsibility> R) {
// 	L.emit(std::move(R), F);
// }

class KaleidoscopeJIT {
  private:
	std::unique_ptr<ExecutionSession> ES;
	std::unique_ptr<EPCIndirectionUtils> EPCIU;

	DataLayout DL;
	MangleAndInterner Mangle;

	RTDyldObjectLinkingLayer ObjectLayer;
	IRCompileLayer CompileLayer;
	IRTransformLayer OptimizeLayer;
	//KaleidoscopeASTLayer ASTLayer;

	JITDylib &MainJD;

	inline static void handleLazyCallThroughError() {
		errs() << "LazyCallThrough error: Could not find function body";
		exit(1);
	}

  public:
	inline KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES,
	                std::unique_ptr<EPCIndirectionUtils> EPCIU, JITTargetMachineBuilder JTMB,
	                DataLayout DL)
		: ES(std::move(ES)), EPCIU(std::move(EPCIU)), DL(std::move(DL)),
		  Mangle(*this->ES, this->DL),
		  ObjectLayer(*this->ES, []() { return std::make_unique<SectionMemoryManager>(); }),
		  CompileLayer(*this->ES, ObjectLayer,
	                   std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
		  OptimizeLayer(*this->ES, CompileLayer, optimizeModule),
		  MainJD(this->ES->createBareJITDylib("<main>")) {
		MainJD.addGenerator(
			cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));

		// Required for Windows:
		ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
	}

	inline ~KaleidoscopeJIT() {
		if (auto Err = ES->endSession())
			ES->reportError(std::move(Err));
		if (auto Err = EPCIU->cleanup())
			ES->reportError(std::move(Err));
	}

	static Expected<std::unique_ptr<KaleidoscopeJIT>> Create();

	inline const DataLayout &getDataLayout() const { return DL; }
	inline JITDylib &getMainJITDylib() { return MainJD; }
	Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr);
	Expected<JITEvaluatedSymbol> lookup(StringRef Name);

  private:
	static Expected<ThreadSafeModule> optimizeModule(ThreadSafeModule TSM,
	                                                 const MaterializationResponsibility &R);
};

} // namespace catalyst::compiler
