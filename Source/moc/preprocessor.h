#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "symbols.h"
#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <stdio.h>

namespace header_tool
{
	struct Macro2
	{
		Macro2(bool isFunction = false, bool isVariadic = false) : isFunction(isFunction), isVariadic(isVariadic)
		{
		}
		bool isFunction;
		bool isVariadic;
		std::vector<Symbol> arguments;
		std::vector<Symbol> symbols;
	};

	struct IncludePath
	{
		inline explicit IncludePath(const std::string &_path, bool isFrameworkPath = false)
			: path(_path), isFrameworkPath(isFrameworkPath)
		{
		}
		std::string path;
		bool isFrameworkPath;
	};

	std::stack<std::string, std::vector<std::string>> g_include_file_stack;
	std::set<std::string> g_preprocessedIncludes;
	std::unordered_map<std::string, Macro2> g_macros;
	std::unordered_map<std::string, std::string> g_nonlocalIncludePathResolutionCache;
	std::vector<IncludePath> includes;

	#ifdef PLATFORM_WINDOWS
	#define ErrorFormatString "%s(%lld): "
	#else
	#define ErrorFormatString "%s:%ld: "
	#endif

	void error(const std::string& filename, uint64 line_num, const char *msg)
	{
		fprintf(stderr, ErrorFormatString "Error: %s\n", filename.c_str(), line_num, msg);
		exit(EXIT_FAILURE);
	}

	void error(const std::string& filename, const Symbol& symbol)
	{
		fprintf(stderr, ErrorFormatString "Parse error at \"%s\"\n", filename.c_str(), symbol.line_num, symbol.lexem().data());
		exit(EXIT_FAILURE);
	}

	inline static void expand_macro(std::vector<Symbol>& into, const Symbol& toExpand, uint64 lineNum, const std::set<std::string>& excludeSymbols);

	inline static std::vector<Symbol> expand_function_macro(Symbol symbol, SymbolStack &symbolstack, uint64 lineNum, Macro2 macro)
	{
		Symbol s = symbol;

		std::vector<Symbol> expansion;

		if (symbolstack.back().symbols[symbolstack.back().index].token != EToken::LPAREN)
		{
			// tokenized incorrectly
			assert(false);
		}

		std::vector<std::vector<Symbol>> arguments;
		arguments.reserve(5);
		while (symbolstack.has_next())
		{
			std::vector<Symbol> argument;
			// strip leading space
			while (symbolstack.back().symbols[symbolstack.back().index].token == EToken::WHITESPACE
				|| symbolstack.back().symbols[symbolstack.back().index].token == EToken::WHITESPACE_ALIAS)
			{
				symbolstack.back().index++;
			}
			int nesting = 0;
			bool vararg = macro.isVariadic && (arguments.size() == macro.arguments.size() - 1);
			while (symbolstack.has_next())
			{
				EToken t = symbolstack.next();
				if (t == EToken::LPAREN)
				{
					++nesting;
				}
				else if (t == EToken::RPAREN)
				{
					--nesting;
					if (nesting < 0)
						break;
				}
				else if (t == EToken::COMMA && nesting == 0)
				{
					if (!vararg)
						break;
				}
				argument.push_back(symbolstack.back().symbols[symbolstack.back().index]);
			}
			arguments.push_back(argument);

			if (nesting < 0)
				break;
			else if (!symbolstack.has_next())
				error(g_include_file_stack.top(), symbolstack.back().symbols[symbolstack.back().index].line_num, "missing ')' in macro usage");
		}

		// empty VA_ARGS
		if (macro.isVariadic && arguments.size() == macro.arguments.size() - 1)
		{
			// todo:
			// arguments += std::vector<Symbol>();
		}

		// now replace the macro arguments with the expanded arguments
		enum Mode
		{
			Normal,
			Hash,
			HashHash
		} mode = Normal;

		for (int i = 0; i < macro.symbols.size(); ++i)
		{
			const Symbol &s = macro.symbols[i];
			if (s.token == EToken::PREPROC_HASH || s.token == EToken::PREPROC_HASHHASH)
			{
				mode = (s.token == EToken::PREPROC_HASH ? Hash : HashHash);
				continue;
			}
			uint64 index = std::distance(macro.arguments.begin(), std::find(macro.arguments.begin(), macro.arguments.end(), s));
			if (mode == Normal)
			{
				if (index >= 0 && index < arguments.size())
				{
					// each argument undergoes macro expansion if it's not used as part of a # or ##
					if (i == macro.symbols.size() - 1 || macro.symbols.at(i + 1).token != EToken::PREPROC_HASHHASH)
					{
						std::vector<Symbol> args = arguments.at(index);
						uint64 idx = 1;

						std::set<std::string> set;
						for (int i = 0; i < symbolstack.size(); ++i)
						{
							set.insert(symbolstack[i].expandedMacro);
							set.insert(symbolstack[i].excludedSymbols.begin(), symbolstack[i].excludedSymbols.end());
						}

						for (int i = 0; i < args.size(); i++)
						{
							expand_macro(expansion, args[idx], lineNum, set);
						}

					}
					else
					{
						expansion.insert(expansion.end(), arguments[index].begin(), arguments[index].end());
					}
				}
				else
				{
					expansion.push_back(s);
				}
			}
			else if (mode == Hash)
			{
				if (index < 0)
				{
					error(g_include_file_stack.top(), s.line_num, "'#' is not followed by a macro parameter");
					continue;
				}
				else if (index >= arguments.size())
				{
					error(g_include_file_stack.top(), s.line_num, "Macro invoked with too few parameters for a use of '#'");
					continue;
				}

				const std::vector<Symbol> &arg = arguments.at(index);
				std::string stringified;
				for (int i = 0; i < arg.size(); ++i)
				{
					stringified += arg.at(i).lexem();
				}
				replace_all(stringified, R"(")", R"(\")");
				stringified.insert(0, 1, '"');
				stringified.insert(stringified.end(), 1, '"');
				expansion.push_back(Symbol(lineNum, EToken::STRING_LITERAL, stringified));
			}
			else if (mode == HashHash)
			{
				if (s.token == EToken::WHITESPACE)
					continue;

				while (expansion.size() && expansion.back().token == EToken::WHITESPACE)
					expansion.pop_back();

				Symbol next = s;
				if (index >= 0 && index < arguments.size())
				{
					const std::vector<Symbol> &arg = arguments.at(index);
					if (arg.size() == 0)
					{
						mode = Normal;
						continue;
					}
					next = arg.at(0);
				}

				if (!expansion.empty() && expansion.back().token == s.token
					&& expansion.back().token != EToken::STRING_LITERAL)
				{
					Symbol last = expansion.back();
					expansion.pop_back();

					std::string lexem = last.lexem() + next.lexem();
					expansion.push_back(Symbol(lineNum, last.token, lexem));
				}
				else
				{
					expansion.push_back(next);
				}

				if (index >= 0 && index < arguments.size())
				{
					const std::vector<Symbol> &arg = arguments.at(index);
					for (int i = 1; i < arg.size(); ++i)
						expansion.push_back(arg[i]);
				}
			}
			mode = Normal;
		}
		if (mode != Normal)
			error(g_include_file_stack.top(), s.line_num, "'#' or '##' found at the end of a macro argument");

		return expansion;
	}

