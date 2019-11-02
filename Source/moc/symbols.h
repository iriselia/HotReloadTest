#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "keywords_and_tokens_baked.h"
#include "utils.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
//#include <qdebug.h>

namespace header_tool
{
	struct Symbol
	{
		uint64 line_num;
		const char* lex_view;
		std::string lex;
		EToken token;
		uint64 from;
		uint64 len;

		inline Symbol() : line_num(-1), token(EToken::NULL_TOKEN), from(0), len(-1)
		{
		}


		inline Symbol(uint64 line_num, EToken token) : line_num(line_num), token(token), from(0), len(-1)
		{
		}

		inline Symbol(uint64 line_num, EToken token, const std::string &lexem) : line_num(line_num), lex_view(lexem.c_str()), lex(lexem), token(token), from(0), len(lex.size())
		{
		}

		inline Symbol(uint64 line_num, EToken token, const std::string &lexem, size_t from, size_t len) : line_num(line_num), lex_view(lexem.c_str() + from), lex(lexem), token(token), from(from), len(len)
		{
		}

		inline std::string lexem() const
		{
			return subset(lex, from, len);
		}
		inline std::string unquotedLexem() const
		{
			return subset(lex, from + 1, len - 2);
		}

		bool operator==(const Symbol& o) const
		{
			return lex == o.lex && from == o.from && len == o.len; // (lex, from, len) == SubArray(o.lex, o.from, o.len);
		}
	};
	//Q_DECLARE_TYPEINFO(Symbol, Q_MOVABLE_TYPE);

	struct SafeSymbols
	{
		std::vector<Symbol> symbols;
		std::string expandedMacro;
		std::set<std::string> excludedSymbols;
		uint64 index;
	};
	//Q_DECLARE_TYPEINFO(SafeSymbols, Q_MOVABLE_TYPE);

	class SymbolStack : public std::vector<SafeSymbols>
	{
	public:
		inline bool has_next()
		{
			while (!empty() && !(back().index + 1 < back().symbols.size()))
			{
				pop_back();
			}

			return !empty();
		}

		inline EToken next()
		{
			while (!empty() && !(back().index + 1 < back().symbols.size()))
			{
				pop_back();
			}

			if (empty())
			{
				return EToken::NULL_TOKEN;
			}

			return back().symbols[back().index++].token;
		}

		inline bool test(EToken token)
		{
			size_t stackPos = size() - 1;
			while (stackPos >= 0 && data()[stackPos].index >= data()[stackPos].symbols.size())
				--stackPos;
			if (stackPos < 0)
				return false;
			if (data()[stackPos].symbols[data()[stackPos].index].token == token)
			{
				next();
				return true;
			}
			return false;
		}
	};
}

#endif // SYMBOLS_H
