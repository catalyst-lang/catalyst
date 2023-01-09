#include <doctest.h>

#include "../src/compiler.hpp"
#include "catalyst/ast/ast.hpp"

using namespace catalyst;

TEST_SUITE("structs") {

TEST_CASE("concept") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        struct bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn main() {
            var a: bla;
            a.i = 10
            a.j = 44.44
            a.k = 9i32
            return a.i + a.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 19);
}

TEST_CASE("copy on assign") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        struct bla { 
            var i = 4
            var j = 9.8
            var k = 9i32
        }

        fn main() {
            var a: bla;
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
	auto ret = compiler::run(result);
    CHECK(ret == 19);
}

TEST_CASE("copy on pass") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        struct bla { 
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
            var a: bla;
            a.i = 10
            a.j = 44.44
            a.k = 9i32

            test(a)

            return a.i + a.k
        }
    )catalyst_source", opts);
	auto ret = compiler::run(result);
    CHECK(ret == 19);
}

TEST_CASE("advanced 1") {
    compiler::options opts;
    auto result = compiler::compile_string(R"catalyst_source(
        struct bla { 
            var i = 4
            var j = 9.8
            //fn lskd() {}
            var g: gleam
        }

        struct gleam {
            var io = true
            var poeder = 3
            var zz: test1
        }

        struct test1{ var b: i8; var c: i8; var d: i32; var e: i16 }

        fn main() {
            var a: bla;

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
	auto ret = compiler::run(result);
    CHECK(ret == 7779);
}

}
