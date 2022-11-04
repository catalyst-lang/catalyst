#pragma once

#include <lexy/callback.hpp>
#include <lexy/code_point.hpp>
#include <lexy/dsl.hpp>
#include <string>

#include "catalyst/ast.hpp"

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
constexpr auto kw_pub = LEXY_KEYWORD("pub", id);
constexpr auto kw_ref = LEXY_KEYWORD("ref", id);
constexpr auto kw_return = LEXY_KEYWORD("return", id);
constexpr auto kw_self = LEXY_KEYWORD("self", id);
constexpr auto kw_Self = LEXY_KEYWORD("Self", id);
constexpr auto kw_static = LEXY_KEYWORD("static", id);
constexpr auto kw_struct = LEXY_KEYWORD("struct", id);
constexpr auto kw_super = LEXY_KEYWORD("super", id);
constexpr auto kw_trait = LEXY_KEYWORD("trait", id);
constexpr auto kw_true = LEXY_KEYWORD("true", id);
constexpr auto kw_type = LEXY_KEYWORD("type", id);
constexpr auto kw_unsafe = LEXY_KEYWORD("unsafe", id);
constexpr auto kw_use = LEXY_KEYWORD("use", id);
constexpr auto kw_var = LEXY_KEYWORD("var", id);
constexpr auto kw_where = LEXY_KEYWORD("where", id);
constexpr auto kw_while = LEXY_KEYWORD("while", id);
constexpr auto kw_async = LEXY_KEYWORD("async", id);
constexpr auto kw_await = LEXY_KEYWORD("await", id);
constexpr auto kw_yield = LEXY_KEYWORD("yield", id);

struct ident : lexy::token_production {
	static constexpr auto name = "identifier";

	static constexpr auto rule = [] {
		return id.reserve(kw_as, kw_break, kw_const, kw_continue, kw_crate, kw_else, kw_enum,
		                  kw_extern, kw_false, kw_fn, kw_for, kw_if, kw_impl, kw_in, kw_let,
		                  kw_loop, kw_match, kw_mod, kw_move, kw_mut, kw_pub, kw_ref, kw_return,
		                  kw_self, kw_Self, kw_static, kw_struct, kw_super, kw_trait, kw_true,
		                  kw_type, kw_unsafe, kw_use, kw_var, kw_where, kw_while, kw_async,
		                  kw_await, kw_yield) |
		       dsl::error<expected_identifier>;
	}();

	static constexpr auto value = lexy::callback<ast::ident>([](auto lex) {
		return ast::ident(
			(parser::char_type *)lex.begin(), 
			(parser::char_type *)lex.end(),
			std::string(lex.begin(), lex.end())
		);
	});
};

struct type : lexy::token_production {
	static constexpr auto rule = dsl::position + dsl::p<ident> + dsl::position;
	static constexpr auto value = lexy::callback<ast::type>([](auto begin, auto ident, auto end) {
		return ast::type((parser::char_type*) begin, (parser::char_type*)end, ident);
	});
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

struct expr_literal_numeric {
	template <typename Base>
	static constexpr auto
		integer = dsl::integer<std::int64_t>(dsl::digits<Base>.sep(dsl::digit_sep_tick));

	// clang-format off
	static constexpr auto suffixes =
		lexy::symbol_table<ast::numeric_classifier>
			.map<'u'>(ast::numeric_classifier::unsigned_)
			.map<'i'>(ast::numeric_classifier::signed_)
			.map<'s'>(ast::numeric_classifier::signed_)
			.map<LEXY_SYMBOL("i8")>(ast::numeric_classifier::signed8)
			.map<LEXY_SYMBOL("u8")>(ast::numeric_classifier::unsigned8)
			.map<LEXY_SYMBOL("i16")>(ast::numeric_classifier::signed16)
			.map<LEXY_SYMBOL("u16")>(ast::numeric_classifier::unsigned16)
			.map<LEXY_SYMBOL("i32")>(ast::numeric_classifier::signed32)
			.map<LEXY_SYMBOL("u32")>(ast::numeric_classifier::unsigned32)
			.map<LEXY_SYMBOL("i64")>(ast::numeric_classifier::signed64)
			.map<LEXY_SYMBOL("u64")>(ast::numeric_classifier::unsigned64);
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
		auto octal_digits = integer<dsl::octal>;
		auto hex_digits = (LEXY_LIT("0x") | LEXY_LIT("0X")) >> integer<dsl::hex>;
		auto binary_digits = (LEXY_LIT("0b") | LEXY_LIT("0B")) >> integer<dsl::binary>;

		auto opt_suffix = dsl::opt(dsl::symbol<suffixes>);

		return dsl::peek(dsl::p<sign> + dsl::digit<>) >>
		       dsl::position + dsl::p<sign> +
		           (dsl::peek(dsl::lit_c<'0'>) >>
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
		         suffix.value_or(ast::numeric_classifier::none)
				 );
		});
};

