#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("virtuals") {

TEST_CASE("Simple virtual function") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return 54
            }
        }

        class B : A {
            override fn test() {
                return 54398
            }
        }

        fn main() {
            var b = B()
            return b.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 54398);
}

TEST_CASE("Simple type-annotated virtual function") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return 54
            }
        }

        class B : A {
            override fn test() {
                return 54398
            }
        }

        fn main() {
            var b: A = B()
            return b.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 54398);
}

TEST_CASE("Virtual function on passed parameter") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return 54
            }
        }

        class B : A {
            override fn test() {
                return 54398
            }
        }

        fn get(a: A) {
            return a.test()
        }

        fn main() {
            var b = B()
            return get(b)
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 54398);
}

TEST_CASE("Virtual function without override") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return 54
            }

            virtual fn blaat() {
                return 13
            }
        }

        class B : A {
            override fn test() {
                return 54398
            }
        }

        fn main() {
            var b = B()
            return b.blaat()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 13);
}

TEST_CASE("Virtual function base class case") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return 54
            }

            virtual fn blaat() {
                return 13
            }
        }

        class B : A {
            override fn test() {
                return 54398
            }
        }

        fn main() {
            var b = A()
            return b.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 54);
}

TEST_CASE("Virtual function triple") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return 54
            }

            virtual fn blaat() {
                return 13
            }
        }

        class B : A {
            override fn test() {
                return 54398
            }
        }

        class C : B {
            override fn test() {
                return 8877
            }
        }

        fn main() {
            var b = C()
            return b.test()
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 8877);
}

TEST_CASE("Virtual functions advanced") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class A {
            virtual fn test() {
                return 54
            }

            virtual fn blaat() {
                return 13
            }
        }

        class B : A {
            override fn test() {
                return 54398
            }
        }

        fn get(a: A) {
            return a.test()
        }

        fn main() {
            var a = A()
            var b = B()
            return get(a) + get(b)
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 54 + 54398);
}

}
