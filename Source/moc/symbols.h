#ifndef SYMBOLS_H
#define SYMBOLS_H

#include "keywords_and_tokens_baked.h"
#include "utils.h"

#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <stack>
//#include <qdebug.h>

namespace header_tool
{
	struct Symbol
	{
		uint64 line_num;
		EToken token;
		union
		{
			const char* c_str;
			const std::string* str;
		};
		uint64 size;

		//const char* lex_view;
		//uint64 from;

		inline Symbol() : line_num(0), token(EToken::NULL_TOKEN), c_str(0), size(0)
		{
		}

		/*
		inline Symbol(uint64 line_num, EToken token) : line_num(line_num), token(token), length(0)
		{
		}
		*/


		inline Symbol(uint64 line_num, EToken token, const std::string* lexem) : line_num(line_num), str(lexem), token(token), size(0)
		{
		}

		inline Symbol(uint64 line_num, EToken token, const char* lexem, size_t len) : line_num(line_num), c_str(lexem), token(token), /*from(from), */size(len)
		{
		}

		~Symbol()
		{
			if (size == 0)
			{
				if (str != 0)
				{
					delete str;
				}
			}
		}

		/*
		inline Symbol& operator=(const Symbol& other)
		{
			line_num = other.line_num;
			lex = other.lex;
			token = other.token;
			from = other.from;
			len = other.len;

			//lex_view = lex.c_str() + from;

			return *this;
		}

		inline Symbol(const Symbol& other) : line_num(other.line_num), lex_view(other.lex_view), lex(other.lex), token(other.token), from(other.from), len(other.len)
		{
			//lex_view = lex.c_str() + from;
		}
		inline Symbol& operator=(Symbol&& other)
		{
			line_num = other.line_num;
			lex = std::move(other.lex);
			token = other.token;
			from = other.from;
			len = other.len;

			lex_view = other.lex_view;

			return *this;
		}

		inline Symbol(Symbol&& other) : line_num(other.line_num), lex(std::move(other.lex)), token(token), from(from), len(len)
		{
			lex_view = lex.c_str() + from;
		}
		*/

		inline std::string string() const
		{
			if (size != 0)
			{
				if (c_str != 0)
				{
					return std::string(c_str, size);
				}
				else
				{
					assert(false);
				}
			}
			else
			{
				if (str != 0)
				{
					return *str;
				}
			}

			return std::string();
		}
		inline std::string unquotedLexem() const
		{
			if (size != 0)
			{
				if (c_str != 0)
				{
					return std::string(c_str + 1, std::max(0ULL, size - 2)); //return subset(lexem, from + 1, length - 2);
				}
				else
				{
					assert(false);
				}
			}
			else
			{
				if (str != 0)
				{
					return *str;
				}
				else
				{
					assert(false);
				}
			}

			return std::string(str->data() + 1, std::max(0ULL, str->size() - 2)); //return subset(lexem, from + 1, length - 2);
		}

		bool operator==(const Symbol& o) const
		{
			return c_str == o.c_str && /*from == o.from &&*/ size == o.size; // (lex, from, len) == SubArray(o.lex, o.from, o.len);
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
			while (!empty() && !(back().index < back().symbols.size()))
			{
				pop_back();
			}

			return !empty();
		}

		inline EToken next()
		{
			while (!empty() && !(back().index < back().symbols.size()))
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
			while (stackPos >= 0
				&& data()[stackPos].index >= data()[stackPos].symbols.size())
			{
				--stackPos;
			}

			if (stackPos < 0)
			{
				return false;
			}

			if (data()[stackPos].symbols[data()[stackPos].index].token == token)
			{
				back().index++;
				return true;
			}
			return false;
		}
	};
}

#endif // SYMBOLS_H
