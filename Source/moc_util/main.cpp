/* shared code */
#pragma region shared code

struct RawKeyword
{
	const char* lexem;
	const char* token;
};

static const RawKeyword s_cpp_preproc_keywords[] =
{
	#define INCLUDE_C_PREPROC_KEYWORDS 1
	#define INCLUDE_C_PREPROC_KEYWORD(lexem, token) {lexem, "PREPROC_" ## #token},
	#define INCLUDE_C_PREPROC_KEYWORD_ALIAS(lexem, token) {lexem, "PREPROC_" ## #token},

	#define INCLUDE_CPP_SHARED_KEYWORDS 1
	#define INCLUDE_CPP_SHARED_KEYWORD(lexem, token) {lexem, #token},
	#define INCLUDE_CPP_SHARED_KEYWORD_ALIAS(lexem, token) {lexem, #token},
	#include "keywords_and_tokens_raw.h"
};

static const RawKeyword s_cpp_keywords[] =
{
	#define INCLUDE_CPP_SHARED_KEYWORDS 1
	#define INCLUDE_CPP_SHARED_KEYWORD(lexem, token) {lexem, #token},
	#define INCLUDE_CPP_SHARED_KEYWORD_ALIAS(lexem, token) {lexem, #token},

	#define INCLUDE_CPP_KEYWORDS 1
	#define INCLUDE_CPP_KEYWORD(lexem, token) {lexem, #token},
	#define INCLUDE_CPP_KEYWORD_ALIAS(lexem, token) {lexem, #token},

	#include "keywords_and_tokens_raw.h"
};

enum class EToken
{
	#define INCLUDE_TOKENS 1
	#define INCLUDE_TOKEN(token) token,

	#define INCLUDE_C_PREPROC_KEYWORDS 1
	#define INCLUDE_C_PREPROC_KEYWORD(lexem, token) PREPROC_ ## token,
	#define INCLUDE_C_PREPROC_KEYWORD_ALIAS(lexem, token) PREPROC_ ## token,
	
	#define INCLUDE_CPP_SHARED_KEYWORDS 1
	#define INCLUDE_CPP_SHARED_KEYWORD(lexem, token) token,
	#define INCLUDE_CPP_SHARED_KEYWORD_ALIAS(lexem, token) token,

	#define INCLUDE_CPP_KEYWORDS 1
	#define INCLUDE_CPP_KEYWORD(lexem, token) token,
	#define INCLUDE_CPP_KEYWORD_ALIAS(lexem, token) token,

	#include "keywords_and_tokens_raw.h"
};

struct KeywordState
{
	EToken	token;
	short	transition_index;
	char	default_char;
	short	default_next_state_index;
	EToken	identifier;
};

#pragma endregion shared code

/* main.cpp */
#pragma region main.cpp
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <assert.h>

struct State
{
	const char*	token;
	int			next_state_indices[128];
	int			transition_index;
	char		default_char;
	int			default_next_state_index;

	const char*	identifier;

	std::string	lexem;

	State(const char* token)
		: token(token), transition_index(-1), default_char(-1), default_next_state_index(-1), identifier(0)
	{
		memset(next_state_indices, -1, sizeof(next_state_indices));
	}

	bool operator==(const State& o) const
	{
		return (strcmp(token, o.token) == 0
			&& transition_index == o.transition_index
			&& default_char == o.default_char
			&& default_next_state_index == o.default_next_state_index
			&& identifier == o.identifier);
	}

	int& nextStateIndex(const char& c)
	{
		return next_state_indices[c];
	}
};

auto isIdentifierStartChar = [](char s)
{
	return (
		(s >= 'a' && s <= 'z') ||
		(s >= 'A' && s <= 'Z') ||
		(s == '_') ||
		(s == '$'));
};

void SingleCharState(std::vector<State> &states, const char* token_name, char lexem)
{
	if (states[0].next_state_indices[lexem] == -1)
	{
		states.emplace_back(token_name).lexem = lexem;
		states[0].next_state_indices[lexem] = (int)states.size() - 1;
	}
	else
	{
		assert(false);
		// todo
		// Should never happen I think?
		//states[next_state_index].token = token_name;
	}

	if (isIdentifierStartChar(lexem))
	{
		states.back().identifier = "IDENTIFIER";
	}
}

void MultiCharState(std::vector<State>& states, const char* lexem, const char* token)
{
	auto isIdentifierChar = [](char s)
	{
		return (
			(s >= 'a' && s <= 'z') ||
			(s >= 'A' && s <= 'Z') ||
			(s >= '0' && s <= '9') ||
			(s == '_') ||
			(s == '$'));
	};

	const char* identifier = 0;
	if (isIdentifierStartChar(*lexem))
		identifier = "IDENTIFIER";
	//else if (*lexem == '#')
	//	identifier = pre ? "PP_HASH" : "HASH";

	int state_index = 0;
	while (*lexem)
	{
		int& next_state_index = states[state_index].nextStateIndex(*lexem);

		if (states[state_index].nextStateIndex(*lexem) == -1)
		{
			const char * intermediateToken = 0;
			if (identifier)
			{
				intermediateToken = identifier;
			}
			else
			{
				intermediateToken = "INCOMPLETE";
			}

			auto& next_state = states.emplace_back(intermediateToken);
			next_state.identifier = identifier;
			next_state.lexem = states[state_index].lexem + *lexem;
			states[state_index].nextStateIndex(*lexem) = (int)states.size() - 1;
		}
		state_index = states[state_index].nextStateIndex(*lexem);
		++lexem;

		if (identifier && !isIdentifierChar(*lexem))
		{
			identifier = 0;
		}

	}
	states[state_index].token = token;
}

