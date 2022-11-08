// Copyright (c) 2021-2022 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include "llvm/Support/TargetSelect.h"

#include "jit.hpp"

namespace catalyst::compiler {

using namespace llvm;
using namespace llvm::orc;

int64_t run_jit(codegen::state &state) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    auto TheJIT = KaleidoscopeJIT::Create();
    if (TheJIT.takeError()) {
        codegen::state::report_error("Problem while initializing JIT");
        return -1;
    }
    state.TheModule->setDataLayout((*TheJIT)->getDataLayout());
    (*TheJIT)->addModule(ThreadSafeModule(std::move(state.TheModule), std::move(state.TheContext)));

    auto ExprSymbol = (*TheJIT)->lookup("main");
    assert(ExprSymbol && "Entry function not found");

    // Get the symbol's address and cast it to the right type (takes no
    // arguments, returns a double) so we can call it as a native function.
    int64_t (*FP)() = (int64_t (*)())(intptr_t)(*ExprSymbol).getAddress();
    return FP();
}

Expected<std::unique_ptr<KaleidoscopeJIT>> KaleidoscopeJIT::Create() {
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

Expected<ThreadSafeModule> KaleidoscopeJIT::optimizeModule(ThreadSafeModule TSM,
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

Error KaleidoscopeJIT::addModule(ThreadSafeModule TSM, ResourceTrackerSP RT) {
    if (!RT)
        RT = MainJD.getDefaultResourceTracker();

    return OptimizeLayer.add(RT, std::move(TSM));
}

Expected<JITEvaluatedSymbol> KaleidoscopeJIT::lookup(StringRef Name) {
    auto mangled = Mangle(Name.str());
    return ES->lookup({&MainJD}, mangled);
}

} // namespace catalyst
