#include <doctest.h>

#include "../src/compiler.hpp"
#include "catalyst/ast/ast.hpp"

using namespace catalyst;

TEST_SUITE("functions") {

TEST_CASE("function concept") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 123i64;
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 123);
}

TEST_CASE("initialize globals") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        var hi = 4
        fn main() {
            return hi;
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 4);
}

TEST_CASE("changing globals") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        var hi = 4
        fn main() {
            hi = 56
            return hi;
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 56);
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

TEST_CASE("function overloading (exact parameter match)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn test() {
            return 4
        }

        fn test(variable: i32) {
            return 4 + variable
        }

        fn test(variable: f32) {
            return 9 + variable
        }

        fn main() {
            return test(1i32)
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 5);
}

TEST_CASE("function overloading (ambiguous parameter)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn test() {
            return 4
        }

        fn test(variable: i32) {
            return 4 + variable
        }

        fn test(variable: f32) {
            return 9 + variable
        }

        fn main() {
            test(1i64)
        }
    )catalyst_source", opts);
    CHECK(result.is_successful == false); // Should fail as ambiguous call
}

TEST_CASE("function overloading (no parameter case)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn test() {
            return 4
        }

        fn test(variable: i32) {
            return 4 + variable
        }

        fn test(variable: f32) {
            return 9 + variable
        }

        fn main() {
            test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 4);
}

}
