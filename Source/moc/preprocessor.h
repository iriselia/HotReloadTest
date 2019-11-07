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
	struct Macro
	{
		Macro(bool isFunction = false, bool isVariadic = false) : isFunction(isFunction), isVariadic(isVariadic)
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
	std::unordered_map<std::string, std::string> g_source_file_atlas;
	std::set<std::string> g_preprocessedIncludes;
	std::unordered_map<std::string, Macro> g_macros;
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
		fprintf(stderr, ErrorFormatString "Parse error at \"%s\"\n", filename.c_str(), symbol.line_num, symbol.string().data());
		exit(EXIT_FAILURE);
	}

	inline static void expand_macro(std::vector<Symbol>& into, const Symbol& toExpand, uint64 lineNum, const std::set<std::string>& excludeSymbols);

	inline static std::vector<Symbol> expand_function_macro(Symbol symbol, SymbolStack &symbolstack, uint64 lineNum, Macro macro)
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
				std::string& stringified = *new std::string;
				for (int i = 0; i < arg.size(); ++i)
				{
					stringified += arg.at(i).string();
				}
				replace_all(stringified, R"(")", R"(\")");
				stringified.insert(0, 1, '"');
				stringified.insert(stringified.end(), 1, '"');
				expansion.push_back(Symbol(lineNum, EToken::STRING_LITERAL, &stringified));
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

					std::string& lexem = *new std::string;
					lexem = last.string() + next.string();
					expansion.push_back(Symbol(lineNum, last.token, &lexem));
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
				if (symbol.string() == symbolstack[i].expandedMacro || (symbolstack[i].excludedSymbols.find(symbol.string()) != symbolstack[i].excludedSymbols.end()))
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
				symbol = std::move(symbolstack.back().symbols[symbolstack.back().index]);
			}

			auto macro_itr = g_macros.find(symbol.string());

			if (symbol.token != EToken::IDENTIFIER || macro_itr == g_macros.end() || skip_replace())
			{
				if (symbolstack.size() > 0)
				{
					symbol = std::move(symbolstack.back().symbols[symbolstack.back().index]);
					symbol.line_num = lineNum;
					symbolstack.back().index++;
				}
				into.push_back(std::move(symbol));
			}
			else
			{
				std::vector<Symbol> newSyms;
				const Macro& macro = (*macro_itr).second;

				if (!macro.isFunction)
				{
					newSyms = macro.symbols;
				}
				else
				{
					newSyms = std::move(expand_function_macro(symbol, symbolstack, lineNum, &macro));
				}

				if (symbolstack.size() > 0)
				{
					symbolstack.back().index++;
				}

				SafeSymbols sf;
				sf.symbols = newSyms;
				sf.index = 0;
				sf.expandedMacro = symbol.string();
				symbolstack.push_back(std::move(sf));
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
		/*
		std::vector<Symbol>& symbols = *_symbols;
		for (std::vector<Symbol>::iterator i = symbols.begin(); i != symbols.end(); ++i)
		{
			if (i->token == EToken::STRING_LITERAL)
			{
				std::vector<Symbol>::iterator mergeSymbol = i;
				uint64 literalsLength = mergeSymbol->size;
				while (++i != symbols.end() && i->token == EToken::STRING_LITERAL)
					literalsLength += i->size - 2; // no quotes

				if (literalsLength != mergeSymbol->size)
				{
					std::string mergeSymbolOriginalLexem = mergeSymbol->unquotedLexem();
					std::string &mergeSymbolLexem = mergeSymbol->string();
					mergeSymbolLexem.resize(0);
					mergeSymbolLexem.reserve(literalsLength);
					mergeSymbolLexem.push_back('"');
					mergeSymbolLexem.append(mergeSymbolOriginalLexem);
					for (std::vector<Symbol>::const_iterator j = mergeSymbol + 1; j != i; ++j)
						mergeSymbolLexem.append(j->lexem.data() + j->from + 1, j->size - 2); // append j->unquotedLexem()
					mergeSymbolLexem.push_back('"');
					mergeSymbol->size = mergeSymbol->lexem.length();
					mergeSymbol->from = 0;
					i = symbols.erase(mergeSymbol + 1, i);
				}
				if (i == symbols.end())
					break;
			}
		}
		*/
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
		/*
		std::filesystem::path file_path = include;
		if (file_path.is_relative())
		{
			file_path = std::filesystem::current_path() / include;
		}

		if (std::filesystem::exists(file_path) && std::filesystem::is_directory(file_path))
			return file_path.string();
		*/

		if (!relativeTo.empty())
		{
			std::filesystem::path file_path;
			file_path = relativeTo;
			file_path = file_path.parent_path();
			file_path /= include;
			if (std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path))
				return file_path.string();
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
					return_value = std::stoi(symbols[index].string());
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
			case EToken::LSHIFT:
				return value << shift_expression(symbols, index);
			case EToken::RSHIFT:
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
			case EToken::LESSEQ:
				return value <= relational_expression(symbols, index);
			case EToken::GREATEREQ:
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
				case EToken::NOTEQ:
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
				case EToken::NOTEQ:
				case EToken::LANGLE:
				case EToken::RANGLE:
				case EToken::LESSEQ:
				case EToken::GREATEREQ:
				case EToken::LSHIFT:
				case EToken::RSHIFT:
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
					return_value = std::stoi(symbols[index - 1].string());
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

	int lbp(EToken token)
	{
		switch (token)
		{
			case EToken::QUESTION:
				return 1;
			case EToken::OROR:
			case EToken::OROR_ALIAS:
				return 2;
			case EToken::ANDAND:
			case EToken::ANDAND_ALIAS:
				return 3;
			case EToken::OR:
			case EToken::OR_ALIAS:
				return 4;
			case EToken::XOR:
			case EToken::XOR_ALIAS:
				return 5;
			case EToken::AND:
			case EToken::AND_ALIAS:
				return 6;
			case EToken::EQEQ:
			case EToken::NOTEQ:
			case EToken::NOTEQ_ALIAS:
				return 7;
			case EToken::LANGLE:
			case EToken::RANGLE:
			case EToken::LESSEQ:
			case EToken::GREATEREQ:
				return 8;
			case EToken::LSHIFT:
			case EToken::RSHIFT:
				return 9;
			case EToken::PLUS:
			case EToken::MINUS:
				return 10;
			case EToken::STAR:
			case EToken::SLASH:
			case EToken::PERCENT:
				return 11;
			case EToken::NOT:
			case EToken::TILDE:
			case EToken::TILDE_ALIAS:
				return 12;
		}

		return 0;
	};

		/*
	Expression(0)
		4.Nud()
		*.Led(4)
			Expression(*.Lbp)
				5.Nud()
		+.Led(4*5)
			Expression(+.Lbp)
				3.Nud()

	Expression(0)
		4.Nud()
		+.Led(4)
			Expression(+.Lbp)
				5.Nud()
				*.Led(5)
					Expression(*.Lbp)
						3.Nud()
	*/

	/*
	var expression = function (rbp) {
		var left;
		var t = token;
		advance();
		left = t.nud();
		while (rbp < token.lbp) {
			t = token;
			advance();
			left = t.led(left);
		}
		return left;
	}
	*/
	

	const std::vector<Symbol>* temp_symbols;
	uint64* temp_index;

	int nud(EToken t)
	{
		const std::vector<Symbol>& symbols = *temp_symbols;
		uint64& index = *temp_index;

		switch (t)
		{
			case EToken::NOT:
				index++;
				return !std::stoi(symbols[index].string());
				break;
			case EToken::INTEGER_LITERAL:
				return std::stoi(symbols[index].string());
				break;
			default:
				assert(false);
				break;
		}
		return 4;
	};

	int expression(int rbp);

	int led(EToken op, int left)
	{
		switch (op)
		{
			case EToken::PLUS:
				return left + expression(lbp(op));
				break;
			case EToken::STAR:
				return left * expression(lbp(op));
				break;
			default:
				// new line
				return left;
				break;
		}

		return 0;
	};

	int expression(int rbp)
	{
		const std::vector<Symbol>& symbols = *temp_symbols;
		uint64& index = *temp_index;
		// 4 * 5 + 3
		int left = nud(symbols[index].token); // 4.Nud()
		index++;
		while (rbp < lbp(symbols[index].token))
		{
			left = led(symbols[index++].token, left); // +.Led(4)
		}
		return left;
	};

	inline std::vector<Symbol> preprocess_internal(const std::string &filename, const std::string& input, const std::vector<Symbol>& symbols, bool preprocess_only)
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
			const Symbol& symbol = symbols[index];
			std::string lexem = symbol.string();
			EToken token = symbol.token;

			auto skip_symbol = [&]()
			{
				if (index < (symbols.size() - 1))
				{
					index++;
				}
				else
				{
					return false;
				}
				return true;
			};

			auto skip_whitespaces = [&]()
			{
				while (symbols[index].token != EToken::WHITESPACE
					&& symbols[index].token != EToken::WHITESPACE_ALIAS
					&& skip_symbol())
				{
				}
				skip_symbol();
			};

			auto skip_line = [&]()
			{
				while (symbols[index].token != EToken::NEWLINE
					&& skip_symbol())
				{
				}
				skip_symbol();
			};



			switch (token)
			{
				case EToken::PREPROC_INCLUDE:
				{
					uint64 lineNum = symbol.line_num;
					std::string include_file;
					bool local = false;

					skip_whitespaces();

					if (symbols[index].token == EToken::STRING_LITERAL)
					{
						local = symbols[index].string()[0] == '\"';
						include_file = symbols[index].unquotedLexem();
						skip_symbol();
					}
					else
					{
						assert(false);
						continue;
					}

					skip_line();

					if (!filename.empty())
					{
						std::filesystem::path file_path;
						file_path = filename;
						file_path = file_path.parent_path();
						file_path /= include_file;
						if (std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path))
						{
							include_file = file_path.string();
						}
						else
						{
							assert(false);
						}
					}

					auto it = g_nonlocalIncludePathResolutionCache.find(include_file);
					if (it == g_nonlocalIncludePathResolutionCache.end())
					{
						it = g_nonlocalIncludePathResolutionCache.insert_or_assign(it, include_file, searchIncludePaths(includes, include_file));
						//it = nonlocalIncludePathResolutionCache.insert(include, searchIncludePaths(includes, include));
					}
					include_file = it->second;

					if (include_file.empty())
					{
						continue;
					}

					if (g_preprocessedIncludes.find(include_file) != g_preprocessedIncludes.end())
					{
						continue;
					}

					g_preprocessedIncludes.insert(include_file);

					//std::vector<Symbol> saveSymbols = symbols;
					uint64 saveIndex = index;

					std::string input;
					{
						std::ifstream ifs;
						ifs.open(include_file, std::iostream::binary);
						if (!ifs.is_open())
						{
							fprintf(stderr, "moc: %s: No such file\n", include_file.c_str());
							continue;
						}
						ifs.seekg(0, ifs.end);
						uint64 size = ifs.tellg();
						ifs.seekg(0, ifs.beg);

						if (size == 0)
						{
							fprintf(stderr, "moc: %s: File Size is 0\n", include_file.c_str());
						}

						input.resize(size);
						ifs.read(&input.front(), size);
					}

					auto source_itr = g_source_file_atlas.emplace(include_file, std::move(input)).first;
					auto& source_name = source_itr->first;
					auto& source = source_itr->second;
					auto include_symbols = tokenize(input, preprocess_only);

					if (!include_symbols.empty())
					{
						const std::vector<Symbol>& preprocessed_symbols = preprocess_internal(include_file, input, include_symbols, preprocess_only);
						result.push_back(Symbol(0, EToken::MOC_INCLUDE_BEGIN, &source_name));
						result.reserve(result.size() + preprocessed_symbols.size());
						std::move(std::begin(preprocessed_symbols), std::end(preprocessed_symbols), std::back_inserter(result));
						result.push_back(Symbol(lineNum, EToken::MOC_INCLUDE_END, &source_name));
					}
					else
					{
						continue;
					}

					//index = 0;



					//symbols = saveSymbols;
					//index = saveIndex;
					continue;
				}
				case EToken::PREPROC_DEFINE:
				{
					// # define
					skip_whitespaces();
					// # define name
					std::string name = symbols[index].string();
					if (name.empty() || !is_ident_start(name[0]))
					{
						assert(false);
						error(g_include_file_stack.top(), symbols[index]);
					}
					skip_symbol();

					Macro macro;
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
								arguments.emplace_back(symbols[index].line_num, EToken::IDENTIFIER, new std::string("__VA_ARGS__"));
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
					std::string name = symbols[index].string();
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
				} break;
				case EToken::PREPROC_IFDEF:
				case EToken::PREPROC_IFNDEF:
				case EToken::PREPROC_IF:
				{
					// fix this crap
					uint64 end_index = 0;// find_end_of_expression(symbols, index);
					end_index++;

					
					auto nud = [&symbols, &index](EToken t, auto& expr_ref, auto& led_ref, auto& nud_ref)->int
					{
						switch (t)
						{
							case EToken::LPAREN:
							{
								index++;
								int result = expr_ref(0, expr_ref, led_ref, nud_ref);
								index++;
								return result;

							} break;
							case EToken::NOT:
								index++;
								return !std::stoi(symbols[index].string());
								break;
							case EToken::INTEGER_LITERAL:
								return std::stoi(symbols[index].string());
								break;
							default:
								assert(false);
								break;
						}
						return 4;
					};

					auto led = [](EToken op, int left, auto& expr_ref, auto& led_ref, auto& nud_ref)->int
					{
						switch (op)
						{
							case EToken::PLUS:
								return left + expr_ref(lbp(op), expr_ref, led_ref, nud_ref);
								break;
							case EToken::STAR:
								return left * expr_ref(lbp(op), expr_ref, led_ref, nud_ref);
								break;
							default:
								// new line
								return left;
								break;
						}

						return 0;
					};

					auto expr = [&symbols, &index](int rbp, auto& expr_impl, auto& led_ref, auto& nud_ref)->int
					{
						// 4 * 5 + 3
						int left = nud_ref(symbols[index].token, expr_impl, led_ref, nud_ref); // 4.Nud()
						index++;
						while (rbp < lbp(symbols[index].token))
						{
							left = led_ref(symbols[index++].token, left, expr_impl, led_ref, nud_ref); // +.Led(4)
						}
						return left;
					};

					auto eval = [&symbols, &index, expr, led, nud]()
					{
						return expr(0, expr, led, nud);
					};

					skip_symbol();
					skip_whitespaces();

					int b = eval();
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
					skip_line();
					continue;
				case EToken::PREPROC_HASH:
					skip_symbol();
					continue;
				case EToken::NEWLINE:
					break;
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
			result.push_back(std::move(symbol));
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