#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("scopes") {

TEST_CASE("sub scopes") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a = 5
            {
                var a = 3
            }
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 5);
}

TEST_CASE("sub scopes 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a = 5
            {
                a = 3
            }
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 3);
}

TEST_CASE("sub scopes 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a = 5
            {
                var a = 3
                {
                    var a = 8
                }
            }
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 5);
}

TEST_CASE("sub scopes 4") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a = 5
            {
                var a = 3
                {
                    var a = 8
                }
                return a
            }
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 3);
}

TEST_CASE("sub scopes 5") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a = 5
            {
                var a = 3
                {
                    var a = 8
                    return a
                }
            }
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 8);
}

TEST_CASE("sub scopes 6") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a = 5
            {
                var a = 3
                var b = 3
                {
                    var a = 8
                    return a + b
                }
            }
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 11);
}

TEST_CASE("sub scopes 7") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        var a = 4
        fn main() {
            var a = 5
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 5);
}

TEST_CASE("sub scopes 8") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            var a = 5
            var total = 0
            {
                var a = 3
                total = total + a
            }
            total = total + a
            {
                var a = 9
                total = total + a
            }
            return total
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 17);
}

}
