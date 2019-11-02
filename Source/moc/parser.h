#ifndef PARSER_H
#define PARSER_H

#include "symbols.h"

#include <stack>

#define Q_NORETURN

namespace header_tool
{
	class Parser
	{
	public:
		Parser() :index(0), displayWarnings(true), displayNotes(true)
		{}
		std::vector<Symbol> symbols;
		uint64 index;
		bool displayWarnings;
		bool displayNotes;

		struct IncludePath
		{
			inline explicit IncludePath(const std::string &_path, bool isFrameworkPath = false )
				: path(_path), isFrameworkPath( isFrameworkPath )
			{}
			std::string path;
			bool isFrameworkPath;
		};
		std::vector<IncludePath> includes;

		std::stack<std::string, std::vector<std::string>> g_include_file_stack;
		//std::stack<std::string, std::vector<std::string>> currentFilenames;

		inline bool hasNext() const
		{
			return (index < symbols.size());
		}
		inline EToken next()
		{
			if (index >= symbols.size()) return EToken::NULL_TOKEN; return symbols.at(index++).token;
		}
		bool test(EToken);
		void next(EToken);
		void next(EToken, const char *msg);
		inline void prev()
		{
			--index;
		}
		inline EToken lookup(uint64 k = 1);
		inline const Symbol &symbol_lookup(int k = 1)
		{
			return symbols.at(index - 1 + k);
		}
		inline EToken token()
		{
			return symbols.at(index - 1).token;
		}
		inline std::string lexem()
		{
			return symbols.at(index - 1).lexem();
		}
		inline std::string unquotedLexem()
		{
			return symbols.at(index - 1).unquotedLexem();
		}
		inline const Symbol &symbol()
		{
			return symbols.at(index - 1);
		}

		Q_NORETURN void error(int rollback);
		Q_NORETURN void error(const char *msg = 0);
		void warning(const char * = 0);
		void note(const char * = 0);

	};

	inline bool Parser::test(EToken token)
	{
		if (index < symbols.size() && symbols.at(index).token == token)
		{
			++index;
			return true;
		}
		return false;
	}

	inline EToken Parser::lookup(uint64 k /*= 1*/)
	{
		const uint64 l = index - 1 + k;
		return l < symbols.size() ? symbols.at(l).token : EToken::NULL_TOKEN;
	}

	inline void Parser::next(EToken token)
	{
		if (!test(token))
			error();
	}

	inline void Parser::next(EToken token, const char *msg)
	{
		if (!test(token))
			error(msg);
	}

}

#endif
