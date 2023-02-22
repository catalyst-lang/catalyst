#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("types") {

TEST_CASE("type: void") {
    compiler::options opts;
    std::cout.setstate(std::ios_base::failbit);
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a: void
            var b = a
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    std::cout.clear();
    REQUIRE(result.is_successful);
}


}