struct expr_literal {
	static constexpr auto name = "literal expression";
	static constexpr auto rule = dsl::p<expr_literal_bool> | dsl::p<expr_literal_numeric>;
	static constexpr auto value = lexy::forward<ast::expr_ptr>;
};

// An expression that is nested inside another expression.
struct nested_expr : lexy::transparent_production
{
    // We change the whitespace rule to allow newlines:
    // as it's nested, we can properly handle continuation lines.
    static constexpr auto whitespace = dsl::ascii::space;
    // The rule itself just recurses back to expression, but with the adjusted whitespace now.
    static constexpr auto rule = dsl::recurse<struct expr>;

    static constexpr auto value = lexy::forward<ast::expr_ptr>;
};

struct argument_list
{
    static constexpr auto whitespace = dsl::ascii::space;
    static constexpr auto rule = dsl::terminator(LEXY_LIT(")"))
                                     .opt_list(dsl::recurse<expr>, dsl::sep(dsl::comma));
    static constexpr auto value = lexy::as_list<std::vector<ast::expr_ptr>>;
};

struct expr : lexy::expression_production {
	static constexpr auto name = "expression";
	static constexpr auto atom = [] {
		auto paren_expr = dsl::parenthesized(dsl::p<nested_expr>);
		return paren_expr | dsl::p<expr_literal> | dsl::else_ >> dsl::p<expr_ident>;
	}();

	struct prec1 : dsl::postfix_op {
		static constexpr auto op = [] {
			auto item = dsl::lit_c<'('> >> dsl::p<argument_list>;
			return dsl::op(item);
		}();
		using operand = dsl::atom;
	};
	
	struct prec2 : dsl::infix_op_left {
		static constexpr auto op = dsl::op(dsl::lit_c<'.'>);
		using operand = prec1;
	};

	struct prec3 : dsl::prefix_op {
		static constexpr auto op = dsl::op<ast::expr_unary_arithmetic::negate>(dsl::lit_c<'-'>);
		using operand = prec2;
	};

	struct prec5 : dsl::infix_op_left {
		static constexpr auto op = dsl::op<ast::expr_binary_arithmetic::times>(dsl::lit_c<'*'>) /
		                           dsl::op<ast::expr_binary_arithmetic::div>(dsl::lit_c<'/'>);
		using operand = prec3;
	};

	struct prec6 : dsl::infix_op_left {
		static constexpr auto op = dsl::op<ast::expr_binary_arithmetic::plus>(dsl::lit_c<'+'>) /
		                           dsl::op<ast::expr_binary_arithmetic::minus>(dsl::lit_c<'-'>);
		using operand = prec5;
	};

	using operation = prec6;

	static constexpr auto value = lexy::callback<ast::expr_ptr>(
		// atoms
		lexy::forward<ast::expr_ptr>, lexy::new_<ast::expr_literal, ast::expr_ptr>,
		lexy::new_<ast::expr_ident, ast::expr_ptr>,
		// unary/binary operators
		lexy::new_<ast::expr_unary_arithmetic, ast::expr_ptr>,
		lexy::new_<ast::expr_binary_arithmetic, ast::expr_ptr>,
		// conditional and assignment

		[](auto lhs, auto pos, std::vector<ast::expr_ptr> params) {
			return std::make_shared<ast::expr_call>(lhs, params);
		},

		[](auto lhs, lexy::op<prec2::op>, auto rhs) {
			return std::make_shared<ast::expr_member_access>(lhs, rhs);
		}

		//lexy::new_<ast::expr_call, ast::expr_ptr>
	    // lexy::forward<ast::expr_if>,
	    // lexy::forward<ast::expr_assignment>
	);
};

// struct expr : lexy::transparent_production {
// 	static constexpr auto rule = LEXY_LIT("EXPR");
// 	static constexpr auto value = lexy::forward<void>;
// };

struct statement_sep : lexy::transparent_production {
	static constexpr auto rule = dsl::newline | dsl::semicolon;
	static constexpr auto value = lexy::forward<void>;
};

struct statement_var {
	static constexpr auto
		rule = kw_var >>
	           dsl::p<ident> +
	               dsl::opt(dsl::colon >> dsl::p<type>) + dsl::opt(dsl::equal_sign >> dsl::p<expr>);
	static constexpr auto value = lexy::construct<ast::statement_var>;
};

struct statement_const {
	static constexpr auto
		rule = kw_const >>
	           dsl::p<ident> +
	               dsl::opt(dsl::colon >> dsl::p<type>) + dsl::opt(dsl::equal_sign >> dsl::p<expr>);
	static constexpr auto value = lexy::construct<ast::statement_const>;
};

