#include <doctest.h>

#include "../src/compiler.hpp"
#include "catalyst/ast/ast.hpp"

using namespace catalyst;

TEST_SUITE("functions") {

TEST_CASE("concept") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 123i64;
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 123);
}

TEST_CASE("return void (explicit)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn v() -> void {

        }

        fn main() {
            v()
            return 123i64;
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 123);
}

TEST_CASE("return void (implicit)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn v() {
            return
        }

        fn main() {
            v()
            return 123i64
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 123);
}

TEST_CASE("return void (implicit no-return)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn v() {

        }

        fn main() {
            v()
            return 123i64
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 123);
}

TEST_CASE("returning") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn func() {
            return 246i64;
        }

        fn main() {
            return func();
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 246);
}

TEST_CASE("parameter") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn func(a: i64) {
            return a;
        }

        fn main() {
            return func(321i64);
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 321);
}

}