void PrintTable(const RawKeyword keywords[], int count)
{
	int i;
	unsigned char c;
	bool preprocessorMode = (keywords == s_cpp_preproc_keywords);
	std::vector<State> states;

	// null token
	states.emplace_back("NULL_TOKEN").identifier = "NULL_TOKEN";

	// identifiers
	for (c = 'a'; c <= 'z'; ++c)
		SingleCharState(states, "CHARACTER", c);
	for (c = 'A'; c <= 'Z'; ++c)
		SingleCharState(states, "CHARACTER", c);

	SingleCharState(states, "CHARACTER", '_');
	SingleCharState(states, "CHARACTER", '$');

	// add digits
	for (c = '0'; c <= '9'; ++c)
		SingleCharState(states, "DIGIT", c);

	// some floats
	for (c = '0'; c <= '9'; ++c)
	{
		MultiCharState(states, (std::string(".") + char(c)).c_str(), "FLOATING_LITERAL");
	}

	// c/c++ keywords
	for (i = 0; i < count; ++i)
	{
		MultiCharState(states, keywords[i].lexem, keywords[i].token);
	}

	// simplify table with default transitions
	int transition_index = -1;
	for (i = 0; i < states.size(); ++i)
	{
		int n = 0;
		int default_char = -1;
		for (c = 0; c < 128; ++c)
		{
			if (states[i].nextStateIndex(c) != -1)
			{
				++n;
				default_char = c;
			}
		}

		if (n)
		{
			states[i].transition_index = ++transition_index;
		}

		if (n == 1)
		{
			states[i].default_next_state_index = states[i].nextStateIndex(default_char);
			states[i].default_char = default_char;
		}
	}

#if 1 // compress table by removing duplicates
	int j, k;
	for (i = 0; i < states.size(); ++i)
	{
		for (j = i + 1; j < states.size(); ++j)
		{
			if (states[i] == states[j])
			{
				for (k = 0; k < states.size(); ++k)
				{
					if (states[k].default_next_state_index == j)
						states[k].default_next_state_index = i;

					if (states[k].default_next_state_index > j)
						--states[k].default_next_state_index;

					for (c = 0; c < 128; ++c)
					{
						if (states[k].nextStateIndex(c) == j)
							states[k].nextStateIndex(c) = i;

						if (states[k].nextStateIndex(c) > j)
							--states[k].nextStateIndex(c);
					}
				}
				states.erase(states.begin() + j);
				--j;
			}
		}
	}
#endif

	printf(
		"static const KeywordState s_%skeyword_states[] =\n"
		"{\n"
		,
		preprocessorMode ? "preproc_" : "");
	for (i = 0; i < states.size(); ++i)
	{
		int a = printf(
			(states[i].default_char != -1) ? "\t{ %-44s, %-4d, %-4d, %-4d, %-26s },   // %-4d \"%s\" -> \"%c\"\n" : "\t{ %-44s, %-4d, %-4d, %-4d, %-26s },   // %-4d \"%s\"\n",
			(std::string("EToken::") + states[i].token).c_str(),
			states[i].transition_index,
			states[i].default_char,
			states[i].default_next_state_index,
			(std::string("EToken::") + (states[i].identifier? states[i].identifier : "NULL_TOKEN")).c_str(),
			i,
			*states[i].lexem.c_str() == '\n' ? "'\\n'" : states[i].lexem.c_str(),
			states[i].default_char);

		a = 0;
	}
	printf("\n};\n"
		"\n"
		"\n");


	printf("static const short s_%skeyword_transitions[][128] =\n", preprocessorMode ? "preproc_" : "");
	printf("{\n");

	int skipped = 0;
	for (i = 0; i < states.size(); ++i)
	{
		if (i && (states[i].transition_index == -1))
		{
			skipped++;
			continue;
		}

		printf("//\t\t\t\t");
		for (c = 0; c < 16; ++c)
		{
			printf("%-5d", c);
		}
		printf("// %-4d \"%s\"\n", i - skipped, *states[i].lexem.c_str() == '\n' ? "\\n" : states[i].lexem.c_str());

		printf("\t\t\t{\n");
		for (c = 0; c < 128; ++c)
		{
			int next = states[i].next_state_indices[c];

			if (!(c % 16))
			{
				printf("/*%4d */\t ", (c / 16) * 16);
			}

			printf("%4d,", next);

			if (!((c + 1) % 16))
				printf("\n");
		}
		printf("\t\t\t},\n");
	}
	printf("};\n"
		"\n"
		"\n");

}

int main(int argc, char** argv)
{
	std::ifstream main_cpp_file(__FILE__);
	std::string main_cpp_code((std::istreambuf_iterator<char>)(main_cpp_file), std::istreambuf_iterator<char>());
	std::string shared_code = main_cpp_code.substr(0, main_cpp_code.find("/* main.cpp */"));

	auto header = R"(
// auto generated

#ifdef CONST
#error
#endif

)";
	printf(++header);
	printf(shared_code.c_str());

	PrintTable(s_cpp_preproc_keywords, sizeof(s_cpp_preproc_keywords)/sizeof(RawKeyword));
	PrintTable(s_cpp_keywords, sizeof(s_cpp_keywords) / sizeof(RawKeyword));
	return 0;
}
#pragma endregion main.cpp
