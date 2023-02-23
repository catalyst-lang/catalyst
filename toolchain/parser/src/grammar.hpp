#pragma once

#include <lexy/callback.hpp>
#include <lexy/code_point.hpp>
#include <lexy/dsl.hpp>
#include <string>

#include "catalyst/ast/ast.hpp"

namespace catalyst::parser::grammar {
namespace dsl = lexy::dsl;

constexpr auto whitespace_normal = dsl::ascii::blank | LEXY_LIT("//") >> dsl::until(dsl::newline) |
                                   LEXY_LIT("/*") >> dsl::until(LEXY_LIT("*/"));
constexpr auto whitespace_incl_nl = dsl::newline | whitespace_normal;

struct expected_identifier {
	static constexpr auto name = "expected identifier";
};

constexpr auto id = dsl::identifier(dsl::unicode::xid_start_underscore, dsl::unicode::xid_continue);
constexpr auto kw_as = LEXY_KEYWORD("as", id);
constexpr auto kw_break = LEXY_KEYWORD("break", id);
constexpr auto kw_const = LEXY_KEYWORD("const", id);
constexpr auto kw_continue = LEXY_KEYWORD("continue", id);
constexpr auto kw_crate = LEXY_KEYWORD("crate", id);
constexpr auto kw_else = LEXY_KEYWORD("else", id);
constexpr auto kw_enum = LEXY_KEYWORD("enum", id);
constexpr auto kw_extern = LEXY_KEYWORD("extern", id);
constexpr auto kw_false = LEXY_KEYWORD("false", id);
constexpr auto kw_fn = LEXY_KEYWORD("fn", id);
constexpr auto kw_for = LEXY_KEYWORD("for", id);
constexpr auto kw_if = LEXY_KEYWORD("if", id);
constexpr auto kw_impl = LEXY_KEYWORD("impl", id);
constexpr auto kw_in = LEXY_KEYWORD("in", id);
constexpr auto kw_let = LEXY_KEYWORD("let", id);
constexpr auto kw_loop = LEXY_KEYWORD("loop", id);
constexpr auto kw_match = LEXY_KEYWORD("match", id);
constexpr auto kw_mod = LEXY_KEYWORD("mod", id);
constexpr auto kw_move = LEXY_KEYWORD("move", id);
constexpr auto kw_mut = LEXY_KEYWORD("mut", id);
constexpr auto kw_ns = LEXY_KEYWORD("ns", id);
constexpr auto kw_pub = LEXY_KEYWORD("pub", id);
constexpr auto kw_ref = LEXY_KEYWORD("ref", id);
constexpr auto kw_return = LEXY_KEYWORD("return", id);
constexpr auto kw_self = LEXY_KEYWORD("self", id);
constexpr auto kw_Self = LEXY_KEYWORD("Self", id);
constexpr auto kw_static = LEXY_KEYWORD("static", id);
constexpr auto kw_struct = LEXY_KEYWORD("struct", id);
constexpr auto kw_class = LEXY_KEYWORD("class", id);
constexpr auto kw_super = LEXY_KEYWORD("super", id);
constexpr auto kw_trait = LEXY_KEYWORD("trait", id);
constexpr auto kw_true = LEXY_KEYWORD("true", id);
constexpr auto kw_type = LEXY_KEYWORD("type", id);
constexpr auto kw_unsafe = LEXY_KEYWORD("unsafe", id);
constexpr auto kw_use = LEXY_KEYWORD("use", id);
constexpr auto kw_var = LEXY_KEYWORD("var", id);
constexpr auto kw_virtual = LEXY_KEYWORD("virtual", id);
constexpr auto kw_where = LEXY_KEYWORD("where", id);
constexpr auto kw_while = LEXY_KEYWORD("while", id);
constexpr auto kw_async = LEXY_KEYWORD("async", id);
constexpr auto kw_await = LEXY_KEYWORD("await", id);
constexpr auto kw_yield = LEXY_KEYWORD("yield", id);

struct expected_nl_sc {
	static constexpr auto name = "expected newline or semicolon";
};

struct ident : lexy::token_production {
	static constexpr auto name = "identifier";

