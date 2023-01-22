# The Catalyst Programming Language

[![continuous-integration](https://github.com/catalyst-lang/catalyst/actions/workflows/ci.yml/badge.svg)](https://github.com/catalyst-lang/catalyst/actions/workflows/ci.yml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/604b1c6c59004203810367fcd8ae6816)](https://www.codacy.com/gh/catalyst-lang/catalyst/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=catalyst-lang/catalyst&amp;utm_campaign=Badge_Grade)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=catalyst-lang_catalyst&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=catalyst-lang_catalyst)
[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=catalyst-lang_catalyst&metric=sqale_rating)](https://sonarcloud.io/summary/new_code?id=catalyst-lang_catalyst)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=catalyst-lang_catalyst&metric=ncloc)](https://sonarcloud.io/summary/new_code?id=catalyst-lang_catalyst)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=catalyst-lang_catalyst&metric=coverage)](https://sonarcloud.io/summary/new_code?id=catalyst-lang_catalyst)

**Catalyst** is an easy to use, feature-complete general-purpose programming language.

Catalyst compiles to highly optimized machine-code, resulting in very high-performing code.

> **Warning**
> The language is under heavy development.

Catalyst's main draw is to provide a modern, current programming language that allows you to write efficient code using high-level concepts and modern, easy to use idioms.  

Catalyst is designed to be a replacement for the usecases where C++ is typically used. C++ is a very powerful language, but has become very bloated and convoluted because of the decades of legacy it needs to drag along. Catalyst is a modern language with much the same goals, while also providing a modern-day development experience in both syntax as well as idioms.

## Usage
```
catalyst file.ct
./build/file
```

## Building
Dependencies for building:
  - `python` 3.6 or later (installing [Python](https://www.python.org/downloads/))
  - `cmake` 3.18 or later (installing [CMake](https://cmake.org/install/))
  - `llvm-dev` 14 or later.  
      - Use `build_and_install_llvm.bat` on Windows.
      - On Linux (or most other Unixes) use:
          ```
          $ wget https://apt.llvm.org/llvm.sh
          $ chmod +x llvm.sh
          $ sudo ./llvm.sh
          ```
      - On MacOS use:
        ```
        $ xcode-select --install
        $ brew install coreutils llvm
        ```
        prefix the `cmake configure` command below with `LLVM_DIR=/opt/homebrew/opt/llvm`

For building, we typically use Ninja, but the platform default should also work by _not_ specifying `-GNinja` below.
```
$ cmake -GNinja -B build -DCMAKE_BUILD_TYPE=Release
$ cmake --build build
```
run the tests:
```
$ ./build/toolchain/parser/test-parser
$ ./build/toolchain/compiler/test-compiler
```

## Goals

1. **Modern syntax and idoms**\
    C++ syntax is a [mess](https://medium.com/@mujjingun_23509/full-proof-that-c-grammar-is-undecidable-34e22dd8b664). While C++ painstakinly aims to remain backwards compatible (for good reasons), we can start with a clean slate. Also, we do not aim for backwards compatibility across major iterations.
2. **Memory safety**\
    One of the biggest challenges with low-level programming is preventing memory leaks and working with pointers in general. C and C++ is notorious for leaving the responsibilty to the programmer, while Rust is the exact opposite leaving developers to [fight the borrow checker](https://kerkour.com/life-is-short-rust-borrow-checker). We believe a programmer is more than capable of working in a safe way by giving him or her the right tools.
3. **Developer productivity**\
    Modern, idiomatic C++ is still [very verbose](https://stackoverflow.com/questions/39769544/why-do-c-stl-function-calls-need-to-be-so-verbose). Working with containers and iterators is very powerful, but also intimidatingly extensive in usage. Most of the time, this power is not needed and we can provide succinct syntax for common (default) usages, while still provide the same level of control if needed.
4. **Executable performance**\
    Catalyst aims to transpile to idiomatic C++. In most cases, that results in an executable performance that is equal to a codebase that is fully in C++. In some cases, we can even optimize further and improve performace above C++. 
5. **Platform-agnostic / Multi-platform**\
    While modern C++ is doing a better job at being able to provide the programmer with tools to write once, run everywhere (like `std::thread` and `std::chrono`), there are still a lot of api's not (fully) cross-platform (Thread pools, shared library loading, IPC, semaphores, etc), not to mention the build system (incompatibilities between LLVM/MSVC/GCC). Catalyst aims to abstract all this away.
6. **Not be a Google project**\
    We don't want to end up on the [Google Graveyard](https://killedbygoogle.com/) and we don't want a marketing firm to run the organization of a programming language. We believe in Open Source and the governance of OS-projects. We know about Carbon. They seem sincere, but still [follow](https://github.com/carbon-language/carbon-lang/pull/221) [the](https://github.com/carbon-language/carbon-lang/pull/193) [Google](https://github.com/carbon-language/carbon-lang/blob/trunk/docs/project/evolution.md#carbon-leads-1) [agenda](https://cla.developers.google.com/about/google-individual?csw=1).


## Language Design Principles
see [Language Design Principles](language_design_principles.md).


## Project status

-   ❌ Structured programming
    -   ✅ While loops
    -   ✅ Variable declarations
    -   ❌ Variable initialization tracking
    -   ❌ Returned var
    -   ❌ Variadics
-   ❌ User defined types
    -   ✅ structs
    -   ✅ classes
    -   ❌ choice
-   ✅ Alias system
-   ❌ OO programming
    -   ❌ Inheritance
    -   ❌ Parameterized class methods w/ inheritance
    -   ❌ Destructors
    -   ✅ Methods
    -   ✅ Static functions / Class functions
-   ❌ Generic programming
    -   ✅ Generic classes
    -   ❌ Generic methods
    -   ✅ Generic functions
    -   ✅ Interfaces
    -   ✅ Generic Interfaces
    -   ✅ Impls
    -   ✅ Generic Impls
    -   ❌ Impl specialization
    -   ❌ Templates
-   ❌ Operator overloading
    -   ✅ ==
    -   ❌ /=
    -   ❌ Other operators
    -   ❌ Constraints
    -   ✅ Implicit “as”
-   ❌ Error handling
-   ✅ Prelude
    -   ✅ Print function
-   ❌ Types
    -   ✅ i32
    -   ❌ Other integral types
    -   ❌ Integral types as library types instead of native
    -   ✅ Tuples
    -   ✅ Pointer
    -   ✅ Functions
    -   ✅ Bool
    -   ✅ String
    -   ❌ Floating point types
    -   ❌ Raw string literals
-   ❌ Code organization
    -   ❌ Mixins
    -   ❌ Imports
    -   ❌ Separate packages
    -   ❌ Modules
    -   ❌ Namespaces
