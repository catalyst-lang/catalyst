#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("types") {

TEST_CASE("type: void") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a: void
            var b = a
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    REQUIRE(result.is_successful);
}


}
