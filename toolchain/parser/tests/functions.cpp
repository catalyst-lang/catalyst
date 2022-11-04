#include <doctest.h>

#include "../src/parser.hpp"
#include "catalyst/ast.hpp"

using namespace catalyst;

TEST_CASE("functions") {
    auto tu = parser::parse_string(R"catalyst_source(
        fn foo() {}
    )catalyst_source");
    CHECK(tu.has_value());
}
