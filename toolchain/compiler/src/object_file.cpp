#include <sstream>
#include <strstream>

#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/CodeGenCoverage.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/IRObjectFile.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Bitcode/BitcodeWriter.h"

#include "../../contrib/tar/tar.hpp"

#include "object_file.hpp"
#include "codegen/codegen.hpp"
#include "codegen/metadata.hpp"

namespace catalyst::compiler {

bool write_bitcode_file(std::ostringstream &oss, const compile_session &result) {
    if (!result.is_successful) {
        return false;
    }

    auto &state = get_state(result);
    llvm::raw_os_ostream raw_os(oss);
    llvm::buffer_ostream dest(raw_os);
    llvm::WriteBitcodeToFile(*state.TheModule, dest);

    return true;
}

bool write_object_file(std::ostringstream &oss, const compile_session &result, const std::string &target_triple) {
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

    // add a pass that emits the code
    llvm::legacy::PassManager pass;
    auto FileType = llvm::CodeGenFileType::CGFT_ObjectFile;

    llvm::raw_os_ostream raw_os(oss);
    llvm::buffer_ostream dest(raw_os);

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        llvm::errs() << "TargetMachine can't emit a file of this type";
        return false;
    }

    pass.run(*state.TheModule);

    return true;
}

bool write_bundle_file(const std::string &filename, const compile_session &result, const std::string &target_triple) {
    std::ofstream outfile(filename, std::ofstream::binary);
    tar::writer writer(outfile);

    {
        std::ostringstream oss_obj;
        write_object_file(oss_obj, result, target_triple);
        writer.put(target_triple, oss_obj.str().c_str(), oss_obj.str().length());
    }

    {
        std::ostringstream oss_bitcode;
        write_bitcode_file(oss_bitcode, result);
        writer.put("bitcode", oss_bitcode.str().c_str(), oss_bitcode.str().length());
    }
    
    {
        std::ostringstream oss_meta;
        create_meta(result, oss_meta);
        writer.put("metadata", oss_meta.str().c_str(), oss_meta.str().length());
    }

    return true;
}

bool read_bundle_file(const std::string &filename, const compile_session &session, const std::string &target_triple) {
    auto &state = get_state(session);
    std::ifstream infile(filename, std::ifstream::binary);
    tar::reader reader(infile);
    bool meta_read = false;
    bool obj_read = false;
    while (reader.contains_another_file()) {
        auto fn = reader.get_next_file_name();
        if (fn == "metadata") {
            // read metadata
            auto size = reader.get_next_file_size();
            auto target = (char *)malloc(size);
            reader.read_next_file(target);
            std::istrstream iss(target, size);
            auto &state = get_state(session);
            read_meta(state, iss);
            free(target);
            meta_read = true;
        } else if (fn == target_triple) {
            // read object file
            // TODO: binary object file format
            reader.skip_next_file();
        } else if (fn == "bitcode") {
            // read bitcode
            auto size = reader.get_next_file_size();
            auto target = (char *)malloc(size);
            reader.read_next_file(target);
            auto data = std::string(target, size);
            llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr =
                llvm::MemoryBuffer::getMemBuffer(data, "", false);
            std::unique_ptr<llvm::MemoryBuffer> objectFileBuffer = std::move(fileOrErr.get());

            llvm::Expected<std::unique_ptr<llvm::Module>> moduleOrErr = 
                llvm::parseBitcodeFile(objectFileBuffer->getMemBufferRef(), *state.TheContext);
            if (!moduleOrErr) {
                // Handle error
                std::cerr << "Error: " << llvm::toString(moduleOrErr.takeError()) << std::endl;
                exit(1);
            }
            llvm::Linker linker(*state.TheModule);
            if (linker.linkInModule(std::move(moduleOrErr.get()))) {
                // Handle error
                std::cerr << "Error: could not link in module" << std::endl;
                exit(1);
            }
            free(target);
            obj_read = true;
        } else {
            reader.skip_next_file();
        }
    }
    if (!meta_read) {
        std::cerr << "Error: metadata not found in bundle" << std::endl;
    }
    if (!obj_read) {
        std::cerr << "Error: object file not found in bundle for architecture " << target_triple << std::endl;
    }

    return meta_read && obj_read;
}

} // namespace catalyst::compiler