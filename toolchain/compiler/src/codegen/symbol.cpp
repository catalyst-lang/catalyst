#include "symbol.hpp"
#include "type.hpp"
#include "codegen.hpp"

namespace catalyst::compiler::codegen {

void symbol::serialize(std::ostream& out) const {
    type->serialize(out);
}

symbol symbol::deserialize(state &state, std::istream& in) {
    symbol s;
    s.ast_node = nullptr;
    s.type = type::deserialize(state, in);
    s.value = nullptr;
    return s;
}

}