	static constexpr auto rule = [] {
		return id.reserve(kw_as, kw_break, kw_const, kw_continue, kw_crate, kw_else, kw_enum,
		                  kw_extern, kw_false, kw_fn, kw_for, kw_if, kw_impl, kw_in, kw_let,
		                  kw_loop, kw_match, kw_mod, kw_move, kw_mut, kw_ns, kw_pub, kw_ref,
		                  kw_return, kw_self, kw_Self, kw_static, kw_struct, kw_class, kw_super,
		                  kw_trait, kw_true, kw_type, kw_unsafe, kw_use, kw_var, kw_virtual,
		                  kw_where, kw_while, kw_async, kw_await, kw_yield) |
		       dsl::error<expected_identifier>;
	}();

	static constexpr auto value = lexy::callback<ast::ident>([](auto lex) {
		return ast::ident((parser::char_type *)lex.begin(), (parser::char_type *)lex.end(),
		                  std::string(lex.begin(), lex.end()));
	});
};

struct type;

struct type_qualified_name_list : lexy::transparent_production {
	static constexpr auto rule = dsl::list(dsl::p<ident>, dsl::sep(dsl::period));
	static constexpr auto value = lexy::as_list<std::vector<ast::ident>>;
};

struct type_qualified_name : lexy::token_production {
	static constexpr auto name = "qualified type name";
	static constexpr auto rule =
		dsl::position + dsl::p<type_qualified_name_list> + dsl::position;
	static constexpr auto value =
		lexy::callback<ast::type_ptr>([](auto begin, auto idents, auto end) {
			return std::make_shared<ast::type_qualified_name>((parser::char_type *)begin,
		                                                      (parser::char_type *)end, idents);
		});
};

struct type_function_parameter : lexy::transparent_production {
	static constexpr auto rule =
		dsl::position +
		dsl::opt(dsl::peek(dsl::p<ident> + dsl::colon) >> dsl::p<ident> + dsl::colon) +
		dsl::recurse<type> + dsl::position;
	static constexpr auto value = lexy::callback<ast::type_function_parameter>(
		[](auto begin, auto ident, auto type, auto end) {
			return ast::type_function_parameter{{begin, end}, ident, type};
		});
};

struct type_function_parameter_list : lexy::transparent_production {
	static constexpr auto whitespace = whitespace_incl_nl;
	static constexpr auto rule =
		dsl::round_bracketed.opt_list(dsl::p<type_function_parameter>, dsl::sep(dsl::comma));
	static constexpr auto value = lexy::as_list<std::vector<ast::type_function_parameter>>;
};

struct type_function : lexy::token_production {
	static constexpr auto name = "function type";
	static constexpr auto rule = dsl::peek(dsl::lit_c<'('>) >>
	                             dsl::position + dsl::p<type_function_parameter_list> +
	                                 (LEXY_LIT("->") >> dsl::recurse<type>)+dsl::position;
	static constexpr auto value =
		lexy::callback<ast::type_ptr>([](auto begin, auto params, auto ret_type, auto end) {
			return std::make_shared<ast::type_function>((parser::char_type *)begin,
		                                                (parser::char_type *)end, params, ret_type);
		});
};

struct type {
	static constexpr auto whitespace = whitespace_normal;
	static constexpr auto name = "typename";
	static constexpr auto rule =
		(dsl::p<type_function> | dsl::else_ >> dsl::p<type_qualified_name>);
	static constexpr auto value = lexy::forward<ast::type_ptr>;
};

struct expr_ident : lexy::token_production {
	static constexpr auto name = "identifier expression";
	static constexpr auto rule = dsl::p<ident>;
	static constexpr auto value = lexy::construct<ast::expr_ident>;
};

struct expr_literal_bool : lexy::token_production {
	struct true_ : lexy::transparent_production {
		static constexpr auto rule = kw_true;
		static constexpr auto value = lexy::constant(true);
	};
	struct false_ : lexy::transparent_production {
		static constexpr auto rule = kw_false;
		static constexpr auto value = lexy::constant(false);
	};

