#include <doctest.h>

#include "../src/compiler.hpp"
#include "catalyst/ast/ast.hpp"

using namespace catalyst;

TEST_CASE("functions") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 123i64;
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 123);
}
