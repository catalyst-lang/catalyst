#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("inheritance") {

TEST_CASE("Accessing base class") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
        }

        class B : A {
            var j = 2i64
            var k = 3.3f32
        }

        fn main() {
            var i = B()
            return i.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 91);
}

TEST_CASE("Accessing direct members 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
        }

        class B : A {
            var j = 2i64
            var k = 3.3f32
        }

        fn main() {
            var i = B()
            return i.j
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 2);
}

TEST_CASE("Accessing direct members 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
        }

        class B : A {
            var j = 2i64
            var k = 65i16
        }

        fn main() {
            var i = B()
            return i.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int16_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 65);
}

TEST_CASE("Base class function") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
            fn test() {
                return 44i64
            }
        }

        class B : A {
            var j = 2i64
            var k = 65i16
        }

        fn main() {
            var i = B()
            return i.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int16_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 44);
}

TEST_CASE("Direct class function") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
            fn test() {
                return 44i64
            }
        }

        class B : A {
            var j = 2i64
            var k = 65i16
            
            fn test2() {
                return 49i64
            }
        }

        fn main() {
            var i = B()
            return i.test2()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int16_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 49);
}

TEST_CASE("Triple inheritance") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
        }

        class B : A {
            var j = 2i64
            var k = 3.3f32
        }
        
        class C : B {
            var m = 55i64
        }

        fn main() {
            var i = C()
            return i.i + i.j + i.m
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 148);
}

TEST_CASE("Shadowing members") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
        }

        class B : A {
            var i = 2i64
            var k = 65i16
        }

        fn main() {
            var i = B()
            return i.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(result.result_type_name == "i64");
    CHECK(ret == 2);
}

TEST_CASE("Shadowing functions") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
            fn test() {
                return 44i16
            }
        }

        class B : A {
            var j = 2i64
            var k = 65i16
            
            fn test() {
                return 49i64
            }
        }

        fn main() {
            var i = B()
            return i.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int16_t>(result);
    REQUIRE(result.is_successful);
    CHECK(result.result_type_name == "i64");
    CHECK(ret == 49);
}

TEST_CASE("Simple polymorphism") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
        }

        class B : A {
            var j = 2i64
            var k = 65i16
        }

        class C : A {
            var e = 8i64
            var q = 53i16
        }

        fn main() {
            var i : A = B()
            return i.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int16_t>(result);
    REQUIRE(result.is_successful);
    CHECK(result.result_type_name == "i32");
    CHECK(ret == 91);
}

TEST_CASE("Simple polymorphism with constructor") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32
        }

        class B : A {
            fn new() {
                this.i = 321
            }

            var j = 2i64
            var k = 65i16
        }

        class C : A {
            fn new() {
                this.i = 567
            }

            var e = 8i64
            var q = 53i16
        }

        fn main() {
            var i : A = B()
            return i.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int16_t>(result);
    REQUIRE(result.is_successful);
    CHECK(result.result_type_name == "i32");
    CHECK(ret == 321);
}

TEST_CASE("Polymorphism static functions") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32

            fn test() {
                return 4i64
            }
        }

        class B : A {
            var j = 2i64
            var k = 65i16

            fn test() {
                return 5i64
            }
        }

        fn main() {
            var i : A = B()
            return i.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 4);
}

TEST_CASE("Polymorphism static functions (alternative)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            var i = 91i32

            fn test() {
                return 4i64
            }
        }

        class B : A {
            var j = 2i64
            var k = 65i16

            fn test() {
                return 5i64
            }
        }

        fn main() {
            var i : B = B()
            return i.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 5);
}

}