	static constexpr auto name = "boolean";
	static constexpr auto rule = dsl::peek(kw_true | kw_false) >>
	                             dsl::position + (dsl::p<true_> | dsl::p<false_>)+dsl::position;
	static constexpr auto value =
		lexy::callback<ast::expr_ptr>([](auto begin, auto value, auto end) {
			return std::make_shared<ast::expr_literal_bool>(begin, end, value);
		});
};

struct expr_literal_numeric : lexy::token_production {
	template <typename Base>
	static constexpr auto integer =
		dsl::integer<std::int64_t>(dsl::digits<Base>.sep(dsl::digit_sep_tick));

	// clang-format off
	static constexpr auto suffixes =
		lexy::symbol_table<ast::numeric_classifier>
			.map<'u'>(ast::numeric_classifier::unsigned_)
			.map<'i'>(ast::numeric_classifier::signed_)
			.map<'s'>(ast::numeric_classifier::signed_)
			.map<LEXY_SYMBOL("z")>(ast::numeric_classifier::size)
			.map<LEXY_SYMBOL("i8")>(ast::numeric_classifier::signed8)
			.map<LEXY_SYMBOL("u8")>(ast::numeric_classifier::unsigned8)
			.map<LEXY_SYMBOL("i16")>(ast::numeric_classifier::signed16)
			.map<LEXY_SYMBOL("u16")>(ast::numeric_classifier::unsigned16)
			.map<LEXY_SYMBOL("i32")>(ast::numeric_classifier::signed32)
			.map<LEXY_SYMBOL("u32")>(ast::numeric_classifier::unsigned32)
			.map<LEXY_SYMBOL("i64")>(ast::numeric_classifier::signed64)
			.map<LEXY_SYMBOL("u64")>(ast::numeric_classifier::signed64)
			.map<LEXY_SYMBOL("i128")>(ast::numeric_classifier::signed128)
			.map<LEXY_SYMBOL("u128")>(ast::numeric_classifier::unsigned128)
			.map<LEXY_SYMBOL("f")>(ast::numeric_classifier::float_)
			.map<LEXY_SYMBOL("f16")>(ast::numeric_classifier::float16)
			.map<LEXY_SYMBOL("f32")>(ast::numeric_classifier::float32)
			.map<LEXY_SYMBOL("f64")>(ast::numeric_classifier::float64)
			.map<LEXY_SYMBOL("f128")>(ast::numeric_classifier::float128)
			.map<LEXY_SYMBOL("f80")>(ast::numeric_classifier::float80);
	// clang-format on

	struct sign : lexy::transparent_production {
		struct plus : lexy::transparent_production {
			static constexpr auto rule = dsl::lit_c<'+'>;
			static constexpr auto value = lexy::constant(+1);
		};
		struct minus : lexy::transparent_production {
			static constexpr auto rule = dsl::lit_c<'-'>;
			static constexpr auto value = lexy::constant(-1);
		};

		// static constexpr auto rule = dsl::peek(dsl::p<plus> | dsl::p<minus>) >>
		//                              (dsl::p<plus> | dsl::p<minus>);
		static constexpr auto rule = dsl::opt(dsl::p<plus> | dsl::p<minus>);
		static constexpr auto value =
			lexy::callback<int>([](std::optional<int> value) { return value.value_or(1); });
	};

	struct exponent : lexy::transparent_production {
		static constexpr auto rule = [] {
			auto exp_char = dsl::lit_c<'e'> | dsl::lit_c<'E'>;
			return exp_char >> dsl::sign + dsl::integer<std::int16_t>;
		}();
		static constexpr auto value = lexy::as_integer<std::int16_t>;
	};

