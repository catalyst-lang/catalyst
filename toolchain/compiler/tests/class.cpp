#include <doctest.h>

#include "../src/compiler.hpp"
#include "catalyst/ast/ast.hpp"

using namespace catalyst;

TEST_SUITE("classes") {

TEST_CASE("class concept") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn main() {
            var a = bla();
            a.i = 10
            a.j = 44.44
            a.k = 9i32
            return a.i + a.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 19);
}

TEST_CASE("ref on assign") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn main() {
            var a = bla();
            a.i = 10
            a.j = 44.44
            a.k = 9i32

            var b = a

            a.i = 1
            a.j = 3.4
            a.k = 13

            return b.i + b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 14);
}

TEST_CASE("ref on pass") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn test(a: bla) {
            a.i = 1
            a.j = 3.4
            a.k = 13
            return 0
        }

        fn main() {
            var a = bla();
            a.i = 10
            a.j = 44.44
            a.k = 9i32

            test(a)

            return a.i + a.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 14);
}

TEST_CASE("ref on return") {
    compiler::options opts;
    // TODO: this doesn't really check if it's a ref
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn test() -> bla {
            var a = bla()
            a.i = 1
            a.j = 3.4
            a.k = 13
            return a
        }

        fn main() {
            var a = test()
            return a.i + a.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 14);
}

TEST_CASE("ref on return and pass as parameter 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn test() -> bla {
            var a = bla()
            a.i = 1
            a.j = 3.4
            a.k = 13
            return a
        }

        fn piz(b: bla) {
            return b.k
        }

        fn main() {
            var a = piz(test())
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 13);
}

TEST_CASE("ref on return and pass as parameter 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn test() -> bla {
            var a = bla()
            a.i = 1
            a.j = 3.4
            a.k = 13
            return a
        }

        fn piz(b: bla) {
            return b.k
        }

        fn main() {
            var a = bla()
            a.i = 44
            a.k = 33
            var b = bla()
            b.k = 222
            b.i = piz(test())
            b.k = piz(a)
            return b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 33);
}

TEST_CASE("pass on return and pass as parameter 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn test(b: bla) {
            b.i = 13
        }

        fn main() {
            var a = bla()
            a.i = 44
            a.k = 33
            test(a)
            return a.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 13);
}

TEST_CASE("pass on return and pass as parameter 4") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9
            var k = 9i32
        }

        fn test() -> bla {
            var a = bla()
            a.i = 1
            a.j = 3
            a.k = 13
            return a
        }

        fn test2(b: bla) -> bla {
            b.i = 83
            return b
        }

        fn piz(b: bla) {
            b.j = 123
            return b.i
        }

        fn main() {
            var r = test2(test())
            return piz(r) + r.k + r.j
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 219);
}

TEST_CASE("pass ref on return and pass as parameter 5") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn test() -> bla {
            var a = bla()
            a.i = 1
            a.j = 3.4
            a.k = 13
            return a
        }

        fn test2(b: bla) -> bla {
            b.i = 83
            return b
        }

        fn piz(b: bla) {
            return b.i
        }

        fn main() {
            var a = test()
            var b = piz(test2(a))
            return a.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 83);
}

TEST_CASE("ref on return and pass as parameter 6") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn test() -> bla {
            var a = bla()
            a.i = 188
            a.j = 3.4
            a.k = 13
            return a
        }

        fn main() {
            var a = test().i
            return a
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 188);
}

TEST_CASE("advanced 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class test1{ var b: i8; var c: i8; var d: i32; var e: i16 }

        class gleam {
            var io = true
            var poeder = 3
            var zz = test1()
        }

        class bla { 
            var i = 4
            var j = 9.8
            //fn lskd() {}
            var g = gleam()
        }

        fn main() {
            var a = bla();

            a.i = 3
            a.j = 1.1
            a.g.poeder = 7779

            var b = a

            b.i = 10
            b.j = 44.44
            b.g.poeder = 6669

            return a.g.poeder
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 6669);
}

TEST_CASE("method 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn set_k(k: i64) {
                this.k = k
            }
        }

        fn main() {
            var b = bla()
            b.set_k(32)
            return b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 32);
}

TEST_CASE("method 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn set_k(k: i64) {
                this.k = k
            }
        }

        fn test() -> bla {
            var a = bla()
            a.i = 1
            a.j = 3.4
            a.k = 13
            return a
        }

        fn main() {
            var a = test()
            a.set_k(99)
            var b = bla()
            b.set_k(32)
            return a.k + b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 131);
}

TEST_CASE("method 3") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn set_k(k: i64) {
                this.k = k
            }
        }


        fn main() {
            var a = bla()
            var b = a
            a.set_k(32)
            return b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 32);
}

TEST_CASE("class initializers") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class Bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn main() {
            var b = Bla()
            return b.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 4);
}

TEST_CASE("class new()") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class Bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn new() {
                this.k = 321
            }
        }

        fn main() {
            var b = Bla()
            return b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 321);
}

TEST_CASE("class initializers + new()") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class Bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn new() {
                this.k = 321
            }
        }

        fn main() {
            var b = Bla()
            return b.k + b.i
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 325);
}

TEST_CASE("class new(i64)") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class Bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn new(variable: i64) {
                this.k = variable
            }
        }

        fn main() {
            var b = Bla(43)
            return b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 43);
}

TEST_CASE("class new overloading 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class Bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn new(variable: i64) {
                this.k = variable
            }

            fn new() {
                this.k = 123
            }
        }

        fn main() {
            var b = Bla(44)
            return b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 44);
}

TEST_CASE("class new overloading 2") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        class Bla { 
            var i = 4
            var j = 9.8
            var k = 9i32

            fn new(variable: i64) {
                this.k = variable
            }

            fn new() {
                this.k = 123
            }
        }

        fn main() {
            var b = Bla()
            return b.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run<int64_t>(result);
    CHECK(ret == 123);
}

}
