#pragma once
#ifndef UTILS_H
#define UTILS_H

//#include <QtCore/qglobal.h>
#include <string>

namespace header_tool
{
	// if the output revision changes, you MUST change it in qobjectdefs.h too
	enum
	{
		mocOutputRevision = 1 // translate moc to stl
	};

	struct sub_array
	{
		inline sub_array() :from(0), len(-1)
		{
		}
		inline sub_array(const char *s) : array(s), from(0), len(array.size())
		{
		}
		inline sub_array(const std::string &a) : array(a.begin(), a.end()), from(0), len(a.size())
		{
		}
		inline sub_array(const std::string &a, size_t from, size_t len) : array(a), from(from), len(len)
		{
		}

		std::string array;
		size_t from, len;
		inline bool operator==(const sub_array &other) const
		{
			if (len != other.len)
				return false;
			for (int i = 0; i < len; ++i)
				if (array[from + i] != other.array[other.from + i])
					return false;
			return true;
		}
	};

	static uint32 hash(const uint8 *p, size_t n)
	{
		uint32 h = 0;

		while (n--)
		{
			h = (h << 4) + *p++;
			h ^= (h & 0xf0000000) >> 23;
			h &= 0x0fffffff;
		}
		return h;
	}

	inline uint32 qHash(const sub_array &key)
	{
		return hash((uint8*)key.array.data() + key.from, key.len);
		//return std::unordered_map( QLatin1String( key.array.data() + key.from, key.len ) );
	}

	inline std::string read_all(FILE* file)
	{
		std::ifstream ifs(file);
		std::string content;
		if (file)
		{
			if (ifs.is_open())
			{
				ifs.seekg(0, ifs.end);
				size_t length = ifs.tellg();
				ifs.seekg(0, ifs.beg);

				if (length == 0)
				{
					return std::string();
				}

				content.resize(length);

				if (content.capacity() == length)
				{
					ifs.read(&content.front(), length);
				}
			}
		}

		return content;
	}

	inline std::string replace_all(std::string str, const std::string& from, const std::string& to)
	{
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}

	inline bool is_whitespace(char s)
	{
		return (s == ' ' || s == '\t' || s == '\n');
	}

	inline bool is_space(char s)
	{
		return (s == ' ' || s == '\t');
	}

	inline bool is_ident_start(char s)
	{
		return ((s >= 'a' && s <= 'z')
			|| (s >= 'A' && s <= 'Z')
			|| s == '_' || s == '$'
			);
	}

	inline bool is_ident_char(char s)
	{
		return ((s >= 'a' && s <= 'z')
			|| (s >= 'A' && s <= 'Z')
			|| (s >= '0' && s <= '9')
			|| s == '_' || s == '$'
			);
	}

	inline bool is_identifier(const char *s, uint64 len)
	{
		if (len < 1)
			return false;
		if (!is_ident_start(*s))
			return false;
		for (int i = 1; i < len; ++i)
			if (!is_ident_char(s[i]))
				return false;
		return true;
	}

	inline bool is_digit_char(char s)
	{
		return (s >= '0' && s <= '9');
	}

	inline bool is_octal_char(char s)
	{
		return (s >= '0' && s <= '7');
	}

	inline bool is_hex_char(char s)
	{
		return ((s >= 'a' && s <= 'f')
			|| (s >= 'A' && s <= 'F')
			|| (s >= '0' && s <= '9')
			);
	}

