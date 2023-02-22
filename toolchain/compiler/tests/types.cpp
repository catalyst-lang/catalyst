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

TEST_CASE("type: function") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn bla(ks: i64) {
            return ks + 1
        }

        fn main() {
            var a : (num: i64) -> i64
            a = bla
            return a(99)
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 100);
}

TEST_CASE("type: function (mismatch)") {
    compiler::options opts;
    std::cout.setstate(std::ios_base::failbit);
    auto result = compiler::compile_string(R"catalyst_source(
        fn bla() {
            return ks + 1
        }

        fn main() {
            var a : (num: i64) -> i64
            a = bla
            return a()
        }
    )catalyst_source", opts);
    std::cout.clear();
    REQUIRE_FALSE(result.is_successful);
}


}