struct statement_expr {
	static constexpr auto rule = dsl::p<expr>;
	static constexpr auto value = lexy::construct<ast::statement_expr>;
};

struct statement : lexy::transparent_production {
	struct expected_nl_sc
    {
        static constexpr auto name = "expected newline or semicolon";
    };

	static constexpr auto name = "statement";
	static constexpr auto rule =
		(dsl::p<statement_var> | dsl::p<statement_const> | dsl::else_ >> dsl::p<statement_expr>) + dsl::peek(dsl::semicolon | dsl::newline).error<expected_nl_sc>;
	static constexpr auto value = lexy::construct<ast::statement>;
};

// struct statements {
// 	struct sep : lexy::transparent_production {
// 		static constexpr auto rule = dsl::while_one(dsl::p<statement_sep>);
// 		static constexpr auto value = lexy::forward<void>;
// 	};
// 	struct opt_sep : lexy::transparent_production {
// 		static constexpr auto rule = dsl::opt(dsl::p<sep>);
// 		static constexpr auto value = lexy::forward<void>;
// 	};

// 	//static constexpr auto rule =
// 	//	dsl::p<opt_sep> + dsl::opt(dsl::list(dsl::p<statement>, dsl::trailing_sep(dsl::p<sep>)));

// 	static constexpr auto rule = []{
// 		auto item = dsl::p<statement>;
// 		auto sep = dsl::sep(dsl::while_one(dsl::ascii::newline | dsl::semicolon));
// 		return dsl::list(item, sep);
// 	}();

// 	static constexpr auto value = lexy::as_list<std::vector<ast::statement>>;
// };

// struct nested_value {
// 	static constexpr auto whitespace = dsl::ascii::space;
// 	static constexpr auto rule = dsl::recurse<statement>;
// 	static constexpr auto value = lexy::forward<ast::statement>;
// };

struct fn_body_block {
	static constexpr auto rule = [] {
		// dsl::curly_bracketed(dsl::p<statements>);
		auto item = dsl::recurse<statement>;
		auto sep = dsl::trailing_sep(dsl::while_one(dsl::semicolon | dsl::newline));
		auto bracketed =
			dsl::brackets(dsl::lit_c<'{'> >> dsl::whitespace(dsl::ascii::space), dsl::lit_c<'}'>);
		return bracketed.opt_list(item, sep);
	}();
	static constexpr auto value =
		lexy::as_list<std::vector<ast::statement>> >>
		lexy::callback<ast::fn_body_block>(lexy::construct<ast::fn_body_block>,
	                                       [](lexy::nullopt &&) { return ast::fn_body_block{}; });
};

struct fn_body_expr {
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
	static constexpr auto rule =
		dsl::position + dsl::p<ident> + dsl::opt(dsl::colon >> dsl::p<type>) + dsl::position;
	static constexpr auto value =
		lexy::callback<ast::fn_parameter>([](auto begin, auto ident, auto type, auto end) {
			return ast::fn_parameter { {begin, end}, ident, type };
		});
};

struct parameter_list {
	static constexpr auto whitespace = whitespace_incl_nl;
	static constexpr auto rule =
		dsl::round_bracketed.opt_list(dsl::p<fn_parameter>, dsl::sep(dsl::comma));
	static constexpr auto value = lexy::as_list<std::vector<ast::fn_parameter>>;
};

struct decl_fn {
	static constexpr auto name = "function declaration";

	static constexpr auto rule = dsl::position + kw_fn + dsl::p<ident> + dsl::p<parameter_list> +
	                             dsl::position + dsl::p<fn_body>;
	// static constexpr auto value = lexy::construct<ast::decl_fn>;
	static constexpr auto value = lexy::callback<ast::decl_ptr>(
		[](auto begin, auto ident, auto parameter_list, auto end, auto body) {
			return std::make_shared<ast::decl_fn>(begin, end, ident, parameter_list, body);
		});
};

struct decl_sep : lexy::transparent_production {
	static constexpr auto rule = dsl::opt(dsl::while_one(dsl::newline));
	static constexpr auto value = lexy::forward<void>;
};

struct decl : lexy::transparent_production {
	static constexpr auto name = "declaration";
	static constexpr auto rule = dsl::p<decl_sep> + dsl::p<decl_fn>;
	static constexpr auto value = lexy::forward<ast::decl_ptr>;
};

// Entry point of the production.
struct translation_unit {
	static constexpr auto whitespace = whitespace_normal;

	static constexpr auto rule = dsl::terminator(dsl::eof).list(
		dsl::p<decl>, dsl::trailing_sep(dsl::while_one(dsl::newline)));
	static constexpr auto value =
		lexy::as_list<std::vector<ast::decl_ptr>> >> lexy::construct<ast::translation_unit>;
};
} // namespace catalyst::parser::grammar
