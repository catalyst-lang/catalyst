// Copyright (c) 2021-2023 Bas du Pré and Catalyst contributors
// SPDX-License-Identifier: MIT

#include <CLI11.hpp>
#include <rang.hpp>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <bit>

#include "catalyst/platform.hpp"
#include "catalyst/version.hpp"
#include "compiler.hpp"
#include "object_file.hpp"

#if CATALYST_PLATFORM_POSIX
#include <csignal>
void signal_handler(int s) {
	std::cout << std::endl << rang::style::reset << rang::fg::red << rang::style::bold;
	std::cout << "Control-C detected, exiting..." << rang::style::reset << std::endl;
	std::exit(1); // will call the correct exit func, no unwinding of the stack though
}
#endif

namespace catalyst::compiler {

struct cli_options {
	std::string input;
	bool compile_only = false;
	bool dump_ast = false;
	bool dump_bytecode = false;
	bool run = true;
	std::string output_file;
	std::string arch = compiler::get_default_target_triple();

	enum class OutputFormat { ASCII = 0, Color, Fancy } format = OutputFormat::ASCII;

	options compiler_options;
};

int main(const cli_options &opts) {
	auto result = compile_file(opts.input.c_str(), opts.compiler_options);
	compiler_debug_print(result);

	if (opts.output_file != "") {
		write_object_file(opts.output_file, result, opts.arch);
	}

	if (opts.run) {
		if (!result.is_runnable) {
			std::cout << "Error: entry point `main` not found or not a function." << std::endl;
			return 2;
		}

		if (result.result_type_name == "i8")
			std::cout << "Result: <i8> " << (int) run<int8_t>(result) << std::endl;
		else if (result.result_type_name == "u8")
			std::cout << "Result: <u8> " << (unsigned) run<uint8_t>(result) << std::endl;
		else if (result.result_type_name == "i16")
			std::cout << "Result: <i16> " << run<int16_t>(result) << std::endl;
		else if (result.result_type_name == "u16")
			std::cout << "Result: <u16> " << run<uint16_t>(result) << std::endl;
		else if (result.result_type_name == "i32")
			std::cout << "Result: <i32> " << run<int32_t>(result) << std::endl;
		else if (result.result_type_name == "u32")
			std::cout << "Result: <u32> " << run<uint32_t>(result) << std::endl;
		else if (result.result_type_name == "i64")
			std::cout << "Result: <i64> " << run<int64_t>(result) << std::endl;
		else if (result.result_type_name == "u64")
			std::cout << "Result: <u64> " << run<uint64_t>(result) << std::endl;
		else if (result.result_type_name == "f32")
			std::cout << "Result: <f32> " << run<float>(result) << std::endl;
		else if (result.result_type_name == "f64")
			std::cout << "Result: <f64> " << run<double>(result) << std::endl;
		else if (result.result_type_name == "bool")
			std::cout << "Result: <bool> " << run<bool>(result) << std::endl;
		else 
			std::cout << "Result: <" << result.result_type_name << "> " << std::showbase << std::hex << run<int64_t>(result) << std::endl;
		
		return 0;
	}
	else
		return result.is_successful ? 0 : 1;
}

} // namespace catalyst::parser

using namespace catalyst;
using namespace catalyst::compiler;

int main(int argc, char **argv) {
	CLI::App app{"Catalyst Compiler"};
	cli_options options;

	std::stringstream version_string;
	version_string << app.get_description() << " " << compiler::version.string() << "(Catalyst "
				   << catalyst::version.string() << ")";
	app.set_version_flag("--version", version_string.str());

	app.add_option("input", options.input, "The Catalyst file to be parsed.")
		->required()
		->check(CLI::ExistingFile);

	//app.add_flag("--dump-ast", options.dump_ast, "Dump AST to stdout.");

	std::map<std::string, cli_options::OutputFormat> output_type_map{
		{"ascii", cli_options::OutputFormat::ASCII},
		{"color", cli_options::OutputFormat::Color},
		{"fancy", cli_options::OutputFormat::Fancy}};
	app.add_option("--format", options.format, "Set the output format")
		->transform(CLI::CheckedTransformer(output_type_map, CLI::ignore_case));

	app.add_option("-O,--optimize", options.compiler_options.optimizer_level, "Optimizer level");
	
	app.add_option("-R,--run", options.run, "Run");

	app.add_option("-o,--output", options.output_file, "Output file");

	app.add_option("--arch", options.arch, "Architecture triple");

	std::atexit([]() { std::cout << rang::style::reset; });
#if CATALYST_PLATFORM_POSIX
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = signal_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, nullptr);
#endif
	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError &e) {
		std::cout << (e.get_exit_code() == 0 ? rang::fg::blue : rang::fg::red);
		return app.exit(e);
	}

	return main(options);
}
