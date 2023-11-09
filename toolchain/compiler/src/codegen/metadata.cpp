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
        out << std::endl;
    }
    
    out << "CATA_END" << std::endl;
}

}