	inline void expand_macro(std::vector<Symbol>& into, const Symbol& identifier, uint64 lineNum, const std::set<std::string>& excludeSymbols = std::set<std::string>())
	{
		if (identifier.token != EToken::IDENTIFIER)
			return;

		SymbolStack symbolstack;
		Symbol symbol = identifier;

		auto skip_replace = [&]()
		{
			bool skip_replace = false;
			for (int i = 0; i < symbolstack.size(); ++i)
			{
				if (symbol.lexem() == symbolstack[i].expandedMacro || (symbolstack[i].excludedSymbols.find(symbol.lexem()) != symbolstack[i].excludedSymbols.end()))
				{
					skip_replace = true;
				}
			}

			return skip_replace;
		};

		for (;;)
		{
			if (symbolstack.size() > 0)
			{
				symbol = symbolstack.back().symbols[symbolstack.back().index];
			}

			auto macro_itr = g_macros.find(symbol.lexem());

			if (symbol.token != EToken::IDENTIFIER || macro_itr == g_macros.end() || skip_replace())
			{
				if (symbolstack.size() > 0)
				{
					symbol = symbolstack.back().symbols[symbolstack.back().index];
					symbol.line_num = lineNum;
					symbolstack.back().index++;
				}
				into.push_back(symbol);
			}
			else
			{
				std::vector<Symbol> newSyms;
				const Macro2& macro = (*macro_itr).second;

				if (!macro.isFunction)
				{
					newSyms = macro.symbols;
				}
				else
				{
					newSyms = expand_function_macro(symbol, symbolstack, lineNum, &macro);
				}

				if (symbolstack.size() > 0)
				{
					symbolstack.back().index++;
				}

				SafeSymbols sf;
				sf.symbols = newSyms;
				sf.index = 0;
				sf.expandedMacro = symbol.lexem();
				symbolstack.push_back(sf);
				continue;
			}


			while (!symbolstack.empty() && !(symbolstack.back().index < symbolstack.back().symbols.size()))
			{
				symbolstack.pop_back();
			}

			if (symbolstack.empty())
			{
				return;
			}
		}
	}