	static constexpr auto name = "numeric";
	static constexpr auto rule = [] {
		auto decimal_digits = integer<dsl::decimal>;
		auto octal_digits = (LEXY_LIT("0o") | LEXY_LIT("0O")) >> integer<dsl::octal>;
		auto hex_digits = (LEXY_LIT("0x") | LEXY_LIT("0X")) >> integer<dsl::hex>;
		auto binary_digits = (LEXY_LIT("0b") | LEXY_LIT("0B")) >> integer<dsl::binary>;

		auto opt_suffix = dsl::opt(dsl::symbol<suffixes>);

		return dsl::peek(dsl::p<sign> + dsl::digit<>) >>
		       dsl::position + dsl::p<sign> +
		           (dsl::peek(dsl::lit_c<'0'> + LEXY_ASCII_ONE_OF("oOxXbB")) >>
		                ((hex_digits | binary_digits | octal_digits) + dsl::nullopt + dsl::nullopt +
		                 opt_suffix + dsl::position) |
		            dsl::else_ >>
		                (decimal_digits +
		                 dsl::opt(dsl::peek(dsl::period) >> dsl::period + decimal_digits) +
		                 dsl::opt(dsl::p<exponent>) + opt_suffix + dsl::position));
	}();

	static constexpr auto value = lexy::callback<ast::expr_ptr>(
		[](auto begin, auto sign, auto value, std::optional<int64_t> fraction,
	       std::optional<int16_t> exponent, std::optional<ast::numeric_classifier> suffix,
	       auto end) {
			return std::make_shared<ast::expr_literal_numeric>(
				begin, end, sign, value, fraction, exponent,
				suffix.value_or(ast::numeric_classifier::none));
		});
};

struct expr_literal {
	static constexpr auto name = "literal expression";
	static constexpr auto rule = dsl::p<expr_literal_bool> | dsl::p<expr_literal_numeric>;
	static constexpr auto value = lexy::forward<ast::expr_ptr>;
};

// An expression that is nested inside another expression.
struct nested_expr : lexy::transparent_production {
	// We change the whitespace rule to allow newlines:
	// as it's nested, we can properly handle continuation lines.
	static constexpr auto whitespace = dsl::ascii::space;
	// The rule itself just recurses back to expression, but with the adjusted whitespace now.
	static constexpr auto rule = dsl::recurse<struct expr>;

	static constexpr auto value = lexy::forward<ast::expr_ptr>;
};

struct expr_with_whitespace : lexy::transparent_production {
	static constexpr auto whitespace = whitespace_incl_nl;
	static constexpr auto rule = dsl::recurse<expr>;
	static constexpr auto value = lexy::forward<ast::expr_ptr>;
};

struct argument_list {
	static constexpr auto name = "argument list";
	static constexpr auto rule =
		dsl::terminator(dsl::lit_c<')'>)
			.opt_list(dsl::recurse<expr_with_whitespace>, dsl::sep(dsl::comma));

	static constexpr auto value = lexy::as_list<std::vector<ast::expr_ptr>>;
};

struct expr : lexy::expression_production {
	static constexpr auto name = "expression";
	static constexpr auto atom = [] {
		auto paren_expr = dsl::parenthesized(dsl::p<nested_expr>);
		return paren_expr | dsl::p<expr_literal> | dsl::else_ >> dsl::p<expr_ident>;
	}();

	struct op_call : dsl::postfix_op {
		static constexpr auto op = [] {
			auto item = dsl::lit_c<'('> >> dsl::p<argument_list>;
			return dsl::op(item);
		}();
		using operand = dsl::atom;
	};

	struct op_member_access : dsl::infix_op_left {
		static constexpr auto op = dsl::op(dsl::lit_c<'.'>);
		using operand = op_call;
	};

	struct prec3 : dsl::prefix_op {
		static constexpr auto op = dsl::op<ast::expr_unary_arithmetic::negate>(dsl::lit_c<'-'>);
		using operand = op_member_access;
	};

	struct op_cast : dsl::postfix_op {
		static constexpr auto op = dsl::op(kw_as >> dsl::p<type>);
		using operand = prec3;
	};

