#include <doctest.h>

#include "../src/parser.hpp"
#include "catalyst/ast/ast.hpp"

using namespace catalyst;

TEST_CASE("single line comment") {
    auto tu = parser::parse_string(R"catalyst_source(
        // this is a comment
        fn foo() {}
    )catalyst_source");
    CHECK(tu.has_value());
}

TEST_CASE("comment after statement") {
    auto tu = parser::parse_string(R"catalyst_source(
        var a = 4 // this is also a comment
    )catalyst_source");
    CHECK(tu.has_value());
}

TEST_CASE("simple comments") {
    auto tu = parser::parse_string(R"catalyst_source(
        // this is a comment
        var a = 4 // this is also a comment
    )catalyst_source");
    CHECK(tu.has_value());
}

TEST_CASE("simple comments 2") {
    auto tu = parser::parse_string(R"catalyst_source(
        // this is a comment
        var a = 4 // this is also a comment
        var b = 3
)catalyst_source");
    CHECK(tu.has_value());
}
