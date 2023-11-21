#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("class casting") {

TEST_CASE("Upcasting") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A { var a = 4 }
        class B : A { var b = 5 }
        class C : B { var c = 6 }

        fn main() {
            var v = B()
            var s: A = v
            return s.a
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 4);
}

TEST_CASE("Downcasting") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A { var a = 4 }
        class B : A { var b = 5 }
        class C : B { var c = 6 }

        fn main() {
            var v: A = B()
            var s: B = v as B
            return s.a
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 4);
}

TEST_CASE("Downcasting MI") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A { var a = 4 }
        class B { var b = 5 }
        class C : A, B { var c = 6 }

        fn main() {
            var v: A = C()
            var s: C = v as C
            return s.c
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 6);
}

}