	struct prec5 : dsl::infix_op_left {
		static constexpr auto op = dsl::op<ast::expr_binary_arithmetic::times>(dsl::lit_c<'*'>) /
		                           dsl::op<ast::expr_binary_arithmetic::div>(dsl::lit_c<'/'>);
		using operand = op_cast;
	};

	struct prec6 : dsl::infix_op_left {
		static constexpr auto op = dsl::op<ast::expr_binary_arithmetic::plus>(dsl::lit_c<'+'>) /
		                           dsl::op<ast::expr_binary_arithmetic::minus>(dsl::lit_c<'-'>);
		using operand = prec5;
	};

	struct assignment : dsl::infix_op_right {
		static constexpr auto op = dsl::op<ast::expr_assignment::assign>(dsl::lit_c<'='>);
		using operand = prec6;
	};

	using operation = assignment;

	static constexpr auto value = lexy::callback<ast::expr_ptr>(
		// atoms
		lexy::forward<ast::expr_ptr>, lexy::new_<ast::expr_literal, ast::expr_ptr>,
		lexy::new_<ast::expr_ident, ast::expr_ptr>,
		// unary/binary operators
		lexy::new_<ast::expr_unary_arithmetic, ast::expr_ptr>,
		lexy::new_<ast::expr_binary_arithmetic, ast::expr_ptr>,
		// conditional and assignment
		lexy::new_<ast::expr_assignment, ast::expr_ptr>,

		[](auto lhs, lexy::op<op_cast::op>, auto type) {
			return std::make_shared<ast::expr_cast>(lhs->lexeme.end, type->lexeme.begin, lhs, type);
		},

		// call
		[](auto lhs, const lexy::op<op_call::op>, std::vector<ast::expr_ptr> params) {
			return std::make_shared<ast::expr_call>(lhs->lexeme.begin, lhs->lexeme.end, lhs,
		                                            params);
		},

		[](auto lhs, const lexy::op<op_member_access::op>, auto rhs) {
			return std::make_shared<ast::expr_member_access>(lhs->lexeme.end, lhs, rhs);
		}

		// lexy::new_<ast::expr_call, ast::expr_ptr>
	    //  lexy::forward<ast::expr_if>,
	    //  lexy::forward<ast::expr_assignment>
	);
};

struct statement_return {
	static constexpr auto name = "return statement";
	static constexpr auto
		rule = dsl::peek(kw_return) >> dsl::position + kw_return >>
	           (dsl::peek(dsl::semicolon | dsl::newline | dsl::lit_c<'}'>).error<expected_nl_sc> |
	            dsl::else_ >> dsl::p<expr>)+dsl::position;

