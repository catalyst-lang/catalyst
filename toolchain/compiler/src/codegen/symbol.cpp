#include "symbol.hpp"
#include "type.hpp"

namespace catalyst::compiler::codegen {

void symbol::serialize(std::ostream& out) const {
    type->serialize(out);
}

symbol symbol::deserialize(std::istream& in) {
    symbol s;
    s.ast_node = nullptr;
    //type = readBinary(in, s.ast_node);
    return s;
}

}