	inline static void merge_string_literals(std::vector<Symbol> *_symbols)
	{
		std::vector<Symbol>& symbols = *_symbols;
		for (std::vector<Symbol>::iterator i = symbols.begin(); i != symbols.end(); ++i)
		{
			if (i->token == EToken::STRING_LITERAL)
			{
				std::vector<Symbol>::iterator mergeSymbol = i;
				uint64 literalsLength = mergeSymbol->len;
				while (++i != symbols.end() && i->token == EToken::STRING_LITERAL)
					literalsLength += i->len - 2; // no quotes

				if (literalsLength != mergeSymbol->len)
				{
					std::string mergeSymbolOriginalLexem = mergeSymbol->unquotedLexem();
					std::string &mergeSymbolLexem = mergeSymbol->lex;
					mergeSymbolLexem.resize(0);
					mergeSymbolLexem.reserve(literalsLength);
					mergeSymbolLexem.push_back('"');
					mergeSymbolLexem.append(mergeSymbolOriginalLexem);
					for (std::vector<Symbol>::const_iterator j = mergeSymbol + 1; j != i; ++j)
						mergeSymbolLexem.append(j->lex.data() + j->from + 1, j->len - 2); // append j->unquotedLexem()
					mergeSymbolLexem.push_back('"');
					mergeSymbol->len = mergeSymbol->lex.length();
					mergeSymbol->from = 0;
					i = symbols.erase(mergeSymbol + 1, i);
				}
				if (i == symbols.end())
					break;
			}
		}
	}

	inline static std::string searchIncludePaths(const std::vector<IncludePath> &includepaths,
		const std::string &include)
	{
		std::filesystem::path fi;
		for (int j = 0; j < includepaths.size() && !std::filesystem::exists(fi); ++j)
		{
			const IncludePath &p = includepaths.at(j);
			if (p.isFrameworkPath)
			{
				const uint64 slashPos = include.find('/');
				if (slashPos == -1)
					continue;
				fi = p.path + '/' + subset(include, 0, slashPos) + ".framework/Headers/";
				fi += subset(include, slashPos + 1);
				// fi.setFile(std::string::fromLocal8Bit(p.path + '/' + include.left(slashPos) + ".framework/Headers/"),
				// std::string::fromLocal8Bit(include.mid(slashPos + 1)));
			}
			else
			{
				fi = p.path + '/' + include;
			}
			// try again, maybe there's a file later in the include paths with the same name
			// (186067)
			if (std::filesystem::is_directory(fi))
			{
				fi.clear();
				continue;
			}
		}

		if (!std::filesystem::exists(fi) || std::filesystem::is_directory(fi))
			return std::string();

		return fi.string();
	}

	inline static std::string resolveInclude(const std::string &include, const std::string &relativeTo)
	{
		if (!relativeTo.empty())
		{
			std::filesystem::path fi;
			fi = relativeTo;
			fi /= include;
			if (std::filesystem::exists(fi) && std::filesystem::is_directory(fi))
				return fi.string();
		}

		auto it = g_nonlocalIncludePathResolutionCache.find(include);
		if (it == g_nonlocalIncludePathResolutionCache.end())
		{
			it = g_nonlocalIncludePathResolutionCache.insert_or_assign(it, include, searchIncludePaths(includes, include));
			//it = nonlocalIncludePathResolutionCache.insert(include, searchIncludePaths(includes, include));
		}
		return it->second;
	}

	int conditional_expression(const std::vector<Symbol>& symbols, uint64& index);

	int evaluate_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int return_value = 0;
		bool is_unary = true;
		uint64 start_index = index;
		uint64 primary_expression_index = index;
		while (is_unary)
		{
			switch (symbols[index++].token)
			{
				case EToken::PLUS:
				case EToken::MINUS:
				case EToken::NOT:
				case EToken::TILDE:
					primary_expression_index++;
				case EToken::MOC_TRUE:
					return 1;
				case EToken::MOC_FALSE:
					return 0;
				default:
				{
					is_unary = false;
					index--;
				}
			}
		}

		// Primary expression
		do
		{
			if (primary_expression_index == index)
			{
				int value;
				if (index < symbols.size() && symbols[index].token == EToken::LPAREN)
				{
					index++;
					value = conditional_expression(symbols, index);
					if (index < symbols.size() && symbols[index].token == EToken::RPAREN)
					{
						index++;
					}
					else
					{
						error(g_include_file_stack.top(), symbols[index]);
					}
				}
				else
				{
					return_value = std::stoi(symbols[index].lexem());
					if (index < symbols.size())
					{
						index++;
					}
					// TODO
					//value = lexem().toInt(0, 0);
				}
			}
			else
			{
				switch (symbols[index++].token)
				{
					case EToken::PLUS:
						return_value = +return_value;
					case EToken::MINUS:
						return_value = -return_value;
					case EToken::NOT:
						return_value = !return_value;
					case EToken::TILDE:
						return_value = ~return_value;
				}
			}
		} while (index != start_index);