	static constexpr auto value = lexy::callback<ast::statement_ptr>(
		[](auto begin, auto end) {
			return std::make_shared<ast::statement_return>(begin, end, std::nullopt);
		},
		[](auto begin, ast::expr_ptr expr, auto end) {
			return std::make_shared<ast::statement_return>(begin, end, expr);
		});
};

struct statement_expr {
	static constexpr auto name = "expression statement";
	static constexpr auto rule = dsl::p<expr>;
	static constexpr auto value = lexy::callback<ast::statement_ptr>(
		[](auto expr) { return std::make_shared<ast::statement_expr>(expr); });
};

struct decl;

struct statement_decl {
	static constexpr auto name = "declaration statement";
	static constexpr auto rule = dsl::recurse_branch<decl>;
	static constexpr auto value = lexy::callback<ast::statement_ptr>(
		[](auto decl) { return std::make_shared<ast::statement_decl>(decl); });
};

// TODO: if statement is now always a block, this should optionally be a single statement
struct statement_if {
	static constexpr auto name = "if statement";
	static constexpr auto rule = kw_if >> dsl::p<expr> + dsl::position +
	                                          dsl::recurse<struct statement_block> +
	                                          dsl::opt(kw_else >> dsl::recurse<struct statement>);
	static constexpr auto value = lexy::callback<ast::statement_ptr>(
		[](auto cond, auto end, auto then, lexy::nullopt &&else_) {
			return std::make_shared<ast::statement_if>(nullptr, end, cond, then, std::nullopt);
		},
		[](auto cond, auto end, auto then, auto else_) {
			return std::make_shared<ast::statement_if>(nullptr, end, cond, then, else_);
		});
};

struct statement_for {
	static constexpr auto name = "for statement";
	static constexpr auto rule = kw_for >> dsl::p<ident> + kw_in + dsl::p<expr> + LEXY_LIT("..") >>
	                             dsl::p<expr> + dsl::position + dsl::recurse<struct statement>;
	static constexpr auto value = lexy::callback<ast::statement_ptr>(
		[](auto ident, auto expr_start, auto expr_end, auto end, auto body) {
			return std::make_shared<ast::statement_for>(nullptr, end, ident, expr_start, expr_end,
		                                                body);
		});
};

struct statement_block {
	static constexpr auto name = "block";
	static constexpr auto rule = [] {
		auto item = dsl::recurse<statement>;
		auto sep = dsl::trailing_sep(dsl::while_one(dsl::semicolon | dsl::newline));
		auto bracketed =
			dsl::brackets(dsl::lit_c<'{'> >> dsl::whitespace(whitespace_incl_nl), dsl::lit_c<'}'>);
		return bracketed.opt_list(item, sep);
	}();
	static constexpr auto
		value = lexy::as_list<std::vector<ast::statement_ptr>> >>
	            lexy::callback<ast::statement_ptr>(
					[](auto list) {
						return std::make_shared<ast::statement_block>(nullptr, nullptr, list);
					},
					[](lexy::nullopt &&) {
						std::vector<ast::statement_ptr> empty;
						return std::make_shared<ast::statement_block>(nullptr, nullptr, empty);
					});
};

struct statement : lexy::transparent_production {
	static constexpr auto name = "statement";
	static constexpr auto rule =
		dsl::position +
		(dsl::p<statement_decl> | dsl::p<statement_return> | dsl::p<statement_if> |
	     dsl::p<statement_for> | dsl::p<statement_block> |
	     dsl::else_ >> dsl::p<statement_expr>)+dsl::position +
		dsl::peek(dsl::semicolon | dsl::newline | dsl::lit_c<'}'>).error<expected_nl_sc>;
	// static constexpr auto value = lexy::forward<ast::statement_ptr>;
	static constexpr auto value =
		lexy::callback<ast::statement_ptr>([](auto begin, ast::statement_ptr stmt, auto end) {
			if (stmt->lexeme.begin == nullptr)
				stmt->lexeme.begin = begin;
			if (stmt->lexeme.end == nullptr)
				stmt->lexeme.end = end;
			return stmt;
		});
};

struct fn_body_block {
	static constexpr auto name = "in function body";
	static constexpr auto rule = [] {
		auto item = dsl::recurse<statement>;
		auto sep = dsl::trailing_sep(dsl::while_one(dsl::semicolon | dsl::newline));
		auto bracketed =
			dsl::brackets(dsl::lit_c<'{'> >> dsl::whitespace(whitespace_incl_nl), dsl::lit_c<'}'>);
		return bracketed.opt_list(item, sep);
	}();
	static constexpr auto
		value = lexy::as_list<std::vector<ast::statement_ptr>> >>
	            lexy::callback<ast::fn_body_block>(
					[](auto list) { return ast::fn_body_block(nullptr, nullptr, list); },
					[](lexy::nullopt &&) { return ast::fn_body_block(nullptr, nullptr, {}); });
};

struct fn_body_expr {
	static constexpr auto name = "function expression";
	static constexpr auto rule = LEXY_LIT("=>") >> dsl::p<expr>;
	static constexpr auto value = lexy::construct<ast::fn_body_expr>;
};

struct fn_body : lexy::transparent_production {
	struct expected_fn_body {
		static constexpr auto name = "expected function body";
	};
	static constexpr auto name = "function body";
	static constexpr auto rule =
		dsl::p<fn_body_block> | dsl::p<fn_body_expr> | dsl::error<expected_fn_body>;
	static constexpr auto value = lexy::construct<ast::fn_body>;
};

struct fn_parameter {
	static constexpr auto name = "parameter";
	static constexpr auto rule =
		dsl::position + dsl::p<ident> + dsl::opt(dsl::colon >> dsl::p<type>) + dsl::position;
	static constexpr auto value =
		lexy::callback<ast::fn_parameter>([](auto begin, auto ident, auto type, auto end) {
			return ast::fn_parameter{{begin, end}, ident, type};
		});
};

struct parameter_list {
	static constexpr auto name = "parameter list";
	static constexpr auto whitespace = whitespace_incl_nl;
	static constexpr auto rule =
		dsl::round_bracketed.opt_list(dsl::p<fn_parameter>, dsl::sep(dsl::comma));
	static constexpr auto value = lexy::as_list<std::vector<ast::fn_parameter>>;
};

struct decl_fn {
	static constexpr auto name = "function declaration";
	static constexpr auto rule = kw_fn >>
	                             dsl::position + dsl::p<ident> + dsl::p<parameter_list> +
	                                 dsl::opt(LEXY_LIT("->") >> dsl::p<type>) + dsl::position
	                                 + dsl::p<fn_body>;

