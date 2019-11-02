#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "symbols.h"
#include <list>
#include <set>
#include <string>
#include <unordered_map>
#include <stdio.h>
#include <regex>

namespace header_tool
{
	enum class TokenizeMode
	{
		CppKeyword,
		CppIdentifier,
		CppExpression,
		PreprocKeyword,
		PreprocDefineIdentifier,
		PreprocExpression,
	};

	inline std::string trim_source(const std::string& input)
	{
		std::string clean_input;
		//clean_input.resize(input.size());
		const char* data = input.data();
		const char* end = input.data() + input.size();
		char* output = const_cast<char*>(clean_input.data());

		int newlines = 0;
		/*
		while (data != end)
		{
			// Skip spaces and tabs
			//while (data != end && (*data == ' ' || *data == '\t'))
			//	data++;

			while (data != end)
			{
				if (*data == '#')
				{
					*output++ = *data++;
				}

				// handle \\\n, \\\r, and \\\r\n
				bool backslash = false;
				if (*data == '\\')
				{
					data++;
					backslash = true;
				}

				switch (*data)
				{
					case '\r':
					{
						data++;
						if (backslash)
						{
							newlines++;
						}
						else
						{
							while (newlines)
							{
								*output++ = '\n';
								--newlines;
							}
						}

						if (*(data + 1) == '\n')
						{
							data++;
						}
						break;
					}

					case '\n':
					{
						if (backslash)
						{
							newlines++;
						}
						else
						{
							if (newlines)
							{
								while (newlines)
								{
									*output++ = '\n';
									--newlines;
								}
							}
							else
							{
								*output++ = '\n';
							}
						}
						data++;
						break;
					}

					default:
					{
						if (backslash)
						{
							data--;
						}
						break;
					}
				}

				if (data == end)
					break;

				*output++ = *data++;
			}
		}
		clean_input.resize(output - clean_input.data());
		//*/

		clean_input = std::regex_replace(input, std::regex(R"(\r\n)"), "\n");

		return clean_input;
	}

	inline EToken next_token(TokenizeMode mode, const char*& data)
	{
		const KeywordState* states;
		const short (*transitions)[128];
		switch (mode)
		{
		case TokenizeMode::CppKeyword:
		case TokenizeMode::CppIdentifier:
		case TokenizeMode::CppExpression:
			states = s_keyword_states;
			transitions = s_keyword_transitions;
			break;
		case TokenizeMode::PreprocKeyword:
		case TokenizeMode::PreprocDefineIdentifier:
		case TokenizeMode::PreprocExpression:
			states = s_preproc_keyword_states;
			transitions = s_preproc_keyword_transitions;
			break;
		default:
			break;
		}

		const KeywordState* state = &states[0];
		for (;;)
		{
			if (static_cast<signed char>(*data) < 0)
			{
				assert(false);
				++data;
				continue;
			}

			const short* transition = transitions[state->transition_index];

			int next_state_index = -1;
			if (*data == state->default_char)
			{
				next_state_index = state->default_next_state_index;
			}
			else if ((state == &states[0]) || (state->transition_index != -1))
			{
				next_state_index = transition[(int)*data];
			}

			if (next_state_index == -1)
			{
				// End of token 
				break;
			}

			state = &states[next_state_index];
			++data;

			/*
			if (state->transition_index == -1)
			{
				assert(false);
				// End of token with no other possibilities
				break;
			}
			*/
		}

		switch (mode)
		{
		case TokenizeMode::PreprocKeyword:
			break;
		case TokenizeMode::PreprocDefineIdentifier:
			assert(state->token != EToken::PREPROC_DEFINED); // Reserved macro name
		default:
			if (state->identifier == EToken::IDENTIFIER)
			{
				// complete identifier
				while (is_ident_char(*data))
				{
					data++;
				}
				return state->identifier;
			}
			break;
		}

		return state->token;
	}

