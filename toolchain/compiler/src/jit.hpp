// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#pragma once

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
#include <memory>

#include "../common/catalyst/ast/ast.hpp"

namespace catalyst::compiler {

using namespace llvm;
using namespace llvm::orc;

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

	static void handleLazyCallThroughError() {
		errs() << "LazyCallThrough error: Could not find function body";
		exit(1);
	}

  public:
	KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES,
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
	}

	~KaleidoscopeJIT() {
		if (auto Err = ES->endSession())
			ES->reportError(std::move(Err));
		if (auto Err = EPCIU->cleanup())
			ES->reportError(std::move(Err));
	}

	static Expected<std::unique_ptr<KaleidoscopeJIT>> Create() {
		auto EPC = SelfExecutorProcessControl::Create();
		if (!EPC)
			return EPC.takeError();

		auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

		auto EPCIU = EPCIndirectionUtils::Create(ES->getExecutorProcessControl());
		if (!EPCIU)
			return EPCIU.takeError();

		(*EPCIU)->createLazyCallThroughManager(
			*ES, pointerToJITTargetAddress(&handleLazyCallThroughError));

		if (auto Err = setUpInProcessLCTMReentryViaEPCIU(**EPCIU))
			return std::move(Err);

		JITTargetMachineBuilder JTMB(ES->getExecutorProcessControl().getTargetTriple());

		auto DL = JTMB.getDefaultDataLayoutForTarget();
		if (!DL)
			return DL.takeError();

		return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(*EPCIU), std::move(JTMB),
		                                         std::move(*DL));
	}

	const DataLayout &getDataLayout() const { return DL; }

	JITDylib &getMainJITDylib() { return MainJD; }

	Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
		if (!RT)
			RT = MainJD.getDefaultResourceTracker();

		return OptimizeLayer.add(RT, std::move(TSM));
	}

	Expected<JITEvaluatedSymbol> lookup(StringRef Name) {
		auto mangled = Mangle(Name.str());
		return ES->lookup({&MainJD}, mangled);
	}

  private:
	static Expected<ThreadSafeModule> optimizeModule(ThreadSafeModule TSM,
	                                                 const MaterializationResponsibility &R) {
		TSM.withModuleDo([](Module &M) {
			// Create a function pass manager.
			auto FPM = std::make_unique<legacy::FunctionPassManager>(&M);

			// Add some optimizations.
			FPM->add(createInstructionCombiningPass());
			FPM->add(createReassociatePass());
			FPM->add(createGVNPass());
			FPM->add(createCFGSimplificationPass());
			FPM->doInitialization();

			// Run the optimizations over all functions in the module being added to
			// the JIT.
			for (auto &F : M)
				FPM->run(F);
		});

		return std::move(TSM);
	}
};

} // namespace catalyst::compiler
