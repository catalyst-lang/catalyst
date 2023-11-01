#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/CodeGenCoverage.h"

#include "object_file.hpp"
#include "codegen/codegen.hpp"

namespace catalyst::compiler {

bool write_object_file(const std::string &filename, const compile_result &result, const std::string &target_triple) {
    if (!result.is_successful) {
        return false;
    }

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string error;
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!target) {
        llvm::errs() << error;
        return false;
    }

    auto CPU = "generic";
    auto Features = "";

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();
    auto TargetMachine = target->createTargetMachine(target_triple, CPU, Features, opt, RM);

    // TODO: this should probably be set before codegeneration, as the optimizer might benefit from the knowledge
    auto &state = get_state(result);
    state.TheModule->setDataLayout(TargetMachine->createDataLayout());
    state.TheModule->setTargetTriple(target_triple);

    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs() << "Could not open file: " << ec.message();
        return false;
    }

    // add a pass that emits the code
    llvm::legacy::PassManager pass;
    auto FileType = llvm::CodeGenFileType::ObjectFile;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
        return false;
    }

    pass.run(*state.TheModule);
    dest.flush();

    return true;
}

} // namespace catalyst::compiler