#include "llvm/Support/TargetSelect.h"

#include "runtime.hpp"
#include <iostream>

using namespace catalyst;
using type = compiler::codegen::type;

namespace catalyst::compiler {

int32_t foo() {
	printf("hier!\n");
	return 6;
}

int32_t bar() {
	printf("bar!\n");
	return 9;
}

int32_t baz(int64_t p) {
	std::cout << "baz!" << p << std::endl;
	return 9;
}

int32_t nacho(double p) {
	std::cout << "nacho!" << p << std::endl;
	return 9;
}

float pizza() {
	printf("pizza!\n");
    return 1.23f;
}

runtime::runtime(codegen::state &state) : target(state) {
    
}

void runtime::register_symbols() {    
	insert_function("foo", llvm::pointerToJITTargetAddress(&foo), type::create_builtin("i32"));
    insert_function("bar", llvm::pointerToJITTargetAddress(&bar), type::create_builtin("i32"));
    insert_function("baz", llvm::pointerToJITTargetAddress(&baz), type::create_builtin("i32"), { type::create_builtin("i64") });
    insert_function("nacho", llvm::pointerToJITTargetAddress(&nacho), type::create_builtin("i32"), { type::create_builtin("f64") });
    insert_function("pizza", llvm::pointerToJITTargetAddress(&pizza), type::create_builtin("f32"));
}

void runtime::insert_function(const char* name, llvm::JITTargetAddress target, const std::shared_ptr<type> &return_type) {
	auto fun = this->state->TheModule->getOrInsertFunction(name, return_type->get_llvm_type(*state));
    
	auto funty = type::create_function(return_type);
	this->state->symbol_table[name] = codegen::symbol(nullptr, fun.getCallee(), funty);
    functions[name] = target;
}

void runtime::insert_function(const char* name, llvm::JITTargetAddress target, const std::shared_ptr<catalyst::compiler::codegen::type> &return_type, std::vector<std::shared_ptr<type>> const &parameters) {
    llvm::SmallVector<llvm::Type *> llvm_args;
    for (const auto &v : parameters)
      llvm_args.push_back(v->get_llvm_type(*state));

    auto fun = this->state->TheModule->getOrInsertFunction(name, llvm::FunctionType::get(return_type->get_llvm_type(*state), llvm_args, false));
	auto funty = type::create_function(return_type, parameters);
	this->state->symbol_table[name] = codegen::symbol(nullptr, fun.getCallee(), funty);
    functions[name] = target;
}

// void* my_malloc(uint64_t size) {
// 	return malloc(size);
// }

// llvm::FunctionCallee runtime::get_malloc() {
// 	auto m = this->state->TheModule->getOrInsertFunction(
// 		"my_malloc", 
// 		llvm::AttributeList::get(*state->TheContext, 0, { llvm::Attribute::NoAlias }),
// 		llvm::PointerType::get(*state->TheContext, 0), 
// 		llvm::IntegerType::get(*state->TheContext, 64)
// 	);

// 	functions["my_malloc"] = llvm::pointerToJITTargetAddress(&my_malloc);

// 	return m;
// }

llvm::FunctionCallee runtime::get_malloc() {
	auto m = this->state->TheModule->getOrInsertFunction(
		"malloc", 
		llvm::AttributeList::get(*state->TheContext, 0, { llvm::Attribute::NoAlias }),
		llvm::PointerType::get(*state->TheContext, 0), 
		llvm::IntegerType::get(*state->TheContext, sizeof(size_t) * 8)
	);

	functions["malloc"] = llvm::pointerToJITTargetAddress(&malloc);

	return m;
}

llvm::FunctionCallee runtime::get_realloc() {
	return this->state->TheModule->getOrInsertFunction(
		"realloc", 
		llvm::PointerType::get(*state->TheContext, 0),
		llvm::PointerType::get(*state->TheContext, 0),
		llvm::IntegerType::get(*state->TheContext, sizeof(size_t) * 8)
	);
}

llvm::FunctionCallee runtime::get_free() {
	return this->state->TheModule->getOrInsertFunction(
		"free", 
		llvm::Type::getVoidTy(*state->TheContext), 
		llvm::PointerType::get(*state->TheContext, 0)
	);
}

}
