#include <doctest.h>
#include <iostream>

#include "../src/compiler.hpp"

using namespace catalyst;

TEST_SUITE("namespaces") {

TEST_CASE("global ns specifier") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas

        fn main() {
            return 7
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 7);
}

TEST_CASE("global and scoped ns specifier") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas

        ns test {
            fn bla() {
                return 998
            }
        }

        fn main() {
            return test.bla()
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 998);
}

TEST_CASE("scoped ns specifier") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns test {
            fn bla() {
                return 9978
            }
        }

        fn main() {
            return test.bla()
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 9978);
}

TEST_CASE("nested ns specifiers") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns test {
            ns test2 {
                fn bla() {
                    return 76
                }
            }
        }

        fn main() {
            return test.test2.bla()
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 76);
}

TEST_CASE("nested ns specifiers 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns test {
            ns test2 {
                fn bla() {
                    return 76
                }
            }

            fn hoi() {
                return test2.bla()
            }
        }

        fn main() {
            return test.hoi()
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 76);
}

TEST_CASE("nested ns specifiers 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns test {
            ns test2 {
                fn bla() {
                    return 76
                }

                var foo = 443
            }

            fn hoi() {
                return test2.foo
            }
        }

        fn main() {
            return test.hoi()
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 443);
}

TEST_CASE("nested ns specifiers 4") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns test {
            ns test2 {
                var foo = 88
            }
        }

        fn main() {
            return test.test2.foo
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 88);
}

TEST_CASE("multiple global ns specifiers") {
    compiler::options opts;
    std::cout.setstate(std::ios_base::failbit);
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas

        fn main() {
            return 988
        }
        
        ns hoi
    )catalyst_source", opts);
    std::cout.clear();
    REQUIRE_FALSE(result.is_successful);
}

TEST_CASE("ns variable") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas {
            var food = 4
        }

        fn main() {
            return bas.food
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 4);
}

TEST_CASE("ns variable shadowing 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        var food = 5
        ns bas {
            var food = 4
        }

        fn main() {
            return bas.food
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 4);
}

TEST_CASE("ns variable shadowing 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        var food = 5
        ns bas {
            var food = 4
            ns test {
                var food = 3
            }
        }

        fn main() {
            return bas.test.food
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 3);
}

TEST_CASE("ns class") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas {
            class Bla {
                var i = 2
            }
        }

        fn main() {
            var b = bas.Bla()
            return b.i
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 2);
}

TEST_CASE("ns class (with type specifier)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas {
            class Bla {
                var i = 2
            }
        }

        fn main() {
            var b: bas.Bla = bas.Bla()
            return b.i
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 2);
}

TEST_CASE("ns struct") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas {
            struct Bla {
                var i = 26
            }
        }

        fn main() {
            var b = bas.Bla()
            return b.i
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 26);
}

TEST_CASE("ns class") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        ns bas {
            class Bla {
                var i = 2
                fn new() {
                    var t = roo.Dix()
                    this.i = t.d
                }
            }
        }

        ns roo {
            class Dix {
                var d = 4
            }
        }

        fn main() {
            var b = bas.Bla()
            return b.i
        }
    )catalyst_source", opts);
    auto ret = compiler::run<int64_t>(result);
    REQUIRE(result.is_successful);
    CHECK(ret == 4);
}


}
