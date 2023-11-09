#include "object_type_reference.hpp"
#include "../codegen.hpp"

namespace catalyst::compiler::codegen::object_type_reference_detail {

std::string get_name_for_type(catalyst::compiler::codegen::state &state, std::shared_ptr<type_custom> type) {
	auto it = std::find_if(std::begin(state.symbol_table), std::end(state.symbol_table),
			[&type](auto&& p) { return *(p.second.type) == *type; });

	if (it == std::end(state.symbol_table)) {
		state.report_message(report_type::error, "Incomplete type " + type->name, nullptr);
		return "";
	}

	return it->first;
}

std::shared_ptr<type> get_type_for_name(catalyst::compiler::codegen::state &state, const std::string& name) {
	if (name == "<unknown>") {
		return type_class::unknown();
	}
	return state.symbol_table[name].type;
}

}