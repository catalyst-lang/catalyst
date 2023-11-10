#include "codegen.hpp"

namespace catalyst::compiler::codegen {

void create_meta(state& state, std::ostream &out) {
    out << "CATA_META" << std::endl;
    out << version.string() << std::endl;
    out << state.global_namespace << std::endl;
    
    // iterate through state.symbol_table and serialize each symbol
    for (auto& [name, symbol] : state.symbol_table) {
        if (symbol.imported) continue;

        out << name;
        out << '\0';
        symbol.serialize(out);
    }
    
    out << "CATA_END" << '\0';
}

void read_meta(state &state, std::istream &in) {
    std::string line;
    std::getline(in, line);
    if (line != "CATA_META") {
        parser::report_message(parser::report_type::error, "Invalid metadata file", std::cerr);
        exit(1);
    }

    std::getline(in, line);
    if (line != version.string()) {
        parser::report_message(parser::report_type::error, "Invalid metadata version", std::cerr);
        exit(1);
    }

    std::getline(in, state.global_namespace);

    while (true) {
        std::string name;
        std::getline(in, name, '\0');
        if (name == "CATA_END") break;

        auto symbol = symbol::deserialize(state, in);
        symbol.imported = true;
        if (isa<type_function>(symbol.type.get())) {
            auto fun = (type_function*)symbol.type.get();
            auto llvm_type = static_cast<llvm::FunctionType*>(fun->get_llvm_type(state));
            auto callee = state.TheModule->getOrInsertFunction(name, llvm_type);
            symbol.value = callee.getCallee();
        }
        state.symbol_table[name] = symbol;
    }
}

}