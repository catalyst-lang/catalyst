#include "llvm/Support/TargetSelect.h"

#include "runtime.hpp"

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
	printf("baz! %ld\n", p);
	return 9;
}

int32_t nacho(double p) {
	printf("nacho! %f\n", p);
	return 9;
}

float pizza() {
	printf("pizza!\n");
    return 1.23f;
}

runtime::runtime(codegen::state &state) : state(&state) {
    
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
	this->state->symbol_table[std::string("root.") + name] = codegen::symbol(nullptr, fun.getCallee(), funty);
    functions[name] = target;
}

void runtime::insert_function(const char* name, llvm::JITTargetAddress target, const std::shared_ptr<catalyst::compiler::codegen::type> &return_type, std::vector<std::shared_ptr<type>> const &parameters) {
    llvm::SmallVector<llvm::Type *> llvm_args;
    for (const auto &v : parameters)
      llvm_args.push_back(v->get_llvm_type(*state));

    auto fun = this->state->TheModule->getOrInsertFunction(name, llvm::FunctionType::get(return_type->get_llvm_type(*state), llvm_args, false));
	auto funty = type::create_function(return_type, parameters);
	this->state->symbol_table[std::string("root.") + name] = codegen::symbol(nullptr, fun.getCallee(), funty);
    functions[name] = target;
}

}