		return return_value;
	}

	int multiplicative_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = evaluate_expression(symbols, index);
		switch (symbols[index++].token)
		{
			case EToken::STAR:
				return value * multiplicative_expression(symbols, index);
			case EToken::PERCENT:
			{
				int remainder = multiplicative_expression(symbols, index);
				return remainder ? value % remainder : 0;
			}
			case EToken::SLASH:
			{
				int div = multiplicative_expression(symbols, index);
				return div ? value / div : 0;
			}
			default:
				index--;
				return value;
		};
	}

	int additive_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = multiplicative_expression(symbols, index);
		switch (symbols[index++].token)
		{
			case EToken::PLUS:
				return value + additive_expression(symbols, index);
			case EToken::MINUS:
				return value - additive_expression(symbols, index);
			default:
				index--;
				return value;
		}
	}

	int shift_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = additive_expression(symbols, index);
		switch (symbols[index++].token)
		{
			case EToken::LTLT:
				return value << shift_expression(symbols, index);
			case EToken::GTGT:
				return value >> shift_expression(symbols, index);
			default:
				index--;
				return value;
		}
	}

	int relational_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = shift_expression(symbols, index);
		switch (symbols[index++].token)
		{
			case EToken::LANGLE:
				return value < relational_expression(symbols, index);
			case EToken::RANGLE:
				return value > relational_expression(symbols, index);
			case EToken::LE:
				return value <= relational_expression(symbols, index);
			case EToken::GE:
				return value >= relational_expression(symbols, index);
			default:
				index--;
				return value;
		}
	}

	int equality_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = relational_expression(symbols, index);
		if (index < symbols.size())
		{
			switch (symbols[index++].token)
			{
				case EToken::EQEQ:
					return value == equality_expression(symbols, index);
				case EToken::NE:
					return value != equality_expression(symbols, index);
				default:
					index--;
					return value;
			}
		}
		// todo: original: return NOTOKEN;
		return 0;
	}

	int AND_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = equality_expression(symbols, index);
		if (index < symbols.size() && symbols[index].token == EToken::AND)
		{
			index++;
			return value & AND_expression(symbols, index);
		}
		return value;
	}

	int exclusive_OR_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = AND_expression(symbols, index);
		if (index < symbols.size() && symbols[index].token == EToken::XOR)
		{
			index++;
			return value ^ exclusive_OR_expression(symbols, index);
		}
		return value;
	}

	int inclusive_OR_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = exclusive_OR_expression(symbols, index);
		if (index < symbols.size() && symbols[index].token == EToken::OR)
		{
			index++;
			return value | inclusive_OR_expression(symbols, index);
		}
		return value;
	}

	int logical_AND_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = inclusive_OR_expression(symbols, index);
		if (index < symbols.size() && symbols[index].token == EToken::ANDAND)
		{
			index++;
			return logical_AND_expression(symbols, index) && value;
		}
		return value;
	}

	int logical_OR_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		int value = logical_AND_expression(symbols, index);
		if (index < symbols.size() && symbols[index].token == EToken::OROR)
		{
			index++;
			return logical_OR_expression(symbols, index) || value;
		}
		return value;
	}

	int conditional_expression(const std::vector<Symbol>& symbols, uint64& index)
	{
		/*
		int return_value = 0;
		bool is_conditional = true;
		uint64 start_index = index;
		uint64 primary_expression_index = index;
		while (is_conditional)
		{
			switch (symbols[index++].token)
			{
				case PP_OROR:
				case PP_ANDAND:
				case PP_OR:
				case PP_HAT:
				case PP_AND:
				case PP_EQEQ:
				case PP_NE:
				case PP_LANGLE:
				case PP_RANGLE:
				case PP_LE:
				case PP_GE:
				case PP_LTLT:
				case PP_GTGT:
				case PP_PLUS:
				case PP_MINUS:
				case PP_STAR:
				case PP_PERCENT:
				case PP_SLASH:
					primary_expression_index++;
				default:
				{
					is_conditional = false;
					index--;
				}
			}
		}

		while (index != start_index)
		{
			if (primary_expression_index == index)
			{
				int value;
				if (index < symbols.size() && symbols[index].token == PP_LPAREN)
				{
					index++;
					value = conditional_expression(symbols, index);
					if (index < symbols.size() && symbols[index].token == PP_RPAREN)
					{
						index++;
					}
					else
					{
						error(g_include_file_stack.top(), symbols[index]);
					}
				}
				else
				{
					if (index < symbols.size())
					{
						index++;
					}
					return_value = std::stoi(symbols[index - 1].lexem());
					// TODO
					//value = lexem().toInt(0, 0);
				}
			}
			else
			{
				switch (symbols[index++].token)
				{
					case PP_PLUS:
						return_value = +return_value;
					case PP_MINUS:
						return_value = -return_value;
					case PP_NOT:
						return_value = !return_value;
					case PP_TILDE:
						return_value = ~return_value;
				}
			}
		}
		*/
		////////////////////

		int value = logical_OR_expression(symbols, index);

		if (index < symbols.size() && symbols[index].token == EToken::QUESTION)
		{
			index++;
			int alt1 = conditional_expression(symbols, index);
			int alt2 = 0;

			if (index < symbols.size() && symbols[index].token == EToken::COLON)
			{
				index++;
				alt2 = conditional_expression(symbols, index);
			}

			return value ? alt1 : alt2;
		}

		return value;
	}

	int conditional_expression2(const std::vector<Symbol>& symbols, int value, uint64 index)
	{
		int return_value = 0;
		bool is_conditional = true;
		uint64 start_index = index;
		uint64 primary_expression_index = index;
		while (is_conditional)
		{
			switch (symbols[index++].token)
			{
				case EToken::LPAREN:
				{
					int value = conditional_expression(symbols, index);
					if (index < symbols.size() && symbols[index].token == EToken::RPAREN)
					{
						index++;
					}
					else
					{
						error(g_include_file_stack.top(), symbols[index]);
					}
				}
				case EToken::OROR:
				case EToken::ANDAND:
				case EToken::OR:
				case EToken::XOR:
				case EToken::AND:
				case EToken::EQEQ:
				case EToken::NE:
				case EToken::LANGLE:
				case EToken::RANGLE:
				case EToken::LE:
				case EToken::GE:
				case EToken::LTLT:
				case EToken::GTGT:
				case EToken::PLUS:
				case EToken::MINUS:
				case EToken::STAR:
				case EToken::PERCENT:
				case EToken::SLASH:
					primary_expression_index++;
				default:
				{
					is_conditional = false;
					index--;
				}
			}
		}

		while (index != start_index)
		{
			if (primary_expression_index == index)
			{
				int value;
				if (index < symbols.size() && symbols[index].token == EToken::LPAREN)
				{
					index++;
					value = conditional_expression(symbols, index);
					if (index < symbols.size() && symbols[index].token == EToken::RPAREN)
					{
						index++;
					}
					else
					{
						error(g_include_file_stack.top(), symbols[index]);
					}
				}
				else
				{
					if (index < symbols.size())
					{
						index++;
					}
					return_value = std::stoi(symbols[index - 1].lexem());
					// TODO
					//value = lexem().toInt(0, 0);
				}
			}
			else
			{
				switch (symbols[index++].token)
				{
					case EToken::PLUS:
						return_value = +return_value;
					case EToken::MINUS:
						return_value = -return_value;
					case EToken::NOT:
						return_value = !return_value;
					case EToken::TILDE:
						return_value = ~return_value;
				}
			}
		}

		return return_value;
	}

	int evaluate_preprocessor_expression(const std::vector<Symbol>& symbols)
	{
		uint64 index = 0;
		bool is_unary_expression = false;

		if (index < symbols.size())
		{
			EToken t = symbols[index].token;
			is_unary_expression |= t == EToken::IDENTIFIER || t == EToken::INTEGER_LITERAL || t == EToken::FLOATING_LITERAL || t == EToken::MOC_TRUE || t == EToken::MOC_FALSE || t == EToken::LPAREN;
			is_unary_expression |= t == EToken::PLUS || t == EToken::MINUS || t == EToken::NOT || t == EToken::TILDE || t == EToken::PREPROC_DEFINED;
		}

		return is_unary_expression ? conditional_expression(symbols, index) : 0;
	}

	int evaluate(const std::vector<Symbol>& symbols, uint64 start_index, uint64 end_index)
	{
		return 0;
	}

	/*
	void extract_expression(const std::vector<Symbol>& symbols, uint64 start_index, uint64& end_index, std::vector<Symbol>& trimmed_expression_symbols)
	{
		uint64& index = start_index;
		std::vector<Symbol> expression_symbols;

		bool is_beginning_allowed = false;
		Token t = symbols[index].token;
		is_beginning_allowed |= t == PP_IDENTIFIER || t == PP_INTEGER_LITERAL || t == PP_FLOATING_LITERAL || t == PP_MOC_TRUE || t == PP_MOC_FALSE || t == PP_LPAREN;
		is_beginning_allowed |= t == PP_PLUS || t == PP_MINUS || t == PP_NOT || t == PP_TILDE || t == PP_DEFINED;

		if (!is_beginning_allowed)
		{
			end_index = start_index;
			return;
		}

		index++;

		while (index < symbols.size())
		{

			Token token = symbols[index].token;

			if (token == PP_IDENTIFIER)
			{
				expand_macro(expression_symbols, symbols, index, symbols[index].lineNum, true);
			}
			else if (token == PP_DEFINED)
			{
				bool braces = false;

				if (index < symbols.size() && symbols.at(index).token == PP_LPAREN)
				{
					index++;
					braces = true;
				}

				if (index < symbols.size() && symbols[index].token == PP_IDENTIFIER)
				{
					index++;
				}
				else
				{
					error(g_include_file_stack.top(), symbols[index]);
				}

				Symbol if_defined_lookup = symbols[index];
				if_defined_lookup.token = g_macros.find(if_defined_lookup.lex) != g_macros.end() ? PP_MOC_TRUE : PP_MOC_FALSE;
				expression_symbols.push_back(if_defined_lookup);
				if (braces)
				{
					if (index < symbols.size() && symbols[index].token == PP_IDENTIFIER)
					{
						index++;
					}
				}
			}
			else if (token == PP_NEWLINE)
			{
				expression_symbols.push_back(symbols[index]);
				break;
			}
			else if (token == PP_WHITESPACE)
			{
			}
			else
			{
				expression_symbols.push_back(symbols[index]);
			}

			index++;
		}
		
		
		
		
		
		
		
		
		

		do
		{
			if (index < symbols.size())
			{
				Token t = symbols[index].token;
				is_unary_expression |= t == PP_IDENTIFIER || t == PP_INTEGER_LITERAL || t == PP_FLOATING_LITERAL || t == PP_MOC_TRUE || t == PP_MOC_FALSE || t == PP_LPAREN;
				is_unary_expression |= t == PP_PLUS || t == PP_MINUS || t == PP_NOT || t == PP_TILDE || t == PP_DEFINED;

				if (!is_unary_expression)
				{
					return index;
				}

				index++;
			}
			else
			{
				return index;
			}

		} while (index < symbols.size() && is_unary_expression);
	}
	*/

	inline std::vector<Symbol> preprocess_internal(const std::string &filename, std::string& input, std::vector<Symbol>& symbols, bool preprocess_only)
	{
		// phase 3: preprocess conditions and substitute std::unordered_map<std::string, Macro>
		std::vector<Symbol> result;
		// Preallocate some space to speed up the code below.
		// The magic value was found by logging the final size
		// and calculating an average when running moc over FOSS projects.
		result.reserve(input.size() / 300000);

		g_include_file_stack.push(filename);
		result.reserve(result.size() + symbols.size());
		uint64 index = 0;
		while (index < symbols.size())
		{
			Symbol& symbol = symbols[index];
			std::string lexem = symbol.lexem();
			EToken token = symbol.token;

			auto skip_symbol = [&]()
			{
				if (index < (symbols.size() - 1))
				{
					index++;
				}
				else
				{
					assert(false);
				}
			};

			auto skip_whitespaces = [&]()
			{
				while (symbols[index].token != EToken::WHITESPACE
					&& symbols[index].token != EToken::WHITESPACE_ALIAS)
				{
					skip_symbol();
				}
				skip_symbol();
			};

			auto skip_line = [&]()
			{
				while (symbols[index].token != EToken::NEWLINE)
				{
					skip_symbol();
				}
				skip_symbol();
			};



			switch (token)
			{
				case EToken::PREPROC_INCLUDE:
				{
					uint64 lineNum = symbol.line_num;
					std::string include;
					bool local = false;

					if (index < symbols.size() && symbols.at(index).token == EToken::STRING_LITERAL)
					{
						index++;
						// TODO
						local = lexem[0] == '\"'; // same as '"'
						include = symbols.at(index - 1).unquotedLexem();
					}
					else
						continue;

					while (index < symbols.size() && symbols[index++].token != EToken::NEWLINE);

					include = resolveInclude(include, local ? filename : std::string());
					if (include.empty())
						continue;

					if (g_preprocessedIncludes.find(include) != g_preprocessedIncludes.end())
						continue;

					g_preprocessedIncludes.insert(include);

					std::vector<Symbol> saveSymbols = symbols;
					uint64 saveIndex = index;

					std::string input;
					{
						std::ifstream ifs;
						ifs.open(include, std::iostream::binary);
						if (ifs.is_open())
						{
							ifs.seekg(0, ifs.end);
							uint64 size = ifs.tellg();
							ifs.seekg(0, ifs.beg);

							if (size == 0)
							{
								fprintf(stderr, "moc: %s: File Size is 0\n", include.c_str());
							}

							input.resize(size);

							if (input.capacity() >= size)
							{
								ifs.read(&input.front(), size);
							}
						}
						else
						{
							fprintf(stderr, "moc: %s: No such file\n", include.c_str());
							continue;
						}
					}

					auto include_symbols = tokenize(input, preprocess_only);

					if (!include_symbols.empty())
					{
						symbols = include_symbols;
					}
					else
					{
						continue;
					}

					index = 0;

					// phase 3: preprocess conditions and substitute std::unordered_map<std::string, Macro>
					result.push_back(Symbol(0, EToken::MOC_INCLUDE_BEGIN, include));
					assert(false);
					//result += preprocess_internal(include, input, symbols, preprocess_only);
					result.push_back(Symbol(lineNum, EToken::MOC_INCLUDE_END, include));

					symbols = saveSymbols;
					index = saveIndex;
					continue;
				}
				case EToken::PREPROC_DEFINE:
				{
					// # define
					skip_whitespaces();
					// # define name
					std::string name = symbols[index].lexem();
					if (name.empty() || !is_ident_start(name[0]))
					{
						assert(false);
						error(g_include_file_stack.top(), symbols[index]);
					}
					skip_symbol();

					Macro2 macro;
					macro.isVariadic = false;

					// # define name(
					if (symbols[index].token == EToken::LPAREN)
					{
						skip_symbol();
						macro.isFunction = true;

						std::vector<Symbol> arguments;
						int arg_count = 0;
						int comma_count = 0;
						while (symbols[index].token != EToken::RPAREN)
						{
							skip_whitespaces();

							EToken token = symbols[index].token;

							switch (token)
							{
							// # define name(arg,
							case EToken::COMMA:
							{
								assert(arg_count == (comma_count + 1));
								comma_count++;
								skip_symbol();
							} break;
							// # define name(arg
							case EToken::IDENTIFIER:
							{
								Symbol symbol = symbols[index];
								if (std::find(arguments.begin(), arguments.end(), symbol) != arguments.end())
								{
									error(g_include_file_stack.top(), symbols[index].line_num, "Duplicate macro parameter.");
								}
								assert(comma_count == arg_count);
								arguments.push_back(symbols[index]);
								skip_symbol();
							} break;
							// # define name(arg...
							case EToken::ELIPSIS:
							{
								macro.isVariadic = true;
								arguments.emplace_back(symbols[index].line_num, EToken::IDENTIFIER, "__VA_ARGS__");
								skip_whitespaces();
								// ... must be the last argument
								if (symbols[index].token != EToken::RPAREN)
								{
									error(g_include_file_stack.top(), symbols[index].line_num, "missing ')' in macro argument list");
								}
							}break;
							default:
							{
								assert(false);
								if (!is_identifier(lexem.data(), lexem.length()))
								{
									error(g_include_file_stack.top(), symbols[index].line_num, "Unexpected character in macro argument list.");
								}
							} break;
							}
							/*
							if (symbols[index].lexem() == "...")
							{
								//GCC extension:    #define FOO(x, y...) x(y)
								// The last argument was already parsed. Just mark the macro as variadic.
								macro.isVariadic = true;
								while (index < symbols.size() && symbols[index++].token == EToken::WHITESPACE);
								if (!(index < symbols.size() && symbols[index].token == EToken::RPAREN))
									error(g_include_file_stack.top(), symbols[index].line_num, "missing ')' in macro argument list.");
								break;
							}
							*/
						}
						macro.arguments = arguments;
						skip_whitespaces();
					}
					else
					{
						macro.isFunction = false;
					}

					skip_whitespaces();

					// # define name(...) expression
					// # define name expression
					uint64 rewind = index;
					do 
					{
						skip_line();
					} while (symbols[index - 2].token == EToken::BACKSLASH);
					uint64 end = index;
					index = rewind;
					macro.symbols.reserve(end - index - 1);

					// remove whitespace where there shouldn't be any:
					// after the macro, after a #, and around ##
					while (index < (end - 1))
					{
						EToken token = symbols[index].token;
						switch (token)
						{
						case EToken::PREPROC_HASH:
						{
							macro.symbols.push_back(symbols[index]);
							skip_whitespaces();
						} break;
						case EToken::PREPROC_HASHHASH:
						case EToken::BACKSLASH:
						case EToken::NEWLINE:
						{
							while (!macro.symbols.empty()
								&& (macro.symbols.back().token != EToken::WHITESPACE
									|| macro.symbols.back().token != EToken::WHITESPACE_ALIAS))
							{
								macro.symbols.pop_back();
							}
							macro.symbols.push_back(symbols[index]);
							if (token == EToken::PREPROC_HASHHASH)
							{
								skip_whitespaces();
							}
						} break;
						default:
							macro.symbols.push_back(symbols[index]);
							break;
						}

						skip_symbol();
					}

					/*
					// remove trailing whitespace
					//while (!macro.symbols.empty() &&
					//	(macro.symbols.back().token == EToken::WHITESPACE || macro.symbols.back().token == EToken::WHITESPACE_ALIAS))
					//	macro.symbols.pop_back();

					// Do we need this?
					if (!macro.symbols.empty())
					{
						if (macro.symbols.front().token == EToken::PREPROC_HASHHASH ||
							macro.symbols.back().token == EToken::PREPROC_HASHHASH)
						{
							error(g_include_file_stack.top(), symbols[index].line_num, "'##' cannot appear at either end of a macro expression.");
						}
					}
					*/
					g_macros.insert_or_assign(name, macro);
					continue;
				} break;
				case EToken::PREPROC_UNDEF:
				{
					skip_whitespaces();
					std::string name = symbols[index].lexem();
					g_macros.erase(name);

					// is it ok to ignore the rest of the line?
					//assert(false);
					skip_line();

					continue;
				} break;
				case EToken::IDENTIFIER:
				{
					expand_macro(result, symbols[index], symbol.line_num);
					index++;
					continue;
				}
				/*
				case EToken::PREPROC_HASH:
				{

					//while (index < symbols.size() && symbols[index++].token != EToken::NEWLINE);
					//continue; // skip unknown preprocessor statement
				}
				*/
				case EToken::PREPROC_IFDEF:
				case EToken::PREPROC_IFNDEF:
				case EToken::PREPROC_IF:
				{
					// fix this crap
					uint64 end_index = 0;// find_end_of_expression(symbols, index);
					end_index++;


					auto evaluateCondition = [&](const std::vector<Symbol>& symbols, uint64& index)
					{
						std::vector<Symbol> expression_symbols;

						while (index < symbols.size())
						{
							EToken token = symbols[index].token;

							if (token == EToken::IDENTIFIER)
							{
								expand_macro(expression_symbols, symbols[index], symbols[index].line_num);
							}
							else if (token == EToken::PREPROC_DEFINED)
							{
								bool braces = false;

								if (index < symbols.size() && symbols.at(index).token == EToken::LPAREN)
								{
									index++;
									braces = true;
								}

								if (index < symbols.size() && symbols[index].token == EToken::IDENTIFIER)
								{
									index++;
								}
								else
								{
									error(g_include_file_stack.top(), symbols[index]);
								}

								Symbol if_defined_lookup = symbols[index];
								if_defined_lookup.token = g_macros.find(if_defined_lookup.lex) != g_macros.end() ? EToken::MOC_TRUE : EToken::MOC_FALSE;
								expression_symbols.push_back(if_defined_lookup);
								if (braces)
								{
									if (index < symbols.size() && symbols[index].token == EToken::IDENTIFIER)
									{
										index++;
									}
								}
							}
							else if (token == EToken::NEWLINE)
							{
								expression_symbols.push_back(symbols[index]);
								break;
							}
							else if (token == EToken::WHITESPACE)
							{
							}
							else
							{
								expression_symbols.push_back(symbols[index]);
							}

							index++;
						}

						return evaluate_preprocessor_expression(expression_symbols);
					};

					while (!evaluateCondition(symbols, index))
					{
						uint64 skip_count = 1;

						while (index < symbols.size() - 1 && skip_count)
						{
							switch (symbols[index++].token)
							{
								case EToken::PREPROC_IF:
								case EToken::PREPROC_IFDEF:
								case EToken::PREPROC_IFNDEF:
									++index;
									skip_count++;
									break;
								case EToken::PREPROC_ENDIF:
								case EToken::PREPROC_ELIF:
								case EToken::PREPROC_ELSE:
									skip_count--;
									break;
							}
							++index;
						}

						if (index >= symbols.size() - 1)
							break;

						if (index < symbols.size() && symbols[index].token == EToken::PREPROC_ELIF)
						{
							index++;
						}
						else
						{
							while (index < symbols.size() && symbols.at(index++).token != EToken::NEWLINE);
							break;
						}
					}
					continue;
				}
				case EToken::PREPROC_ELIF:
				case EToken::PREPROC_ELSE:
				{
					uint64 skip_count = 1;

					while (index < symbols.size() - 1 && skip_count)
					{
						switch (symbols[index++].token)
						{
							case EToken::PREPROC_IF:
							case EToken::PREPROC_IFDEF:
							case EToken::PREPROC_IFNDEF:
								index++;
								skip_count++;
								break;
							case EToken::PREPROC_ENDIF:
								skip_count--;
								break;
						}
					}
					[[fallthrough]];
				}
				case EToken::PREPROC_ENDIF:
					while (index < symbols.size() && symbols[index++].token != EToken::NEWLINE);
					continue;
				case EToken::PREPROC_HASH:
				case EToken::NEWLINE:
					index++;
					continue;
				/*
				case EToken::SIGNALS:
				case EToken::SLOTS:
				{
					Symbol sym = symbol;
					if (g_macros.find("QT_NO_KEYWORDS") != g_macros.end())
						sym.token = EToken::IDENTIFIER;
					else
						sym.token = (token == EToken::SIGNALS ? EToken::Q_SIGNALS_TOKEN : EToken::Q_SLOTS_TOKEN);
					result.push_back(sym);
				} continue;
				*/
				default:
					break;
			}
			result.push_back(symbol);
			index++;
		}

		g_include_file_stack.pop();

		return result;
	}


	inline std::vector<Symbol> preprocess(const std::string &filename, std::string& input, std::vector<Symbol>& symbols, bool preprocess_only)
	{
		#if 0
		for (int j = 0; j < symbols.size(); ++j)
			fprintf(stderr, "line %d: %s(%s)\n",
				symbols[j].lineNum,
				symbols[j].lexem().constData(),
				tokenTypeName(symbols[j].token));
		#endif



		return preprocess_internal(filename, input, symbols, preprocess_only);

		//merge_string_literals(&result);

		#if 0
		for (int j = 0; j < result.size(); ++j)
			fprintf(stderr, "line %d: %s(%s)\n",
				result[j].lineNum,
				result[j].lexem().constData(),
				tokenTypeName(result[j].token));
		#endif

		//return result;
	}


}

#endif