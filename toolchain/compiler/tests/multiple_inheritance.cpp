#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("multiple inheritance") {

TEST_CASE("Simple multiple inheritance") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return this.a
            }

            var a = 5
        }

        class D {
            virtual fn dtest() {
                return this.i
            }

            var i = 44
        }

        class MI : A, D {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 5);
}

TEST_CASE("Simple multiple inheritance 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return this.a
            }

            var a = 5
        }

        class D {
            virtual fn dtest() {
                return this.i
            }

            var i = 44
        }

        class MI : A, D {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.dtest()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 44);
}

TEST_CASE("Simple multiple inheritance 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return this.a
            }

            fn bla() {
                return this.b
            }
            var a = 5
            var b = 3
        }

        class D {
            virtual fn dtest() {
                return this.i
            }

            fn test2() {
                return this.i
            }

            var i = 44
        }

        class MI : A, D {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.bla()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 3);
}

TEST_CASE("Simple multiple inheritance 4") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return this.a
            }

            fn bla() {
                return this.b
            }
            var a = 5
            var b = 3
        }

        class D {
            virtual fn dtest() {
                return this.i
            }

            fn test2() {
                return this.i + 4
            }

            var i = 44
        }

        class MI : A, D {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.test2()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 48);
}

TEST_CASE("Complex multiple inheritance") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return this.a
            }

            virtual fn blaat() {
                return 11
            }

            fn hoi() {
                return this.s
            }

            var a = 5
            var s = 333
        }

        class C {
            virtual fn ietsInD() {
                return 5
            }

        }

        class D : C {
            override fn ietsInD() {
                return this.i + 12
            }
            fn dtest() {
                return this.i
            }

            var i = 44
        }

        class MI : A, D {

            fn mitest() {
                return 9
            }

            var m = 4

        }

        fn main() {
            var v = MI()
            return v.ietsInD()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 56);
}

TEST_CASE("Function name conflict 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return this.a
            }

            fn bla() {
                return this.b
            }
            var a = 5
            var b = 3
        }

        class D {
            virtual fn test() {
                return this.i
            }

            fn test2() {
                return this.i + 4
            }

            var i = 44
        }

        class MI : A, D {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 5);
}

TEST_CASE("Function name conflict 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return this.a
            }

            fn bla() {
                return this.b
            }
            var a = 5
            var b = 3
        }

        class D {
            virtual fn test() {
                return this.i
            }

            fn test2() {
                return this.i + 4
            }

            var i = 44
        }

        class MI : D, A {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 44);
}

TEST_CASE("Function name conflict 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            fn test() {
                return this.a
            }

            var a = 5
            var b = 3
        }

        class D {
            fn test() {
                return this.i
            }

            var i = 44
        }

        class MI : D, A {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 44);
}

TEST_CASE("Diamond 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            fn test() {
                return this.a
            }

            var a = 5
            var b = 3
        }

        class B : A {
            fn testB() {
                return this.i
            }

            var i = 44
        }

        class C : A {
            fn testC() {
                return this.i
            }

            var i = 44
        }

        class MI : B, C {
            var m = 4
        }

        fn main() {
            var v = MI()
            return v.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 5);
}

}