	// static constexpr auto value = lexy::construct<ast::decl_fn>;
	static constexpr auto value = lexy::callback<ast::decl_ptr>(
		[](auto begin, auto ident, auto parameter_list, lexy::nullopt, auto end, auto body) {
			return std::make_shared<ast::decl_fn>(begin, end, ident, parameter_list, std::nullopt,
		                                          body);
		},
		[](auto begin, auto ident, auto parameter_list, auto type, auto end, auto body) {
			return std::make_shared<ast::decl_fn>(begin, end, ident, parameter_list, type, body);
		});
};

struct decl_var {
	static constexpr auto name = "variable declaration";
	static constexpr auto rule = kw_var >> dsl::p<ident> +
	                                           dsl::opt(dsl::colon >> dsl::p<type>) + dsl::position
	                                           + dsl::opt(dsl::equal_sign >> dsl::p<expr>);

	static constexpr auto value = lexy::callback<ast::decl_ptr>(
		[](auto ident, lexy::nullopt type, auto end, lexy::nullopt expr) {
			return std::make_shared<ast::decl_var>(ident.lexeme.begin, end, ident, std::nullopt,
		                                           std::nullopt);
		},
		[](auto ident, auto type, auto end, lexy::nullopt expr) {
			return std::make_shared<ast::decl_var>(ident.lexeme.begin, end, ident, type,
		                                           std::nullopt);
		},
		[](auto ident, lexy::nullopt type, auto end, auto expr) {
			return std::make_shared<ast::decl_var>(ident.lexeme.begin, end, ident, std::nullopt,
		                                           expr);
		},
		[](auto ident, auto type, auto end, auto expr) {
			return std::make_shared<ast::decl_var>(ident.lexeme.begin, end, ident, type, expr);
		});
};

// TODO: refactor this into decl_var
struct decl_const {
	static constexpr auto name = "constant declaration";
	static constexpr auto rule = kw_const >>
	                             dsl::p<ident> +
	                                 dsl::opt(dsl::colon >> dsl::p<type>) + dsl::position
	                                 + dsl::opt(dsl::equal_sign >> dsl::p<expr>);

