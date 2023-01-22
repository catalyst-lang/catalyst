#include <doctest.h>

#include "../src/compiler.hpp"
#include "catalyst/ast/ast.hpp"

using namespace catalyst;

TEST_SUITE("arithmetics") {

TEST_CASE("integer addition") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 + 5
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 8);
}

TEST_CASE("integer addition 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 + 5 + 7
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 15);
}

TEST_CASE("float addition") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 4.4 + 6.6
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(11.0));
}

TEST_CASE("float addition 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 4.4 + 6.6 + 5.5
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(16.5));
}

TEST_CASE("integer cast addition") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3i16 + 5i32
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    CHECK(ret == 8);
}

TEST_CASE("integer addition and subtraction") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 + 5 - 6
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 2);
}

TEST_CASE("integer addition and subtraction 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 - 5 + 6
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 4);
}

TEST_CASE("integer addition and subtraction 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 - (5 + 6)
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == -8);
}

TEST_CASE("integer multiplication") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 * 6 * 2
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 36);
}

TEST_CASE("float multiplication") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3.5 * 6.2
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(21.7));
}

TEST_CASE("integer and float multiplication") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 * 8 * 0.5
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(12.0));
}

TEST_CASE("integer division") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 10 / 2
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 5);
}

TEST_CASE("integer division 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 8 / 3
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 2);
}

TEST_CASE("integer division 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 89 / 3 / 4
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 7);
}

TEST_CASE("float division 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 48.14 / 6.64
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(7.25));
}

TEST_CASE("float division 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 181.922853 / 6.23 / 5.7 
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(5.123));
}

TEST_CASE("float/int division 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 89 / 3.0 / 4
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(7.416666666666667));
}

TEST_CASE("float/int division 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 89 / 3 / 4.0
        }
    )catalyst_source", opts);
	auto ret = compiler::run<double>(result);
    CHECK(ret == doctest::Approx(7.25));
}

TEST_CASE("cast float -> integer") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 * 8 * 0.5 as i32
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    CHECK(ret == 0);
}

TEST_CASE("cast float -> integer 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return (3 * 8 * 0.5) as i32
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    CHECK(ret == 12);
}

TEST_CASE("cast integer -> float") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 3 as f32
        }
    )catalyst_source", opts);
	auto ret = compiler::run<float>(result);
    CHECK(ret == doctest::Approx(3.0));
}

TEST_CASE("signed int -> unsigned") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        fn main() {
            return 4u32 - 6i32
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int32_t>(result);
    CHECK(ret == -2);
}

}
