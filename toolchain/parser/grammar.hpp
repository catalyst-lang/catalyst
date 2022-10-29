#pragma once

#include <lexy/code_point.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <string>

#include "ast.hpp"

namespace catalyst::parser::grammar {
namespace dsl = lexy::dsl;

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
constexpr auto kw_where = LEXY_KEYWORD("where", id);
constexpr auto kw_while = LEXY_KEYWORD("while", id);
constexpr auto kw_async = LEXY_KEYWORD("async", id);
constexpr auto kw_await = LEXY_KEYWORD("await", id);
constexpr auto kw_dyn = LEXY_KEYWORD("dyn", id);

struct ident {
	static constexpr auto name = "identifier";

	static constexpr auto rule = [] {
		return id.reserve(kw_as, kw_break, kw_const, kw_continue, kw_crate, kw_else, kw_enum,
		                  kw_extern, kw_false, kw_fn, kw_for, kw_if, kw_impl, kw_in, kw_let,
		                  kw_loop, kw_match, kw_mod, kw_move, kw_mut, kw_pub, kw_ref, kw_return,
		                  kw_self, kw_Self, kw_static, kw_struct, kw_super, kw_trait, kw_true,
		                  kw_type, kw_unsafe, kw_use, kw_where, kw_while, kw_async, kw_await,
		                  kw_dyn);
	}();

	static constexpr auto value = lexy::callback<ast::ident>([](auto lex) {
		return ast::ident{
		    std::string(lex.begin(), lex.end()), (parser::char_type *)lex.begin(),
		    (parser::char_type *)lex.end()
		    //&lex
		};
	});
};

struct decl {
	static constexpr auto name = "declaration";
	static constexpr auto rule = [] {
		return kw_fn + dsl::p<ident>;
		// return LEXY_LIT("let") >> dsl::identifier(lead_char, trailing_char);
		//  static constexpr auto value = lexy::forward<;
	}();
	static constexpr auto value = lexy::construct<ast::decl>;
};

// Entry point of the production.
struct translation_unit {

	static constexpr auto whitespace = dsl::ascii::space |
	                                   LEXY_LIT("//") >> dsl::until(dsl::newline) |
	                                   LEXY_LIT("/*") >> dsl::until(LEXY_LIT("*/"));

	static constexpr auto rule = dsl::terminator(dsl::eof).list(dsl::p<decl>);
	static constexpr auto value =
	    lexy::as_list<std::vector<ast::decl>> >> lexy::construct<ast::translation_unit>;
};
} // namespace catalyst::parser::grammar