	inline std::vector<Symbol> tokenize_internal(std::string& input, bool preprocess = false)
	{
		if (input.empty())
			return std::vector<Symbol>();

		input = trim_source(input);

		const char* begin = input.data();
		const char* data = input.data();
		const char* lexem = input.data();
		const char* end = input.data() + input.size();

		int line_num = 1;

		std::vector<Symbol> symbols;
		// Preallocate some space to speed up the code below.
		// The magic divisor value was found by calculating the average ratio between
		// input size and the final size of symbols.
		// This yielded a value of 16.x when compiling Qt Base.
		symbols.reserve(input.size() / 16);
		begin = input.data();
		data = begin;

		TokenizeMode mode = TokenizeMode::CppKeyword;

		while (*data)
		{
			static int column = 0;

			auto push_symbol = [&](EToken token)
			{
				symbols.emplace_back(line_num, token, input, lexem - begin, data - lexem);
				column++;
				lexem = data;
				return token;
			};

			auto skip_whitespace = [&]()
			{
				int count = 0;
				while (*data && (*data == ' ' || *data == '\t'))
				{
					data++;
					lexem++;
					count++;
				}
				return count;
			};

			auto push_whitespace = [&]()
			{
				int count = 0;
				while (*data && (*data == ' ' || *data == '\t'))
				{
					symbols.emplace_back(line_num, *data == ' ' ? EToken::WHITESPACE : EToken::WHITESPACE_ALIAS, input, lexem - begin, data - lexem);
					data++;
					column++;
					lexem = data;
					count++;
				}
				return count;
			};

			lexem = data;
			EToken token = next_token(mode, data);
			if (column == 0 && token == EToken::NULL_TOKEN && *data == '#')
			{
				mode = TokenizeMode::PreprocKeyword;
				continue;
			}

			/*
			if (token == EToken::NULL_TOKEN)
			{
				assert(false);
				if (*data)
					++data;
				// an error really, but let's ignore this input
				// to not confuse moc later. However in pre-processor
				// only mode let's continue.
				if (!preprocess)
					continue;
			}
			*/

			switch (token)
			{
			case EToken::NULL_TOKEN:
				assert(false);
				if (*data)
					++data;
				break;
			case EToken::PREPROC_HASH:
			{
				// #
				push_symbol(token);
				push_whitespace();

				EToken preproc_directive = push_symbol(next_token(TokenizeMode::PreprocKeyword, data));
				switch (preproc_directive)
				{
				case EToken::PREPROC_LINE:
				case EToken::PREPROC_PRAGMA:
				{
					assert(false);
				} break;
				case EToken::PREPROC_IF:
				case EToken::PREPROC_IFDEF:
				case EToken::PREPROC_IFNDEF:
				case EToken::PREPROC_ELIF:
				case EToken::PREPROC_DEFINED:
				case EToken::PREPROC_UNDEF:
				{
					mode = TokenizeMode::PreprocExpression;
				} break;
				case EToken::PREPROC_DEFINE:
				{
					// # define
					assert(push_whitespace());
					// # define name
					push_symbol(next_token(TokenizeMode::PreprocDefineIdentifier, data));
					// # define name
					// # define name value
					// # define name (expression)
					// # define name(x, y) (expression)
					// # define name(x, ...) (expression)
					// todo: check RVO
					mode = TokenizeMode::CppExpression;
					/*
					while (*data != '\n' && *data != '\\')
					{
						push_symbol(next_token(TokenizeMode::CppExpression, data));
					}
					*/
					continue;
				} break;
				case EToken::PREPROC_INCLUDE:
				{
					// # include
					push_whitespace();
					// " or <
					token = push_symbol(next_token(TokenizeMode::PreprocKeyword, data));
					assert(token == EToken::QUOTE || token == EToken::LANGLE);
					while (*data && *data != '\n' && *data != '>' && *data != '"')
					{
						++data;
					}
					push_symbol(EToken::STRING_LITERAL);
					// " or >
					token = push_symbol(next_token(TokenizeMode::PreprocKeyword, data));
					assert(token == EToken::QUOTE || token == EToken::RANGLE);
					break;
				} break;
				case EToken::PREPROC_ERROR:
				{
					push_symbol(token);
					while (*data && *data != '\n')
					{
						++data;
					}
					push_symbol(EToken::STRING_LITERAL);
				} break;
				default:
				{

				} break;
				}
			}
			break;
			case EToken::PREPROC_HASHHASH:
			{
				if (mode == TokenizeMode::CppKeyword)
				{
					assert(false);
					continue;
				}
				break;
			} break;
			case EToken::QUOTE:
			{
				while (*data && (*data != '\"'))
				{
					if (*data == '\\')
					{
						++data;
						if (!*data) break;
					}
					++data;
				}

				if (*data)  //Skip last quote
					++data;

				token = EToken::STRING_LITERAL;
				/*
				// concatenate multi-line strings for easier
				// STRING_LITERAL handling in moc
				if (!preprocess
					&& !symbols.empty()
					&& symbols.back().token == EToken::STRING_LITERAL)
				{
					assert(false);
					const std::string newString
						= '\"'
						+ symbols.back().unquotedLexem()
						+ subset(input, lexem - begin + 1, data - lexem - 2)
						+ '\"';
					symbols.back() = Symbol(symbols.back().line_num, EToken::STRING_LITERAL, newString);
					continue;
				}
				*/
			} break;
			case EToken::SINGLEQUOTE:
			{
				// escape sequence (e.g., '\t')
				// universal character (e.g., '\u02C0').
				if (*data == '\\')
				{
					data++;
					// do-while loop to process '\'' easily
					do
					{
						data++;
					}
					while (*data && *data != '\'');
					data++;
				}
				// plain character (e.g., 'x')
				else
				{
					data += 2;
				}
				push_symbol(EToken::CHARACTER_LITERAL);
				continue;
			} break;
			case EToken::LANGLE_SCOPE:
			{
				assert(false);
				// split <:: into two tokens, < and ::
				token = EToken::LANGLE;
				data -= 2;
			} break;
			case EToken::DIGIT:
				while (is_digit_char(*data) || *data == '\'')
					++data;
				if (!*data || *data != '.')
				{
					token = EToken::INTEGER_LITERAL;
					if (data - lexem == 1 &&
						(*data == 'x' || *data == 'X')
						&& *lexem == '0')
					{
						++data;
						while (is_hex_char(*data) || *data == '\'')
							++data;
					}
					push_symbol(token);
					break;
				}
				token = EToken::FLOATING_LITERAL;
				++data;
				[[fallthrough]];
			case EToken::FLOATING_LITERAL:
				while (is_digit_char(*data) || *data == '\'')
					++data;
				if (*data == '+' || *data == '-')
					++data;
				if (*data == 'e' || *data == 'E')
				{
					++data;
					while (is_digit_char(*data) || *data == '\'')
						++data;
				}
				if (*data == 'f' || *data == 'F'
					|| *data == 'l' || *data == 'L')
					++data;
				push_symbol(token);
				break;
			case EToken::CHARACTER:
				assert(false);
				break;
			case EToken::C_COMMENT:
			{
				while (*data && (*(data - 1) != '/' || *(data - 2) != '*'))
				{
					//if (*data == '\n')
					//	++line_num;
					++data;
				}
				push_symbol(EToken::C_COMMENT);
			} break;
			case EToken::WHITESPACE:
				column--;
				push_symbol(token);
				//while (*data && (*data == ' ' || *data == '\t'))
				//	++data;
				//if (preprocess) // tokenize whitespace
				//	break;
				//continue;
				break;
			case EToken::CPP_COMMENT:
			{
				push_symbol(token);
				while (*data && *data != '\n')
				{
					++data;
				}
				push_symbol(EToken::CPP_COMMENT);
				continue;
			} break;
			case EToken::NEWLINE:
			{
				line_num++;
				push_symbol(token);
				mode = TokenizeMode::CppKeyword;
				column = 0;
				continue;
			} break;
			case EToken::BACKSLASH:
			{
				if (*data && *data == '\n')
				{
					push_symbol(token);
					continue;
				}
				else
				{
					assert(false);
				}
			} break;
			/*
			case EToken::LANGLE:
				if (mode != TokenizeMode::TokenizeInclude)
					break;
				token = EToken::STRING_LITERAL;
				while (*data && *data != '\n' && *(data - 1) != '>')
					++data;
				break;
			*/
			default:
			{
				push_symbol(token);
			} break;
			}
		}
		symbols.emplace_back(); // eof symbol
		return symbols;
	}

	inline std::vector<Symbol> tokenize(std::string& input, bool preprocess = false)
	{
		return tokenize_internal(input, preprocess);
	}
} // namespace header_tool

#endif // TOKENIZER_H