	static constexpr auto value = lexy::callback<ast::decl_ptr>(
		[](auto ident, lexy::nullopt type, auto end, lexy::nullopt expr) {
			return std::make_shared<ast::decl_const>(nullptr, end, ident, std::nullopt,
		                                             std::nullopt);
		},
		[](auto ident, auto type, auto end, lexy::nullopt expr) {
			return std::make_shared<ast::decl_const>(nullptr, end, ident, type, std::nullopt);
		},
		[](auto ident, lexy::nullopt type, auto end, auto expr) {
			return std::make_shared<ast::decl_const>(nullptr, end, ident, std::nullopt, expr);
		},
		[](auto ident, auto type, auto end, auto expr) {
			return std::make_shared<ast::decl_const>(nullptr, end, ident, type, expr);
		});
};

struct decl_list_sep : lexy::transparent_production {
	static constexpr auto rule = dsl::opt(dsl::while_one(dsl::newline));
	static constexpr auto value = lexy::forward<void>;
};

struct decl_list {
	static constexpr auto name = "declarations";
	// static constexpr auto whitespace = whitespace_incl_nl;
	static constexpr auto rule = dsl::curly_bracketed.opt_list(
		dsl::p<decl_list_sep> + dsl::recurse<decl>,
		dsl::ignore_trailing_sep(dsl::while_one(dsl::newline | dsl::semicolon)));
	static constexpr auto value = lexy::as_list<std::vector<ast::decl_ptr>>;
};

struct decl_struct {
	static constexpr auto name = "struct declaration";
	static constexpr auto rule = kw_struct >> dsl::p<ident> + dsl::p<decl_list> + dsl::position;

	static constexpr auto value =
		lexy::callback<ast::decl_ptr>([](auto ident, auto decls, auto end) {
			return std::make_shared<ast::decl_struct>(ident.lexeme.begin, end, ident, decls);
		});
};

struct decl_class {
	static constexpr auto name = "class declaration";
	static constexpr auto
		rule = kw_class >>
	           dsl::p<ident> +
	               dsl::opt(dsl::colon >> dsl::p<type>) + dsl::p<decl_list> + dsl::position;

	static constexpr auto value = lexy::callback<ast::decl_ptr>(
		[](auto ident, lexy::nullopt, auto decls, auto end) {
			return std::make_shared<ast::decl_class>(ident.lexeme.begin, end, ident, std::nullopt,
		                                             decls);
		},
		[](auto ident, auto super, auto decls, auto end) {
			return std::make_shared<ast::decl_class>(ident.lexeme.begin, end, ident, super, decls);
		});
};

struct decl_ns {
	static constexpr auto name = "namespace declaration";
	static constexpr auto rule = kw_ns >>
	                             dsl::p<ident> + dsl::opt(dsl::p<decl_list>) + dsl::position;

	static constexpr auto value = lexy::callback<ast::decl_ptr>(
		[](auto ident, lexy::nullopt, auto end) {
			return std::make_shared<ast::decl_ns>(ident.lexeme.begin, end, ident,
		                                          std::vector<ast::decl_ptr>{}, true);
		},
		[](auto ident, auto decls, auto end) {
			return std::make_shared<ast::decl_ns>(ident.lexeme.begin, end, ident, decls);
		});
};

struct decl {
	static constexpr auto name = "declaration";
	static constexpr auto rule = (dsl::p<decl_fn> | dsl::p<decl_var> | dsl::p<decl_const> |
	                              dsl::p<decl_struct> | dsl::p<decl_class> | dsl::p<decl_ns>);
	static constexpr auto value = lexy::forward<ast::decl_ptr>;
};

struct decl_sep : lexy::transparent_production {
	static constexpr auto rule = dsl::opt(dsl::while_one(dsl::newline));
	static constexpr auto value = lexy::forward<void>;
};

// Entry point of the production.
struct translation_unit {
	static constexpr auto name = "the input";
	static constexpr auto whitespace = whitespace_normal;

	static constexpr auto rule = dsl::terminator(dsl::eof).list(
		(dsl::p<decl_sep> + dsl::p<decl>), dsl::trailing_sep(dsl::while_one(dsl::newline)));
	static constexpr auto value =
		lexy::as_list<std::vector<ast::decl_ptr>> >> lexy::construct<ast::translation_unit>;
};
} // namespace catalyst::parser::grammar
