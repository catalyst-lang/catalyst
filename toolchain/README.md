# Toolchain

The toolchain is the combination of tools, libraries and scripts that collectively
represents 'the Catalyst compiler'. 

## Parser

The Parser reads tokens from input and produces an [Abstract Syntax Tree (AST)](https://en.wikipedia.org/wiki/Abstract_syntax_tree).

## Transpiler

The transpiler consumes an [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree) and produces C++ source code.

## Mechanic

The mechanic is the glue between the parts. It combines all the tools into one usable CLI tool that builds and organizes Catalyst projects.

## Common

Common files, libraries, tools and scripts used by more than one unit of the toolchain.
