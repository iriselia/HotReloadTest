
#include "tokenizer.h"
#include "preprocessor.h"
#include "moc.h"

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

# include <iomanip>

#include <docopt.h>
// #include <CommandLineOption.h>
// #include <CommandLine.h>

#if 0

#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>

//#include <qcoreapplication.h>
#endif
namespace header_tool
{
	static const char* USAGE = R"(
Meta Object Compiler.

    Usage:
      moc [options] [-D | --define <macro=value>]... [-U | --undef <macro>]... <header>
      moc -h | --help | --version

    Arguments:
      header    Header file to read from, otherwise stdin.

    Options:
      -o, --output <file>                   Write output to file rather than stdout.
      -I, --include-dir <dir>               Add dir to the include path for header files.
      -F, --framework <framework>           Add Mac framework to the include path for header files.
      -E, --expand-only                     Expand preprocessor macros only; do not generate meta object code.
      -D, --define <macro=value>            Define macro, with optional definition.
      -U, --undef <macro>                   Undefine macro.
      -M, --meta-data <key=value>           Add key/value pair to plugin meta data.
      -c, --config                          Set configuration file to set additional options.
      -cf --compiler-flavor (msvc|unix)     Set the compiler flavor: either "msvc" or "unix".
      -n, --no-include                      Do not generate an #include statement.
      -p, --path-prefix <path>              Path prefix for included file.
      -f, --force-include <file>            Force #include <file> (overwrite default).
      -b, --prepend-include <file>          Prepend #include <file> (preserve default include).
      -in, --include <file>                 Parse <file> as an #include before the main source(s).
      -nn, --no-notes                       Do not display notes.
      -nw, --no-warnings                    Do not display warnings (implies --no-notes).
      -i, --ignore-option-clashes           Ignore all options that conflict with compilers, like -pthread
                                            conflicting with moc's -p option.
)";

	struct Args
	{
		// Positional arguments
		std::string					headerFile;

		// Options
		std::string					output;
		std::vector<std::string>	includeDirs;
		std::vector<std::string>	macFrameworks;
		bool						expandOnly;
		std::vector<std::string>	defines;
		std::vector<std::string>	undefines;
		std::vector<std::string>	metadata;
		std::string					configFile;
		std::string					compilerFlavor;
		bool						noInclude;
		std::string					pathPrefix;
		std::vector<std::string>	forceIncludes;
		std::vector<std::string>	prependIncludes;
		std::vector<std::string>	includes;
		bool						noNotes;
		bool						noWarnings;
		bool						ignoreOptionClashes;

		Args(std::map<std::string, docopt::value>& argsMap) :
			headerFile			(argsMap["<header>"].isString()					? argsMap["<header>"].asString()				: headerFile),
			includeDirs			(argsMap["--include-dir"].isStringList()		? argsMap["--include-dir"].asStringList()		: includeDirs),
			macFrameworks		(argsMap["--framework"].isStringList()			? argsMap["--framework"].asStringList()			: macFrameworks),
			output				(argsMap["--output"].isString()					? argsMap["--output"].asString()				: output),
			pathPrefix			(argsMap["--path-prefix"].isString()			? argsMap["--path-prefix"].asString()			: pathPrefix),
			expandOnly			(argsMap["--expand-only"].isBool()				? argsMap["--expand-only"].asBool()				: expandOnly),
			defines				(argsMap["--define"].isStringList()				? argsMap["--define"].asStringList()			: defines),
			undefines			(argsMap["--undef"].isStringList()				? argsMap["--undef"].asStringList()				: undefines),
			noInclude			(argsMap["--no-include"].isBool()				? argsMap["--no-include"].asBool()				: noInclude),
			forceIncludes		(argsMap["--force-include"].isStringList()		? argsMap["--force-include"].asStringList()		: forceIncludes),
			prependIncludes		(argsMap["--prepend-include"].isStringList()	? argsMap["--prepend-include"].asStringList()	: prependIncludes),
			noNotes				(argsMap["--no-notes"].isBool()					? argsMap["--no-notes"].asBool()				: noNotes),
			noWarnings			(argsMap["--no-warnings"].isBool()				? argsMap["--no-warnings"].asBool()				: noWarnings),
			ignoreOptionClashes	(argsMap["--ignore-option-clashes"].isBool()	? argsMap["--ignore-option-clashes"].asBool()	: ignoreOptionClashes)
		{
			if (!is_source_file(headerFile))
			{
				std::cout << "Invalid source file name: " << headerFile << '\n';
			}

			bool debug = true;
			if (debug)
			{
				std::stringstream ss;
				for (auto const& arg : argsMap)
				{
					ss << std::left << std::setfill(' ') << std::setw(25) << arg.first << " = " << arg.second << std::endl;
				}
				std::cout << ss.str();
			}
		}
	};

	static bool initializePpAndMoc(/*Preprocessor& pp, */Moc& moc, Args& args)
	{
		bool autoInclude = true;
		bool defaultInclude = true;

		/*
		pp.macros["Q_MOC_RUN"];
		pp.macros["__cplusplus"];

		// Don't stumble over GCC extensions
		Macro dummyVariadicFunctionMacro(true, true);
		dummyVariadicFunctionMacro.arguments.push_back(Symbol(0, PP_IDENTIFIER, "__VA_ARGS__"));
		pp.macros["__attribute__"] = dummyVariadicFunctionMacro;
		pp.macros["__declspec"] = dummyVariadicFunctionMacro;

		pp.preprocessOnly = args.expandOnly;

		for (const std::string &path : args.includes)
		{
			pp.includes.emplace_back(path);
		}
		for (const std::string &path : args.macFrameworks)
		{
			pp.includes.emplace_back(Preprocessor::IncludePath(path, true));
		}
		for (const std::string &define : args.defines)
		{
			std::string name = define;// .toLocal8Bit();
			std::string value("1");
			uint64 eq = name.find('=');
			if (eq >= 0)
			{
				value = subset(name, eq + 1);
				name = subset(name, 0, eq);
			}
			if (name.empty())
			{
				printf("Missing macro name");
				return false;
			}
			Macro macro;
			macro.symbols = tokenize(value, 1, TokenizeMode::TokenizeDefine);
			macro.symbols.pop_back(); // remove the EOF symbol
			pp.macros[name] = macro;
		}
		for (const std::string &define : args.undefines)
		{
			pp.macros.erase(define);
		}
		*/

		moc.g_include_file_stack.push(args.headerFile);
		moc.noInclude = args.noInclude;
		moc.displayNotes = !args.noNotes;
		moc.displayWarnings = moc.displayNotes = !args.noWarnings;

		autoInclude = !args.noInclude;

		if (!args.ignoreOptionClashes)
		{
			if (!args.forceIncludes.empty())
			{
				moc.noInclude = false;
				autoInclude = false;
				for (const std::string &forceInclude : args.forceIncludes)
				{
					moc.includeFiles.push_back(forceInclude);
					defaultInclude = false;
				}
			}
			for (const std::string &include : args.prependIncludes)
				moc.includeFiles.insert(moc.includeFiles.begin(), include);
			if (!args.pathPrefix.empty())
				moc.includePath = args.pathPrefix;
		}
		if (autoInclude)
		{
			std::filesystem::path headerFilePath = args.headerFile;
			moc.noInclude = (headerFilePath.has_extension() && headerFilePath.extension() != ".h");
		}


		if (args.compilerFlavor.empty() || args.compilerFlavor == "unix")
		{
			/*
			// traditional Unix compilers use both CPATH and CPLUS_INCLUDE_PATH
			// $CPATH feeds to #include <...> and #include "...", whereas
			// CPLUS_INCLUDE_PATH is equivalent to GCC's -isystem, so we parse later
			const auto cpath = qgetenv("CPATH").split(QDir::listSeparator().toLatin1());
			for (const std::string &p : cpath)
			pp.includes.emplace_back(p);
			const auto cplus_include_path = qgetenv("CPLUS_INCLUDE_PATH").split(QDir::listSeparator().toLatin1());
			for (const std::string &p : cplus_include_path)
			pp.includes.emplace_back(p);
			*/
		}
		else if (args.compilerFlavor == "msvc")
		{
			// MSVC uses one environment variable: INCLUDE
			auto ptr = getenv("INCLUDE");
			std::string msvcIncludes = ptr;
			/*
			const auto include = getenv( "INCLUDE" ).split( QDir::listSeparator().toLatin1() );
			for (const std::string &p : include)
			pp.includes.emplace_back(p);
			// */
		}
		else
		{
			printf((std::string("Unknown compiler flavor '") + args.compilerFlavor + "'; valid values are: msvc, unix.").c_str());
			return false;
		}

		//todo
		// moc.includes = pp.includes;

		if (defaultInclude)
		{
			if (moc.includePath.empty())
			{
				if (args.headerFile.size())
				{
					if (args.output.size())
					{
						/*
						This function looks at two file names and returns the name of the
						infile with a path relative to outfile.

						Examples:

						/tmp/abc, /tmp/bcd -> abc
						xyz/a/bc, xyz/b/ac -> ../a/bc
						/tmp/abc, xyz/klm -> /tmp/abc
						*/

						// TODO
						/*
						static std::string combinePath(const std::string &infile, const std::string &outfile)
						{
							std::filesystem::path cwd(std::filesystem::current_path());
							std::filesystem::path in_file_info(infile);
							std::filesystem::path out_file_info(outfile);
							std::filesystem::current_path(in_file_info);
							const std::string relative_path = out_file_info.relative_path().string();
							std::filesystem::current_path(cwd);

							#ifdef PLATFORM_WINDOWS
							// It's a system limitation.
							// It depends on the Win API function which is used by the program to open files.
							// cl apparently uses the functions that have the MAX_PATH limitation.
							if (out_file_info.string().length() + relative_path.length() + 1 >= 260)
								return in_file_info.string();
							#endif
							return relative_path;
						}
						*/
						//moc.includeFiles.push_back(combinePath(filename, output));
					}
					else
					{
						moc.includeFiles.push_back(args.headerFile);
					}
				}
			}
			else
			{
				//moc.includeFiles.push_back(combinePath(filename, filename));
			}
		}

		const auto metadata = args.metadata;
		for (const std::string &md : metadata)
		{
			uint64 split = md.find('=');
			std::string key = subset(md, 0, split);
			std::string value = subset(md, split + 1);

			if (split == std::string::npos || key.empty() || value.empty())
			{
				std::cerr << "missing key or value for option '-M'";
			}
			else if (key.find('.') != std::string::npos)
			{
				// Don't allow keys with '.' for now, since we might need this
				// format later for more advanced meta data API
				std::cerr << "A key cannot contain the letter '.' for option '-M'";
			}
			else
			{
				//QJsonArray array = moc.metaArgs.value(key);
				//array.append(value);
				//moc.metaArgs.insert(key, array);
			}
		}

		return true;
	}

	std::string composePreprocessorOutput(const std::vector<Symbol> &symbols)
	{
		std::string output;
		uint64 lineNum = 1;
		EToken last = EToken::NULL_TOKEN;
		EToken secondlast = last;
		int index = 0;
		while (index < symbols.size() - 1)
		{
			Symbol sym = symbols[index++];
			switch (sym.token)
			{
			case EToken::NEWLINE:
				secondlast = last;
				last = EToken::NEWLINE;
				output += '\n';
				lineNum++;
				if (last != EToken::NEWLINE)
				{

				}
				continue;
			case EToken::WHITESPACE:
				secondlast = last;
				last = EToken::WHITESPACE;
				output += sym.lex[sym.from];
				if (last != EToken::WHITESPACE)
				{

				}
				continue;
			case EToken::WHITESPACE_ALIAS:
				secondlast = last;
				last = EToken::WHITESPACE_ALIAS;
				output += sym.lex[sym.from];
				if (last != EToken::WHITESPACE_ALIAS)
				{

				}
				continue;
			case EToken::STRING_LITERAL:
				if (last == EToken::STRING_LITERAL)
					output.erase(output.length() - 1);
				else if (secondlast == EToken::STRING_LITERAL && last == EToken::WHITESPACE)
					output.erase(output.length() - 2);
				else
					break;
				output += subset(sym.lexem(), 1);
				secondlast = last;
				last = EToken::STRING_LITERAL;
				continue;
			case EToken::MOC_INCLUDE_BEGIN:
				lineNum = 0;
				continue;
			case EToken::MOC_INCLUDE_END:
				lineNum = sym.line_num;
				continue;
			default:
				break;
			}
			secondlast = last;
			last = sym.token;

			const int64 padding = sym.line_num - lineNum;
			if (padding > 0)
			{
				output.resize(output.size() + padding);
				memset((void*)(output.data() + output.size() - padding), '\n', padding);
				lineNum = sym.line_num;
			}

			output += sym.lexem();
		}

		return output;
	}


	static int runMoc(int argc, char **argv)
	{
		std::map<std::string, docopt::value> argsMap = docopt::docopt(++USAGE, { argv + 1, argv + argc }, true, "Meta Object Compiler 1.0");
		if (argsMap.empty())
		{
			return false;
		}

		Args args(argsMap);

		// Preprocessor pp;
		Moc moc;
		initializePpAndMoc(/*pp, */moc, args);
		std::string input;

		if (args.headerFile.empty())
		{
			args.headerFile = "standard input";
			std::istreambuf_iterator<char> begin(std::cin), end;
			input = std::string(begin, end);
		}
		else
		{
			moc.filename = args.headerFile;

			std::ifstream ifs;
			ifs.open(moc.filename, std::iostream::binary);
			if (ifs.is_open())
			{
				ifs.seekg(0, ifs.end);
				uint64 size = ifs.tellg();
				ifs.seekg(0, ifs.beg);

				if (size == 0)
				{
					fprintf(stderr, "moc: %s: File Size is 0\n", args.headerFile.c_str());
				}

				input.resize(size);

				if (input.capacity() >= size)
				{
					ifs.read(&input.front(), size);
				}
			}
			else
			{
				fprintf(stderr, "moc: %s: No such file\n", args.headerFile.c_str());
				return 1;
			}
		}

		for (const std::string &includeName : args.includes)
		{
			std::string rawName = resolveInclude(includeName, moc.filename);
			if (rawName.empty())
			{
				fprintf(stderr, "Warning: Failed to resolve include \"%s\" for moc file %s\n",
					includeName.data(), moc.filename.empty() ? "<standard input>" : moc.filename.data());
			}
			else
			{
				FILE* f = fopen(rawName.c_str(), "w");
				if (f)
				{
					moc.symbols.emplace_back(0, EToken::MOC_INCLUDE_BEGIN, rawName);
					auto symbols = tokenize(input, args.expandOnly);
					auto temp = preprocess(rawName, f, symbols, args.expandOnly);
					moc.symbols.insert(moc.symbols.end(), temp.begin(), temp.end());
					moc.symbols.emplace_back(0, EToken::MOC_INCLUDE_END, rawName);
				}
				else
				{
					fprintf(stderr, "Warning: Cannot open %s included by moc file %s: %s\n",
						rawName.data(),
						moc.filename.empty() ? "<standard input>" : moc.filename.data(), "error"
					/*f.errorString().toLocal8Bit().constData()*/);
				}
			}
		}

		std::vector<Symbol> symbols;


		symbols = tokenize(input, args.expandOnly);
		//symbols = preprocess(moc.filename, in, symbols, args.expandOnly);

		if (!args.expandOnly)
		{
			moc.parse(symbols);
		}

		// 3. and output meta object code
		{
			FILE* out = stdout;

			if (args.output.size())
			{ // output file specified
#if defined(_MSC_VER) && _MSC_VER >= 1400
				if (fopen_s(&out, args.output.c_str(), "w"))
#else
				out = fopen(output).constData(), "w"); // create output file
				if (!out)
#endif
				{
					fprintf(stderr, "moc: Cannot create %s\n", args.output.c_str());
					return 1;
				}
			}

			// original: if (!args.expandOnly)
			if (args.expandOnly)
			{
				fprintf(out, "%s\n", composePreprocessorOutput(symbols).data());
			}
			else
			{
				if (moc.classList.empty())
					moc.note("No relevant classes found. No output generated.");
				else
					moc.generate(out);
			}

			if (args.output.size())
				fclose(out);
		}

		return 0;
	}
}

int main(int _argc, char **_argv)
{
	return header_tool::runMoc(_argc, _argv);
}