	inline const char *skipQuote(const char *data)
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
		return data;
	}

	template<typename T, typename... Args>
	T subset(const T& vector, uint64 from, uint64 len = -1)
	{
		if (from >= vector.size())
		{
			return T();
		}

		if (len < 0)
		{
			len = vector.size() - from;
		}

		if (from < 0)
		{
			len += from;
			from = 0;
		}

		if (len + from > vector.size())
		{
			len = vector.size() - from;
		}
		if (from == 0 && len == vector.size())
		{
			return vector;
		}
		return T(vector.data() + from, vector.data() + from + len);
	}
	/*
	template<typename T>
	T sub(const T& vec, size_t pos)
	{
		return sub(vec, pos, -1);
	}
	*/

	inline std::string readFile(FILE* file)
	{
		std::ifstream ifs(file);
		std::string content;
		if (file)
		{
			if (ifs.is_open())
			{
				ifs.seekg(0, ifs.end);
				uint64 length = ifs.tellg();
				ifs.seekg(0, ifs.beg);

				if (length == 0)
				{
					return std::string();
				}

				content.resize(length);

				if (content.capacity() >= length)
				{
					ifs.read(&content.front(), length);
				}
			}
		}

		return content;
	}

	#ifndef Q_FALLTHROUGH
	#  if (defined(Q_CC_GNU) && Q_CC_GNU >= 700) && !defined(Q_CC_INTEL)
	#    define Q_FALLTHROUGH() __attribute__((fallthrough))
	#  else
	#    define Q_FALLTHROUGH() (void)0
	#endif
	#endif

	inline bool path_ends_with(const std::filesystem::path::string_type & lhs, const std::filesystem::path::string_type & rhs)
	{
		if (lhs.size() < rhs.size())
		{
			return false;
		}

		for (std::size_t i = 0, s1_max_index = lhs.size() - 1, s2_max_index = rhs.size() - 1; i < s2_max_index + 1; i++)
		{
			auto& s1_next = lhs[s1_max_index - i];
			auto& s2_next = rhs[s2_max_index - i];

			if (s1_next != s2_next)
			{
				return false;
			}
		}

		return true;
	}

	inline bool is_implementation_file(const std::filesystem::path & path)
	{
		static const std::filesystem::path::string_type cplusplus = TEXT(".c++");
		static const std::filesystem::path::string_type cpp = TEXT(".cpp");
		static const std::filesystem::path::string_type cxx = TEXT(".cxx");
		static const std::filesystem::path::string_type cc = TEXT(".cc");
		static const std::filesystem::path::string_type c = TEXT(".c");

		static const std::filesystem::path::string_type pchcpp = TEXT(".pch.cpp");
		static const std::filesystem::path::string_type pchc = TEXT(".pch.c");

		auto& entry_string = path.native();
		if (path_ends_with(entry_string, cplusplus) ||
			path_ends_with(entry_string, cpp) ||
			path_ends_with(entry_string, cxx) ||
			path_ends_with(entry_string, cc) ||
			path_ends_with(entry_string, c))
		{
			if (!path_ends_with(entry_string, pchcpp) &&
				!path_ends_with(entry_string, pchc)
				// To utf 8 if on windows
				&& !std::regex_match(path.string(), std::regex(".+(?:CMake).+Modules.+", std::regex::optimize))
				&& !std::regex_match(path.string(), std::regex(".+(?:CMakeFiles).+", std::regex::optimize))
				&& !std::regex_match(path.string(), std::regex(".+(?:enc_temp_folder).+", std::regex::optimize)))
			{
				return true;
			}
		}

		return false;
	}

	inline bool is_declaration_file(const std::filesystem::path & path)
	{
		static const std::filesystem::path::string_type hpp = TEXT(".hpp");
		static const std::filesystem::path::string_type inl = TEXT(".inl");
		static const std::filesystem::path::string_type h = TEXT(".h");

		static const std::filesystem::path::string_type generatedpchh = TEXT(".generated.pch.h");
		static const std::filesystem::path::string_type generatedh = TEXT(".generated.h");

		auto& entry_string = path.native();
		if (path_ends_with(entry_string, hpp) ||
			path_ends_with(entry_string, inl) ||
			path_ends_with(entry_string, h))
		{
			if (!path_ends_with(entry_string, generatedpchh) &&
				!path_ends_with(entry_string, generatedh) &&
				!std::regex_match(path.string(), std::regex(".+(?:CMake).+Modules.+", std::regex::optimize)) &&
				!std::regex_match(path.string(), std::regex(".+(?:enc_temp_folder).+", std::regex::optimize)))
			{
				return true;
			}
		}

		return false;
	}

	inline bool is_source_file(const std::filesystem::path & path)
	{
		return is_implementation_file(path) || is_declaration_file(path);
	}
}

#endif // UTILS_H
