#include "moc.h"
#include "generator.h"
//#include "qdatetime.h"
#include "utils.h"
//#include <QtCore/qfile.h>
//#include <QtCore/qfileinfo.h>
//#include <QtCore/qdir.h>

// for normalizeTypeInternal
//#include <private/qmetaobject_moc_p.h>
namespace header_tool
{
	// This function is shared with moc.cpp. This file should be included where needed.
	static std::string normalizeTypeInternal(const char *t, const char *e, bool fixScope = false, bool adjustConst = true)
	{
		size_t len = e - t;
		/*
		Convert 'char const *' into 'const char *'. Start at index 1,
		not 0, because 'const char *' is already OK.
		*/
		std::string constbuf;
		for (int i = 1; i < len; i++) {
			if ( t[i] == 'c'
				&& strncmp(t + i + 1, "onst", 4) == 0
				&& (i + 5 >= len || !is_ident_char(t[i + 5]))
				&& !is_ident_char(t[i-1])
				) {
				constbuf = std::string(t, len);
				if (is_space(t[i-1]))
					constbuf.erase(i-1, 6);
				else
					constbuf.erase(i, 5);
				constbuf.insert(0, "const ");
				t = constbuf.data();
				e = constbuf.data() + constbuf.length();
				break;
			}
			/*
			We mustn't convert 'char * const *' into 'const char **'
			and we must beware of 'Bar<const Bla>'.
			*/
			if (t[i] == '&' || t[i] == '*' ||t[i] == '<')
				break;
		}
		if (adjustConst && e > t + 6 && strncmp("const ", t, 6) == 0) {
			if (*(e-1) == '&') { // treat const reference as value
				t += 6;
				--e;
			} else if (is_ident_char(*(e-1)) || *(e-1) == '>') { // treat const value as value
				t += 6;
			}
		}
		std::string result;
		result.reserve(len);

#if 1
		// consume initial 'const '
		if (strncmp("const ", t, 6) == 0) {
			t+= 6;
			result += "const ";
		}
#endif

		// some type substitutions for 'unsigned x'
		if (strncmp("unsigned", t, 8) == 0) {
			// make sure "unsigned" is an isolated word before making substitutions
			if (!t[8] || !is_ident_char(t[8])) {
				if (strncmp(" int", t+8, 4) == 0) {
					t += 8+4;
					result += "uint";
				} else if (strncmp(" long", t+8, 5) == 0) {
					if ((strlen(t + 8 + 5) < 4 || strncmp(t + 8 + 5, " int", 4) != 0) // preserve '[unsigned] long int'
						&& (strlen(t + 8 + 5) < 5 || strncmp(t + 8 + 5, " long", 5) != 0) // preserve '[unsigned] long long'
						) {
						t += 8+5;
						result += "ulong";
					}
				} else if (strncmp(" short", t+8, 6) != 0  // preserve unsigned short
					&& strncmp(" char", t+8, 5) != 0) {    // preserve unsigned char
														   //  treat rest (unsigned) as uint
					t += 8;
					result += "uint";
				}
			}
		} else {
			// discard 'struct', 'class', and 'enum'; they are optional
			// and we don't want them in the normalized signature
			struct {
				const char *keyword;
				int len;
			} optional[] = {
				{ "struct ", 7 },
				{ "class ", 6 },
				{ "enum ", 5 },
				{ 0, 0 }
			};
			int i = 0;
			do {
				if (strncmp(optional[i].keyword, t, optional[i].len) == 0) {
					t += optional[i].len;
					break;
				}
			} while (optional[++i].keyword != 0);
		}

		bool star = false;
		while (t != e) {
			char c = *t++;
			if (fixScope && c == ':' && *t == ':' ) {
				++t;
				c = *t++;
				size_t i = result.size() - 1;
				while (i >= 0 && is_ident_char(result.at(i)))
					--i;
				result.resize(i + 1);
			}
			star = star || c == '*';
			result += c;
			if (c == '<') {
				//template recursion
				const char* tt = t;
				int templdepth = 1;
				int scopeDepth = 0;
				while (t != e) {
					c = *t++;
					if (c == '{' || c == '(' || c == '[')
						++scopeDepth;
					if (c == '}' || c == ')' || c == ']')
						--scopeDepth;
					if (scopeDepth == 0) {
						if (c == '<')
							++templdepth;
						if (c == '>')
							--templdepth;
						if (templdepth == 0 || (templdepth == 1 && c == ',')) {
							result += normalizeTypeInternal(tt, t-1, fixScope, false);
							result += c;
							if (templdepth == 0) {
								if (*t == '>')
									result += ' '; // avoid >>
								break;
							}
							tt = t;
						}
					}
				}
			}

			// cv qualifers can appear after the type as well
			if (!is_ident_char(c) && t != e && (e - t >= 5 && strncmp("const", t, 5) == 0)
				&& (e - t == 5 || !is_ident_char(t[5]))) {
				t += 5;
				while (t != e && is_space(*t))
					++t;
				if (adjustConst && t != e && *t == '&') {
					// treat const ref as value
					++t;
				} else if (adjustConst && !star) {
					// treat const as value
				} else if (!star) {
					// move const to the front (but not if const comes after a *)
					result.insert(0, "const ");
				} else {
					// keep const after a *
					result += "const";
				}
			}
		}

		return result;
	}

	// only moc needs this function
	static std::string normalizeType(const std::string &ba, bool fixScope = false)
	{
		const char *s = ba.data();
		size_t len = ba.size();
		char stackbuf[64];
		char *buf = (len >= 64 ? new char[len + 1] : stackbuf);
		char *data = buf;
		char last = 0;
		while (*s && is_space(*s))
			s++;
		while (*s)
		{
			while (*s && !is_space(*s))
				last = *data++ = *s++;
			while (*s && is_space(*s))
				s++;
			if (*s && ((is_ident_char(*s) && is_ident_char(last))
				|| ((*s == ':') && (last == '<'))))
			{
				last = *data++ = ' ';
			}
		}
		*data = '\0';
		std::string result = normalizeTypeInternal(buf, data, fixScope);
		if (buf != stackbuf)
			delete[] buf;
		return result;
	}

	bool Moc::parseClassHead(ClassDef *def)
	{
		// figure out whether this is a class declaration, or only a
		// forward or variable declaration.
		int i = 0;
		EToken token;
		do
		{
			token = lookup(i++);
			if (token == EToken::COLON || token == EToken::LBRACE)
				break;
			if (token == EToken::SEMIC || token == EToken::RANGLE)
				return false;
		}
		while (token != EToken::NULL_TOKEN);

		if (!test(EToken::IDENTIFIER)) // typedef struct { ... }
			return false;
		std::string name = lexem();

		// support "class IDENT name" and "class IDENT(IDENT) name"
		// also support "class IDENT name (final|sealed|Q_DECL_FINAL)"
		if (test(EToken::LPAREN))
		{
			until(EToken::RPAREN);
			if (!test(EToken::IDENTIFIER))
				return false;
			name = lexem();
		}
		else  if (test(EToken::IDENTIFIER))
		{
			const std::string lex = lexem();
			if (lex != "final" && lex != "sealed" && lex != "Q_DECL_FINAL")
				name = lex;
		}

		def->qualified += name;
		while (test(EToken::SCOPE))
		{
			def->qualified += lexem();
			if (test(EToken::IDENTIFIER))
			{
				name = lexem();
				def->qualified += name;
			}
		}
		def->classname = name;

		if (test(EToken::IDENTIFIER))
		{
			const std::string lex = lexem();
			if (lex != "final" && lex != "sealed" && lex != "Q_DECL_FINAL")
				return false;
		}

		if (test(EToken::COLON))
		{
			do
			{
				test(EToken::VIRTUAL);
				FunctionDef::Access access = FunctionDef::Public;
				if (test(EToken::PRIVATE))
					access = FunctionDef::Private;
				else if (test(EToken::PROTECTED))
					access = FunctionDef::Protected;
				else
					test(EToken::PUBLIC);
				test(EToken::VIRTUAL);
				const std::string type = parseType().name;
				// ignore the 'class Foo : BAR(Baz)' case
				if (test(EToken::LPAREN))
				{
					until(EToken::RPAREN);
				}
				else
				{
					def->superclassList.emplace_back(type, access);
				}
			}
			while (test(EToken::COMMA));

			if (!def->superclassList.empty()
				&& knownGadgets.find(std::get<0>(def->superclassList.front())) != knownGadgets.end())
			{
				// Q_GADGET subclasses are treated as Q_GADGETs
				knownGadgets.insert_or_assign(def->classname, def->qualified);
				knownGadgets.insert_or_assign(def->qualified, def->qualified);
			}
		}
		if (!test(EToken::LBRACE))
			return false;
		def->begin = index - 1;
		bool foundRBrace = until(EToken::RBRACE);
		def->end = index;
		index = def->begin + 1;
		return foundRBrace;
	}

	Type Moc::parseType()
	{
		Type type;
		bool hasSignedOrUnsigned = false;
		bool isVoid = false;
		type.firstToken = lookup();
		for (;;)
		{
			switch (next())
			{
				case EToken::SIGNED:
				case EToken::UNSIGNED:
					hasSignedOrUnsigned = true;
					Q_FALLTHROUGH();
				case EToken::CONST:
				case EToken::VOLATILE:
					type.name += lexem();
					type.name += ' ';
					if (lookup(0) == EToken::VOLATILE)
						type.isVolatile = true;
					continue;
				case EToken::Q_MOC_COMPAT_TOKEN:
				case EToken::Q_INVOKABLE_TOKEN:
				case EToken::Q_SCRIPTABLE_TOKEN:
				case EToken::Q_SIGNALS_TOKEN:
				case EToken::Q_SLOTS_TOKEN:
				case EToken::Q_SIGNAL_TOKEN:
				case EToken::Q_SLOT_TOKEN:
					type.name += lexem();
					return type;
				case EToken::NULL_TOKEN:
					return type;
				default:
					prev();
					break;
			}
			break;
		}
		test(EToken::ENUM) || test(EToken::CLASS) || test(EToken::STRUCT);
		for (;;)
		{
			switch (next())
			{
				case EToken::IDENTIFIER:
					// void mySlot(unsigned myArg)
					if (hasSignedOrUnsigned)
					{
						prev();
						break;
					}
					Q_FALLTHROUGH();
				case EToken::CHAR:
				case EToken::SHORT:
				case EToken::INT:
				case EToken::LONG:
					type.name += lexem();
					// preserve '[unsigned] long long', 'short int', 'long int', 'long double'
					if (test(EToken::LONG) || test(EToken::INT) || test(EToken::DOUBLE))
					{
						type.name += ' ';
						prev();
						continue;
					}
					break;
				case EToken::FLOAT:
				case EToken::DOUBLE:
				case EToken::VOID:
				case EToken::BOOL:
					type.name += lexem();
					isVoid |= (lookup(0) == EToken::VOID);
					break;
				case EToken::NULL_TOKEN:
					return type;
				default:
					prev();
					;
			}
			if (test(EToken::LANGLE))
			{
				if (type.name.empty())
				{
					// '<' cannot start a type
					return type;
				}
				type.name += lexemUntil(EToken::RANGLE);
			}
			if (test(EToken::SCOPE))
			{
				type.name += lexem();
				type.isScoped = true;
			}
			else
			{
				break;
			}
		}
		while (test(EToken::CONST) || test(EToken::VOLATILE) || test(EToken::SIGNED) || test(EToken::UNSIGNED)
			|| test(EToken::STAR) || test(EToken::AND) || test(EToken::ANDAND))
		{
			type.name += ' ';
			type.name += lexem();
			if (lookup(0) == EToken::AND)
				type.referenceType = Type::Reference;
			else if (lookup(0) == EToken::ANDAND)
				type.referenceType = Type::RValueReference;
			else if (lookup(0) == EToken::STAR)
				type.referenceType = Type::Pointer;
		}
		type.rawName = type.name;
		// transform stupid things like 'const void' or 'void const' into 'void'
		if (isVoid && type.referenceType == Type::NoReference)
		{
			type.name = "void";
		}
		return type;
	}

	bool Moc::parseEnum(EnumDef *def)
	{
		bool isTypdefEnum = false; // typedef enum { ... } Foo;

		if (test(EToken::CLASS))
			def->isEnumClass = true;

		if (test(EToken::IDENTIFIER))
		{
			def->name = lexem();
		}
		else
		{
			if (lookup(-1) != EToken::TYPEDEF)
				return false; // anonymous enum
			isTypdefEnum = true;
		}
		if (test(EToken::COLON))
		{ // C++11 strongly typed enum
// enum Foo : unsigned long { ... };
			parseType(); //ignore the result
		}
		if (!test(EToken::LBRACE))
			return false;
		do
		{
			if (lookup() == EToken::RBRACE) // accept trailing comma
				break;
			next(EToken::IDENTIFIER);
			def->values.push_back(lexem());
		}
		while (test(EToken::EQ) ? until(EToken::COMMA) : test(EToken::COMMA));
		next(EToken::RBRACE);
		if (isTypdefEnum)
		{
			if (!test(EToken::IDENTIFIER))
				return false;
			def->name = lexem();
		}
		return true;
	}

	void Moc::parseFunctionArguments(FunctionDef *def)
	{
		// todo
		//Q_UNUSED(def);
		while (hasNext())
		{
			ArgumentDef  arg;
			arg.type = parseType();
			if (arg.type.name == "void")
				break;
			if (test(EToken::IDENTIFIER))
				arg.name = lexem();
			while (test(EToken::LBRACK))
			{
				arg.rightType += lexemUntil(EToken::RBRACK);
			}
			if (test(EToken::CONST) || test(EToken::VOLATILE))
			{
				arg.rightType += ' ';
				arg.rightType += lexem();
			}
			arg.normalizedType = normalizeType(std::string(arg.type.name + ' ' + arg.rightType));
			arg.typeNameForCast = normalizeType(std::string(noRef(arg.type.name) + "(*)" + arg.rightType));
			if (test(EToken::EQ))
				arg.isDefault = true;
			def->arguments.push_back(arg);
			if (!until(EToken::COMMA))
				break;
		}

		if (!def->arguments.empty()
			&& def->arguments.back().normalizedType == "QPrivateSignal")
		{
			def->arguments.pop_back();
			def->isPrivateSignal = true;
		}
	}

	bool Moc::testFunctionAttribute(FunctionDef *def)
	{
		if (index < symbols.size() && testFunctionAttribute(symbols.at(index).token, def))
		{
			++index;
			return true;
		}
		return false;
	}

	bool Moc::testFunctionAttribute(EToken tok, FunctionDef *def)
	{
		switch (tok)
		{
			case EToken::Q_MOC_COMPAT_TOKEN:
				def->isCompat = true;
				return true;
			case EToken::Q_INVOKABLE_TOKEN:
				def->isInvokable = true;
				return true;
			case EToken::Q_SIGNAL_TOKEN:
				def->isSignal = true;
				return true;
			case EToken::Q_SLOT_TOKEN:
				def->isSlot = true;
				return true;
			case EToken::Q_SCRIPTABLE_TOKEN:
				def->isInvokable = def->isScriptable = true;
				return true;
			default: break;
		}
		return false;
	}

	bool Moc::testFunctionRevision(FunctionDef *def)
	{
		if (test(EToken::Q_REVISION_TOKEN))
		{
			next(EToken::LPAREN);
			std::string revision = lexemUntil(EToken::RPAREN);
			revision.erase(0, 1);
			revision.erase(revision.back(), 1);
			bool ok = false;

			ok = true;
			def->revision = std::stoi(revision);
			if (!ok || def->revision < 0)
				error("Invalid revision");
			return true;
		}

		return false;
	}

	// returns false if the function should be ignored
	bool Moc::parseFunction(FunctionDef *def, bool inMacro)
	{
		def->isVirtual = false;
		def->isStatic = false;
		//skip modifiers and attributes
		while (test(EToken::INLINE) || (test(EToken::STATIC) && (def->isStatic = true) == true) ||
			(test(EToken::VIRTUAL) && (def->isVirtual = true) == true) //mark as virtual
			|| testFunctionAttribute(def) || testFunctionRevision(def))
		{
		}
		bool templateFunction = (lookup() == EToken::TEMPLATE);
		def->type = parseType();
		if (def->type.name.empty())
		{
			if (templateFunction)
				error("Template function as signal or slot");
			else
				error();
		}
		bool scopedFunctionName = false;
		if (test(EToken::LPAREN))
		{
			def->name = def->type.name;
			scopedFunctionName = def->type.isScoped;
			def->type = Type("int");
		}
		else
		{
			Type tempType = parseType();;
			while (!tempType.name.empty() && lookup() != EToken::LPAREN)
			{
				if (testFunctionAttribute(def->type.firstToken, def))
					; // fine
				else if (def->type.firstToken == EToken::Q_SIGNALS_TOKEN)
					error();
				else if (def->type.firstToken == EToken::Q_SLOTS_TOKEN)
					error();
				else
				{
					if (!def->tag.empty())
						def->tag += ' ';
					def->tag += def->type.name;
				}
				def->type = tempType;
				tempType = parseType();
			}
			next(EToken::LPAREN, "Not a signal or slot declaration");
			def->name = tempType.name;
			scopedFunctionName = tempType.isScoped;
		}

		// we don't support references as return types, it's too dangerous
		if (def->type.referenceType == Type::Reference)
		{
			std::string rawName = def->type.rawName;
			def->type = Type("void");
			def->type.rawName = rawName;
		}

		def->normalizedType = normalizeType(def->type.name);

		if (!test(EToken::RPAREN))
		{
			parseFunctionArguments(def);
			next(EToken::RPAREN);
		}

		// support optional std::unordered_map<std::string, Macro> with compiler specific options
		while (test(EToken::IDENTIFIER))
			;

		def->isConst = test(EToken::CONST);

		while (test(EToken::IDENTIFIER))
			;

		if (inMacro)
		{
			next(EToken::RPAREN);
			prev();
		}
		else
		{
			if (test(EToken::THROW))
			{
				next(EToken::LPAREN);
				until(EToken::RPAREN);
			}
			if (test(EToken::SEMIC))
				;
			else if ((def->inlineCode = test(EToken::LBRACE)))
				until(EToken::RBRACE);
			else if ((def->isAbstract = test(EToken::EQ)))
				until(EToken::SEMIC);
			else
				error();
		}

		if (scopedFunctionName)
		{
			const std::string msg = "Function declaration " + def->name
				+ " contains extra qualification. Ignoring as signal or slot.";
			warning(msg.data());
			return false;
		}
		return true;
	}

	// like parseFunction, but never aborts with an error
	bool Moc::parseMaybeFunction(const ClassDef *cdef, FunctionDef *def)
	{
		def->isVirtual = false;
		def->isStatic = false;
		//skip modifiers and attributes
		while (test(EToken::EXPLICIT) || test(EToken::INLINE) || (test(EToken::STATIC) && (def->isStatic = true) == true) ||
			(test(EToken::VIRTUAL) && (def->isVirtual = true) == true) //mark as virtual
			|| testFunctionAttribute(def) || testFunctionRevision(def))
		{
		}
		bool tilde = test(EToken::TILDE);
		def->type = parseType();
		if (def->type.name.empty())
			return false;
		bool scopedFunctionName = false;
		if (test(EToken::LPAREN))
		{
			def->name = def->type.name;
			scopedFunctionName = def->type.isScoped;
			if (def->name == cdef->classname)
			{
				def->isDestructor = tilde;
				def->isConstructor = !tilde;
				def->type = Type();
			}
			else
			{
				def->type = Type("int");
			}
		}
		else
		{
			Type tempType = parseType();;
			while (!tempType.name.empty() && lookup() != EToken::LPAREN)
			{
				if (testFunctionAttribute(def->type.firstToken, def))
					; // fine
				else if (def->type.name == "Q_SIGNAL")
					def->isSignal = true;
				else if (def->type.name == "Q_SLOT")
					def->isSlot = true;
				else
				{
					if (!def->tag.empty())
						def->tag += ' ';
					def->tag += def->type.name;
				}
				def->type = tempType;
				tempType = parseType();
			}
			if (!test(EToken::LPAREN))
				return false;
			def->name = tempType.name;
			scopedFunctionName = tempType.isScoped;
		}

		// we don't support references as return types, it's too dangerous
		if (def->type.referenceType == Type::Reference)
		{
			std::string rawName = def->type.rawName;
			def->type = Type("void");
			def->type.rawName = rawName;
		}

		def->normalizedType = normalizeType(def->type.name);

		if (!test(EToken::RPAREN))
		{
			parseFunctionArguments(def);
			if (!test(EToken::RPAREN))
				return false;
		}
		def->isConst = test(EToken::CONST);
		if (scopedFunctionName
			&& (def->isSignal || def->isSlot || def->isInvokable))
		{
			const std::string msg = "parsemaybe: Function declaration " + def->name
				+ " contains extra qualification. Ignoring as signal or slot.";
			warning(msg.data());
			return false;
		}
		return true;
	}

	void Moc::parse(std::vector<Symbol>& symbols)
{
		std::vector<NamespaceDef> namespaceList;
		bool templateClass = false;
		uint64 index = 0;

		while (index < symbols.size())
		{
			EToken t = symbols[index++].token;
			switch (t)
			{
				case EToken::NAMESPACE:
					{
					uint64 rewind = index;
						if (test(EToken::IDENTIFIER))
						{
							std::string nsName = lexem();
							std::vector<std::string> nested;
							while (test(EToken::SCOPE))
							{
								next(EToken::IDENTIFIER);
								nested.push_back(nsName);
								nsName = lexem();
							}
							if (test(EToken::EQ))
							{
								// namespace Foo = Bar::Baz;
								until(EToken::SEMIC);
							}
							else if (test(EToken::LPAREN))
							{
								// Ignore invalid code such as: 'namespace __identifier("x")' (QTBUG-56634)
								until(EToken::RPAREN);
							}
							else if (!test(EToken::SEMIC))
							{
								NamespaceDef def;
								def.classname = nsName;

								next(EToken::LBRACE);
								def.begin = index - 1;
								until(EToken::RBRACE);
								def.end = index;
								index = def.begin + 1;

								const bool parseNamespace = g_include_file_stack.size() <= 1;
								if (parseNamespace)
								{
									for (size_t i = namespaceList.size() - 1; i >= 0; --i)
									{
										if (inNamespace(&namespaceList.at(i)))
										{
											def.qualified.insert(0, namespaceList.at(i).classname + "::");
										}
									}
									for (const std::string &ns : nested)
									{
										NamespaceDef parentNs;
										parentNs.classname = ns;
										parentNs.qualified = def.qualified;
										def.qualified += ns + "::";
										parentNs.begin = def.begin;
										parentNs.end = def.end;
										namespaceList.push_back(parentNs);
									}
								}

								while (parseNamespace && inNamespace(&def) && hasNext())
								{
									switch (next())
									{
										case EToken::NAMESPACE:
											if (test(EToken::IDENTIFIER))
											{
												while (test(EToken::SCOPE))
													next(EToken::IDENTIFIER);
												if (test(EToken::EQ))
												{
													// namespace Foo = Bar::Baz;
													until(EToken::SEMIC);
												}
												else if (!test(EToken::SEMIC))
												{
													until(EToken::RBRACE);
												}
											}
											break;
										case EToken::Q_NAMESPACE_TOKEN:
											def.hasQNamespace = true;
											break;
										case EToken::Q_ENUMS_TOKEN:
										case EToken::Q_ENUM_NS_TOKEN:
											parseEnumOrFlag(&def, false);
											break;
										case EToken::Q_ENUM_TOKEN:
											error("Q_ENUM can't be used in a Q_NAMESPACE, use Q_ENUM_NS instead");
											break;
										case EToken::Q_FLAGS_TOKEN:
										case EToken::Q_FLAG_NS_TOKEN:
											parseEnumOrFlag(&def, true);
											break;
										case EToken::Q_FLAG_TOKEN:
											error("Q_FLAG can't be used in a Q_NAMESPACE, use Q_FLAG_NS instead");
											break;
										case EToken::Q_DECLARE_FLAGS_TOKEN:
											parseFlag(&def);
											break;
										case EToken::Q_CLASSINFO_TOKEN:
											parseClassInfo(&def);
											break;
										case EToken::ENUM:
											{
												EnumDef enumDef;
												if (parseEnum(&enumDef))
													def.enumList.push_back(enumDef);
											} break;
										case EToken::CLASS:
										case EToken::STRUCT:
											{
												ClassDef classdef;
												if (!parseClassHead(&classdef))
													continue;
												while (inClass(&classdef) && hasNext())
													next(); // consume all Q_XXXX std::unordered_map<std::string, Macro> from this class
											} break;
										default: break;
									}
								}
								namespaceList.push_back(def);
								index = rewind;
								if (!def.hasQNamespace && (!def.classInfoList.empty() || !def.enumDeclarations.empty()))
									error("Namespace declaration lacks Q_NAMESPACE macro.");
							}
						}
						break;
					}
				case EToken::SEMIC:
				case EToken::RBRACE:
					templateClass = false;
					break;
				case EToken::TEMPLATE:
					templateClass = true;
					break;
				case EToken::MOC_INCLUDE_BEGIN:
					g_include_file_stack.push(symbol().unquotedLexem());
					break;
				case EToken::MOC_INCLUDE_END:
					g_include_file_stack.pop();
					break;
				case EToken::Q_DECLARE_INTERFACE_TOKEN:
					parseDeclareInterface();
					break;
				case EToken::Q_DECLARE_METATYPE_TOKEN:
					parseDeclareMetatype();
					break;
				case EToken::USING:
					if (test(EToken::NAMESPACE))
					{
						while (test(EToken::SCOPE) || test(EToken::IDENTIFIER))
							;
						next(EToken::SEMIC);
					}
					break;
				case EToken::CLASS:
				case EToken::STRUCT:
					{
						if (g_include_file_stack.size() <= 1)
							break;

						ClassDef def;
						if (!parseClassHead(&def))
							continue;

						while (inClass(&def) && hasNext())
						{
							switch (next())
							{
								case EToken::Q_OBJECT_TOKEN:
									def.hasQObject = true;
									break;
								case EToken::Q_GADGET_TOKEN:
									def.hasQGadget = true;
									break;
								default: break;
							}
						}

						if (!def.hasQObject && !def.hasQGadget)
							continue;

						for (size_t i = namespaceList.size() - 1; i >= 0; --i)
							if (inNamespace(&namespaceList.at(i)))
							{

								def.qualified.insert(0, namespaceList.at(i).classname + "::");
							}

						std::unordered_map<std::string, std::string> &classHash = def.hasQObject ? knownQObjectClasses : knownGadgets;
						classHash.insert_or_assign(def.classname, def.qualified);
						classHash.insert_or_assign(def.qualified, def.qualified);

						continue;
					}
				default: break;
			}
			if ((t != EToken::CLASS && t != EToken::STRUCT) || g_include_file_stack.size() > 1)
				continue;
			ClassDef def;
			if (parseClassHead(&def))
			{
				FunctionDef::Access access = FunctionDef::Private;
				if ( !namespaceList.empty() )
				{
					for ( size_t i = namespaceList.size() - 1; i >= 0; --i )
						if ( inNamespace( &namespaceList.at( i ) ) )
							def.qualified.insert( 0, namespaceList.at( i ).classname + "::" );
				}
				while (inClass(&def) && hasNext())
				{
					switch ((t = next()))
					{
						case EToken::PRIVATE:
							access = FunctionDef::Private;
							if (test(EToken::Q_SIGNALS_TOKEN))
								error("Signals cannot have access specifier");
							break;
						case EToken::PROTECTED:
							access = FunctionDef::Protected;
							if (test(EToken::Q_SIGNALS_TOKEN))
								error("Signals cannot have access specifier");
							break;
						case EToken::PUBLIC:
							access = FunctionDef::Public;
							if (test(EToken::Q_SIGNALS_TOKEN))
								error("Signals cannot have access specifier");
							break;
						case EToken::CLASS:
							{
								ClassDef nestedDef;
								if (parseClassHead(&nestedDef))
								{
									while (inClass(&nestedDef) && inClass(&def))
									{
										t = next();
										if (t >= EToken::Q_META_TOKEN_BEGIN && t < EToken::Q_META_TOKEN_END)
											error("Meta object features not supported for nested classes");
									}
								}
							} break;
						case EToken::Q_SIGNALS_TOKEN:
							parseSignals(&def);
							break;
						case EToken::Q_SLOTS_TOKEN:
							switch (lookup(-1))
							{
								case EToken::PUBLIC:
								case EToken::PROTECTED:
								case EToken::PRIVATE:
									parseSlots(&def, access);
									break;
								default:
									error("Missing access specifier for slots");
							}
							break;
						case EToken::Q_OBJECT_TOKEN:
							def.hasQObject = true;
							if (templateClass)
								error("Template classes not supported by Q_OBJECT");
							if (def.classname != "Qt" && def.classname != "QObject" && def.superclassList.empty())
								error("Class contains Q_OBJECT macro but does not inherit from QObject");
							break;
						case EToken::Q_GADGET_TOKEN:
							def.hasQGadget = true;
							if (templateClass)
								error("Template classes not supported by Q_GADGET");
							break;
						case EToken::Q_PROPERTY_TOKEN:
							parseProperty(&def);
							break;
						case EToken::Q_PLUGIN_METADATA_TOKEN:
							parsePluginData(&def);
							break;
						case EToken::Q_ENUMS_TOKEN:
						case EToken::Q_ENUM_TOKEN:
							parseEnumOrFlag(&def, false);
							break;
						case EToken::Q_ENUM_NS_TOKEN:
							error("Q_ENUM_NS can't be used in a Q_OBJECT/Q_GADGET, use Q_ENUM instead");
							break;
						case EToken::Q_FLAGS_TOKEN:
						case EToken::Q_FLAG_TOKEN:
							parseEnumOrFlag(&def, true);
							break;
						case EToken::Q_FLAG_NS_TOKEN:
							error("Q_FLAG_NS can't be used in a Q_OBJECT/Q_GADGET, use Q_FLAG instead");
							break;
						case EToken::Q_DECLARE_FLAGS_TOKEN:
							parseFlag(&def);
							break;
						case EToken::Q_CLASSINFO_TOKEN:
							parseClassInfo(&def);
							break;
						case EToken::Q_INTERFACES_TOKEN:
							parseInterfaces(&def);
							break;
						case EToken::Q_PRIVATE_SLOT_TOKEN:
							parseSlotInPrivate(&def, access);
							break;
						case EToken::Q_PRIVATE_PROPERTY_TOKEN:
							parsePrivateProperty(&def);
							break;
						case EToken::ENUM:
							{
								EnumDef enumDef;
								if (parseEnum(&enumDef))
									def.enumList.push_back(enumDef);
							} break;
						case EToken::SEMIC:
						case EToken::COLON:
							break;
						default:
							FunctionDef funcDef;
							funcDef.access = access;
							uint64 rewind = index--;
							if (parseMaybeFunction(&def, &funcDef))
							{
								if (funcDef.isConstructor)
								{
									if ((access == FunctionDef::Public) && funcDef.isInvokable)
									{
										def.constructorList.push_back(funcDef);
										while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault)
										{
											funcDef.wasCloned = true;
											funcDef.arguments.pop_back();
											def.constructorList.push_back(funcDef);
										}
									}
								}
								else if (funcDef.isDestructor)
								{
									// don't care about destructors
								}
								else
								{
									if (access == FunctionDef::Public)
										def.publicList.push_back(funcDef);
									if (funcDef.isSlot)
									{
										def.slotList.push_back(funcDef);
										while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault)
										{
											funcDef.wasCloned = true;
											funcDef.arguments.pop_back();
											def.slotList.push_back(funcDef);
										}
										if (funcDef.revision > 0)
											++def.revisionedMethods;
									}
									else if (funcDef.isSignal)
									{
										def.signalList.push_back(funcDef);
										while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault)
										{
											funcDef.wasCloned = true;
											funcDef.arguments.pop_back();
											def.signalList.push_back(funcDef);
										}
										if (funcDef.revision > 0)
											++def.revisionedMethods;
									}
									else if (funcDef.isInvokable)
									{
										def.methodList.push_back(funcDef);
										while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault)
										{
											funcDef.wasCloned = true;
											funcDef.arguments.pop_back();
											def.methodList.push_back(funcDef);
										}
										if (funcDef.revision > 0)
											++def.revisionedMethods;
									}
								}
							}
							else
							{
								index = rewind;
							}
					}
				}

				next(EToken::RBRACE);

				if (!def.hasQObject && !def.hasQGadget && def.signalList.empty() && def.slotList.empty()
					&& def.propertyList.empty() && def.enumDeclarations.empty())
					continue; // no meta object code required


				if (!def.hasQObject && !def.hasQGadget)
					error("Class declaration lacks Q_OBJECT macro.");

				// TODO
				/*
				// Add meta tags to the plugin meta data:
				if (!def.pluginData.iid.empty())
					def.pluginData.metaArgs = metaArgs;
				*/

				checkSuperClasses(&def);
				checkProperties(&def);

				classList.push_back(def);
				std::unordered_map<std::string, std::string> &classHash = def.hasQObject ? knownQObjectClasses : knownGadgets;
				classHash.insert_or_assign(def.classname, def.qualified);
				classHash.insert_or_assign(def.qualified, def.qualified);
			}
		}
		for (const auto &n : namespaceList)
		{
			if (!n.hasQNamespace)
				continue;
			ClassDef def;
			static_cast<BaseDef &>(def) = static_cast<BaseDef>(n);
			def.qualified += def.classname;
			def.hasQGadget = true;
			auto it = std::find_if(classList.begin(), classList.end(), [&def](const ClassDef &val) {
				return def.classname == val.classname && def.qualified == val.qualified;
			});

			if (it != classList.end())
			{
				it->classInfoList.insert(it->classInfoList.begin(), def.classInfoList.begin(), def.classInfoList.end());
				it->enumDeclarations.insert(def.enumDeclarations.begin(), def.enumDeclarations.end());
				it->enumList.insert(it->enumList.begin(), def.enumList.begin(), def.enumList.end());
				it->flagAliases.insert(def.flagAliases.begin(), def.flagAliases.end());
			}
			else
			{
				knownGadgets.insert_or_assign(def.classname, def.qualified);
				knownGadgets.insert_or_assign(def.qualified, def.qualified);
				classList.push_back(def);
			}
		}
	}

	static bool any_type_contains(const std::vector<PropertyDef> &properties, const std::string &pattern)
	{
		for (const auto &p : properties)
		{
			if (p.type.find(pattern) != std::string::npos)
				return true;
		}
		return false;
	}

	static bool any_arg_contains(const std::vector<FunctionDef> &functions, const std::string &pattern)
	{
		for (const auto &f : functions)
		{
			for (const auto &arg : f.arguments)
			{
				if (arg.normalizedType.find(pattern) != std::string::npos)
					return true;
			}
		}
		return false;
	}

	static std::vector<std::string> make_candidates()
	{
		std::vector<std::string> result;
		// todo
		/*
		result
#define STREAM_SMART_POINTER(SMART_POINTER) << #SMART_POINTER
			QT_FOR_EACH_AUTOMATIC_TEMPLATE_SMART_POINTER(STREAM_SMART_POINTER)
#undef STREAM_SMART_POINTER
#define STREAM_1ARG_TEMPLATE(TEMPLATENAME) << #TEMPLATENAME
			QT_FOR_EACH_AUTOMATIC_TEMPLATE_1ARG(STREAM_1ARG_TEMPLATE)
#undef STREAM_1ARG_TEMPLATE
			;
			*/
		return result;
	}

	static std::vector<std::string> requiredQtContainers(const std::vector<ClassDef> &classes)
	{
		static const std::vector<std::string> candidates = make_candidates();

		std::vector<std::string> required;
		required.reserve(candidates.size());

		for (const auto &candidate : candidates)
		{
			const std::string pattern = candidate + '<';

			for (const auto &c : classes)
			{
				if (any_type_contains(c.propertyList, pattern) ||
					any_arg_contains(c.slotList, pattern) ||
					any_arg_contains(c.signalList, pattern) ||
					any_arg_contains(c.methodList, pattern))
				{
					required.push_back(candidate);
					break;
				}
			}
		}

		return required;
	}

	void Moc::generate(FILE *out)
	{
		std::string fn = filename;
		size_t i = filename.length() - 1;
		while (i > 0 && filename.at(i - 1) != '/' && filename.at(i - 1) != '\\')
			--i;                                // skip path
		if (i >= 0)
			fn = subset(filename, i);
		fprintf(out, "/****************************************************************************\n"
			"** Meta object code from reading C++ file '%s'\n**\n", fn.data());
		fprintf(out, "** Created by: The Qt Meta Object Compiler version %d (Qt %s)\n**\n", mocOutputRevision, "1.0" /* QT_VERSION_STR */);
		fprintf(out, "** WARNING! All changes made in this file will be lost!\n"
			"*****************************************************************************/\n\n");


		if (!noInclude)
		{
			if (!!includePath.size() && !(includePath.back() == '/'))
				includePath += '/';
			for (int i = 0; i < includeFiles.size(); ++i)
			{
				std::string inc = includeFiles.at(i);
				if (inc.at(0) != '<' && inc.at(0) != '"')
				{
					if (includePath.size() && includePath != "./")
						inc.insert(0, includePath);
					inc = '\"' + inc + '\"';
				}
				fprintf(out, "#include %s\n", inc.data());
			}
		}
		if (classList.size() && classList.front().classname == "Qt")
			fprintf(out, "#include <QtCore/qobject.h>\n");

		fprintf(out, "#include <QtCore/std::string.h>\n"); // For std::stringData
		fprintf(out, "#include <QtCore/qmetatype.h>\n");  // For QMetaType::Type
		if (mustIncludeQPluginH)
			fprintf(out, "#include <QtCore/qplugin.h>\n");

		const auto qtContainers = requiredQtContainers(classList);
		for (const std::string &qtContainer : qtContainers)
			fprintf(out, "#include <QtCore/%s>\n", qtContainer.data());


		fprintf(out, "#if !defined(Q_MOC_OUTPUT_REVISION)\n"
			"#error \"The header file '%s' doesn't include <QObject>.\"\n", fn.data());
		fprintf(out, "#elif Q_MOC_OUTPUT_REVISION != %d\n", mocOutputRevision);
		fprintf(out, "#error \"This file was generated using the moc from %s."
			" It\"\n#error \"cannot be used with the include files from"
			" this version of Qt.\"\n#error \"(The moc has changed too"
			" much.)\"\n", "1.0" /* QT_VERSION_STR */ );
		fprintf(out, "#endif\n\n");

		fprintf(out, "QT_BEGIN_MOC_NAMESPACE\n");
		fprintf(out, "QT_WARNING_PUSH\n");
		fprintf(out, "QT_WARNING_DISABLE_DEPRECATED\n");

		fputs("", out);
		for (i = 0; i < classList.size(); ++i)
		{
			Generator generator(&classList[i], metaTypes, knownQObjectClasses, knownGadgets, out);
			generator.generateCode();
		}
		fputs("", out);

		fprintf(out, "QT_WARNING_POP\n");
		fprintf(out, "QT_END_MOC_NAMESPACE\n");
	}

	void Moc::parseSlots(ClassDef *def, FunctionDef::Access access)
	{
		int defaultRevision = -1;
		if (test(EToken::Q_REVISION_TOKEN))
		{
			next(EToken::LPAREN);
			std::string revision = lexemUntil(EToken::RPAREN);
			revision.erase(0, 1);
			revision.erase(revision.back(), 1);
			bool ok = false;

			ok = true;
			defaultRevision = std::stoi(revision);// .toInt(&ok);
			//defaultRevision = revision.toInt(&ok);
			if (!ok || defaultRevision < 0)
				error("Invalid revision");
		}

		next(EToken::COLON);
		while (inClass(def) && hasNext())
		{
			switch (next())
			{
				case EToken::PUBLIC:
				case EToken::PROTECTED:
				case EToken::PRIVATE:
				case EToken::Q_SIGNALS_TOKEN:
				case EToken::Q_SLOTS_TOKEN:
					prev();
					return;
				case EToken::SEMIC:
					continue;
				case EToken::FRIEND:
					until(EToken::SEMIC);
					continue;
				case EToken::USING:
					error("'using' directive not supported in 'slots' section");
				default:
					prev();
			}

			FunctionDef funcDef;
			funcDef.access = access;
			if (!parseFunction(&funcDef))
				continue;
			if (funcDef.revision > 0)
			{
				++def->revisionedMethods;
			}
			else if (defaultRevision != -1)
			{
				funcDef.revision = defaultRevision;
				++def->revisionedMethods;
			}
			def->slotList.push_back(funcDef);
			while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault)
			{
				funcDef.wasCloned = true;
				funcDef.arguments.pop_back();
				def->slotList.push_back(funcDef);
			}
		}
	}

	void Moc::parseSignals(ClassDef *def)
	{
		int defaultRevision = -1;
		if (test(EToken::Q_REVISION_TOKEN))
		{
			next(EToken::LPAREN);
			std::string revision = lexemUntil(EToken::RPAREN);
			revision.erase(0, 1);
			revision.erase(revision.back(), 1);
			bool ok = false;

			ok = true;
			defaultRevision = std::stoi(revision);// .toInt(&ok);
			//defaultRevision = revision.toInt(&ok);
			if (!ok || defaultRevision < 0)
				error("Invalid revision");
		}

		next(EToken::COLON);
		while (inClass(def) && hasNext())
		{
			switch (next())
			{
				case EToken::PUBLIC:
				case EToken::PROTECTED:
				case EToken::PRIVATE:
				case EToken::Q_SIGNALS_TOKEN:
				case EToken::Q_SLOTS_TOKEN:
					prev();
					return;
				case EToken::SEMIC:
					continue;
				case EToken::FRIEND:
					until(EToken::SEMIC);
					continue;
				case EToken::USING:
					error("'using' directive not supported in 'signals' section");
				default:
					prev();
			}
			FunctionDef funcDef;
			funcDef.access = FunctionDef::Public;
			parseFunction(&funcDef);
			if (funcDef.isVirtual)
				warning("Signals cannot be declared virtual");
			if (funcDef.inlineCode)
				error("Not a signal declaration");
			if (funcDef.revision > 0)
			{
				++def->revisionedMethods;
			}
			else if (defaultRevision != -1)
			{
				funcDef.revision = defaultRevision;
				++def->revisionedMethods;
			}
			def->signalList.push_back(funcDef);
			while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault)
			{
				funcDef.wasCloned = true;
				funcDef.arguments.pop_back();
				def->signalList.push_back(funcDef);
			}
		}
	}

	void Moc::createPropertyDef(PropertyDef &propDef)
	{
		std::string type = parseType().name;
		if (type.empty())
			error();
		propDef.designable = propDef.scriptable = propDef.stored = "true";
		propDef.user = "false";
		/*
		  The Q_PROPERTY construct cannot contain any commas, since
		  commas separate macro arguments. We therefore expect users
		  to type "std::map" instead of "std::map<std::string, QVariant>". For
		  coherence, we also expect the same for
		  QValueList<QVariant>, the other template class supported by
		  QVariant.
		*/
		type = normalizeType(type);
		if (type == "std::map")
			type = "std::map<std::string,QVariant>";
		else if (type == "QValueList")
			type = "QValueList<QVariant>";
		else if (type == "LongLong")
			type = "qlonglong";
		else if (type == "ULongLong")
			type = "qulonglong";

		propDef.type = type;

		next();
		propDef.name = lexem();
		while (test(EToken::IDENTIFIER))
		{
			const std::string l = lexem();
			if (l[0] == 'C' && l == "CONSTANT")
			{
				propDef.constant = true;
				continue;
			}
			else if (l[0] == 'F' && l == "FINAL")
			{
				propDef.final = true;
				continue;
			}

			std::string v, v2;
			if (test(EToken::LPAREN))
			{
				v = lexemUntil(EToken::RPAREN);
				v = subset(v, 1, v.length() - 2); // removes the '(' and ')'
			}
			else if (test(EToken::INTEGER_LITERAL))
			{
				v = lexem();
				if (l != "REVISION")
					error(1);
			}
			else
			{
				next(EToken::IDENTIFIER);
				v = lexem();
				if (test(EToken::LPAREN))
					v2 = lexemUntil(EToken::RPAREN);
				else if (v != "true" && v != "false")
					v2 = "()";
			}
			switch (l[0])
			{
				case 'M':
					if (l == "MEMBER")
						propDef.member = v;
					else
						error(2);
					break;
				case 'R':
					if (l == "READ")
						propDef.read = v;
					else if (l == "RESET")
						propDef.reset = v + v2;
					else if (l == "REVISION")
					{
						bool ok = false;
						ok = true;

						propDef.revision = std::stoi(v); //v.toInt(&ok);
						if (!ok || propDef.revision < 0)
							error(1);
					}
					else
						error(2);
					break;
				case 'S':
					if (l == "SCRIPTABLE")
						propDef.scriptable = v + v2;
					else if (l == "STORED")
						propDef.stored = v + v2;
					else
						error(2);
					break;
				case 'W': if (l != "WRITE") error(2);
					propDef.write = v;
					break;
				case 'D': if (l != "DESIGNABLE") error(2);
					propDef.designable = v + v2;
					break;
				case 'E': if (l != "EDITABLE") error(2);
					propDef.editable = v + v2;
					break;
				case 'N': if (l != "NOTIFY") error(2);
					propDef.notify = v;
					break;
				case 'U': if (l != "USER") error(2);
					propDef.user = v + v2;
					break;
				default:
					error(2);
			}
		}
		if (propDef.read.empty() && propDef.member.empty())
		{
			const std::string msg = "Property declaration " + propDef.name
				+ " has no READ accessor function or associated MEMBER variable. The property will be invalid.";
			warning(msg.data());
		}
		if (propDef.constant && !propDef.write.empty())
		{
			const std::string msg = "Property declaration " + propDef.name
				+ " is both WRITEable and CONSTANT. CONSTANT will be ignored.";
			propDef.constant = false;
			warning(msg.data());
		}
		if (propDef.constant && !propDef.notify.empty())
		{
			const std::string msg = "Property declaration " + propDef.name
				+ " is both NOTIFYable and CONSTANT. CONSTANT will be ignored.";
			propDef.constant = false;
			warning(msg.data());
		}
	}

	void Moc::parseProperty(ClassDef *def)
	{
		next(EToken::LPAREN);
		PropertyDef propDef;
		createPropertyDef(propDef);
		next(EToken::RPAREN);

		if (!propDef.notify.empty())
			def->notifyableProperties++;
		if (propDef.revision > 0)
			++def->revisionedProperties;
		def->propertyList.push_back(propDef);
	}

	void Moc::parsePluginData(ClassDef *def)
	{
		next(EToken::LPAREN);
		std::string metaData;
		while (test(EToken::IDENTIFIER))
		{
			std::string l = lexem();
			if (l == "IID")
			{
				next(EToken::STRING_LITERAL);
				def->pluginData.iid = unquotedLexem();
			}
			else if (l == "FILE")
			{
				next(EToken::STRING_LITERAL);
				std::string metaDataFile = unquotedLexem();
				std::filesystem::path fi = g_include_file_stack.top();
				fi /= metaDataFile;

				FILE* f = fopen(fi.string().c_str(), "r");
				// QFileInfo fi(QFileInfo(std::string::fromLocal8Bit(currentFilenames.top().constData())).dir(), std::string::fromLocal8Bit(metaDataFile.constData()));
				for (int j = 0; j < includes.size() && !std::filesystem::exists(fi); ++j)
				{
					const IncludePath &p = includes.at(j);
					if (p.isFrameworkPath)
						continue;

					fi = p.path;
					fi /= metaDataFile;
					//fi.setFile(std::string::fromLocal8Bit(p.path.data()), std::string::fromLocal8Bit(metaDataFile.data()));
					// try again, maybe there's a file later in the include paths with the same name
					if (std::filesystem::is_directory(fi))
					{
						fi.clear();// = QFileInfo();
						continue;
					}
				}
				if (!std::filesystem::exists(fi))
				{
					const std::string msg = "Plugin Metadata file " + lexem()
						+ " does not exist. Declaration will be ignored";
					error(msg.data());
					return;
				}
				FILE* file = fopen(fi.string().c_str(), "r");
				//QFile file(fi.canonicalFilePath());
				if (!file) // file.open(QFile::ReadOnly))
				{
					std::string msg = "Plugin Metadata file " + lexem() + " could not be opened."; //: " + file.errorString().toUtf8();
					error(msg.data());
					return;
				}
				metaData = read_all(file);
			}
		}

		if (!metaData.empty())
		{
			/*
			def->pluginData.metaData = QJsonDocument::fromJson(metaData);
			if (!def->pluginData.metaData.isObject())
			{
				const std::string msg = "Plugin Metadata file " + lexem()
					+ " does not contain a valid JSON object. Declaration will be ignored";
				warning(msg.data());
				def->pluginData.iid = std::string();
				return;
			}
			*/
		}

		mustIncludeQPluginH = true;
		next(EToken::RPAREN);
	}

	void Moc::parsePrivateProperty(ClassDef *def)
	{
		next(EToken::LPAREN);
		PropertyDef propDef;
		next(EToken::IDENTIFIER);
		propDef.inPrivateClass = lexem();
		while (test(EToken::SCOPE))
		{
			propDef.inPrivateClass += lexem();
			next(EToken::IDENTIFIER);
			propDef.inPrivateClass += lexem();
		}
		// also allow void functions
		if (test(EToken::LPAREN))
		{
			next(EToken::RPAREN);
			propDef.inPrivateClass += "()";
		}

		next(EToken::COMMA);

		createPropertyDef(propDef);

		if (!propDef.notify.empty())
			def->notifyableProperties++;
		if (propDef.revision > 0)
			++def->revisionedProperties;

		def->propertyList.push_back(propDef);
	}

	void Moc::parseEnumOrFlag(BaseDef *def, bool isFlag)
	{
		next(EToken::LPAREN);
		std::string identifier;
		while (test(EToken::IDENTIFIER))
		{
			identifier = lexem();
			while (test(EToken::SCOPE) && test(EToken::IDENTIFIER))
			{
				identifier += "::";
				identifier += lexem();
			}
			def->enumDeclarations[identifier] = isFlag;
		}
		next(EToken::RPAREN);
	}

	void Moc::parseFlag(BaseDef *def)
	{
		next(EToken::LPAREN);
		std::string flagName, enumName;
		while (test(EToken::IDENTIFIER))
		{
			flagName = lexem();
			while (test(EToken::SCOPE) && test(EToken::IDENTIFIER))
			{
				flagName += "::";
				flagName += lexem();
			}
		}
		next(EToken::COMMA);
		while (test(EToken::IDENTIFIER))
		{
			enumName = lexem();
			while (test(EToken::SCOPE) && test(EToken::IDENTIFIER))
			{
				enumName += "::";
				enumName += lexem();
			}
		}

		def->flagAliases.insert_or_assign(enumName, flagName);
		next(EToken::RPAREN);
	}

	void Moc::parseClassInfo(BaseDef *def)
	{
		next(EToken::LPAREN);
		ClassInfoDef infoDef;
		next(EToken::STRING_LITERAL);
		infoDef.name = symbol().unquotedLexem();
		next(EToken::COMMA);
		if (test(EToken::STRING_LITERAL))
		{
			infoDef.value = symbol().unquotedLexem();
		}
		else
		{
			// support Q_CLASSINFO("help", QT_TR_NOOP("blah"))
			next(EToken::IDENTIFIER);
			next(EToken::LPAREN);
			next(EToken::STRING_LITERAL);
			infoDef.value = symbol().unquotedLexem();
			next(EToken::RPAREN);
		}
		next(EToken::RPAREN);
		def->classInfoList.push_back(infoDef);
	}

	void Moc::parseInterfaces(ClassDef *def)
	{
		next(EToken::LPAREN);
		while (test(EToken::IDENTIFIER))
		{
			std::vector<ClassDef::Interface> iface;
			iface.push_back(ClassDef::Interface(lexem()));
			while (test(EToken::SCOPE))
			{
				iface.back().className += lexem();
				next(EToken::IDENTIFIER);
				iface.back().className += lexem();
			}
			while (test(EToken::COLON))
			{
				next(EToken::IDENTIFIER);
				iface.push_back(ClassDef::Interface(lexem()));
				while (test(EToken::SCOPE))
				{
					iface.back().className += lexem();
					next(EToken::IDENTIFIER);
					iface.back().className += lexem();
				}
			}
			// resolve from classnames to interface ids
			for (int i = 0; i < iface.size(); ++i)
			{
				const std::string iid = interface2IdMap[iface.at(i).className];
				if (iid.empty())
					error("Undefined interface");

				iface[i].interfaceId = iid;
			}
			def->interfaceList.push_back(iface);
		}
		next(EToken::RPAREN);
	}

	void Moc::parseDeclareInterface()
	{
		next(EToken::LPAREN);
		std::string interface;
		next(EToken::IDENTIFIER);
		interface += lexem();
		while (test(EToken::SCOPE))
		{
			interface += lexem();
			next(EToken::IDENTIFIER);
			interface += lexem();
		}
		next(EToken::COMMA);
		std::string iid;
		if (test(EToken::STRING_LITERAL))
		{
			iid = lexem();
		}
		else
		{
			next(EToken::IDENTIFIER);
			iid = lexem();
		}
		interface2IdMap.insert_or_assign(interface, iid);
		next(EToken::RPAREN);
	}

	void Moc::parseDeclareMetatype()
	{
		next(EToken::LPAREN);
		std::string typeName = lexemUntil(EToken::RPAREN);
		typeName.erase(0, 1);
		typeName.erase(typeName.back(), 1);
		metaTypes.push_back(typeName);
	}

	void Moc::parseSlotInPrivate(ClassDef *def, FunctionDef::Access access)
	{
		next(EToken::LPAREN);
		FunctionDef funcDef;
		next(EToken::IDENTIFIER);
		funcDef.inPrivateClass = lexem();
		// also allow void functions
		if (test(EToken::LPAREN))
		{
			next(EToken::RPAREN);
			funcDef.inPrivateClass += "()";
		}
		next(EToken::COMMA);
		funcDef.access = access;
		parseFunction(&funcDef, true);
		def->slotList.push_back(funcDef);
		while (funcDef.arguments.size() > 0 && funcDef.arguments.back().isDefault)
		{
			funcDef.wasCloned = true;
			funcDef.arguments.pop_back();
			def->slotList.push_back(funcDef);
		}
		if (funcDef.revision > 0)
			++def->revisionedMethods;

	}

	std::string Moc::lexemUntil(EToken target)
	{
		uint64 from = index;
		until(target);
		std::string s;
		while (from <= index)
		{
			std::string n = symbols.at(from++ - 1).lexem();
			if (s.size() && n.size())
			{
				char prev = s.at(s.size() - 1);
				char next = n.at(0);
				if ((is_ident_char(prev) && is_ident_char(next))
					|| (prev == '<' && next == ':')
					|| (prev == '>' && next == '>'))
					s += ' ';
			}
			s += n;
		}
		return s;
	}

	bool Moc::until(EToken target)
	{
		int braceCount = 0;
		int brackCount = 0;
		int parenCount = 0;
		int angleCount = 0;
		if (index)
		{
			switch (symbols.at(index - 1).token)
			{
				case EToken::LBRACE: ++braceCount; break;
				case EToken::LBRACK: ++brackCount; break;
				case EToken::LPAREN: ++parenCount; break;
				case EToken::LANGLE: ++angleCount; break;
				default: break;
			}
		}

		//when searching commas within the default argument, we should take care of template depth (anglecount)
		// unfortunatelly, we do not have enough semantic information to know if '<' is the operator< or
		// the beginning of a template type. so we just use heuristics.
		uint64 possible = -1;

		while (index < symbols.size())
		{
			EToken t = symbols.at(index++).token;
			switch (t)
			{
				case EToken::LBRACE: ++braceCount; break;
				case EToken::RBRACE: --braceCount; break;
				case EToken::LBRACK: ++brackCount; break;
				case EToken::RBRACK: --brackCount; break;
				case EToken::LPAREN: ++parenCount; break;
				case EToken::RPAREN: --parenCount; break;
				case EToken::LANGLE:
					if (parenCount == 0 && braceCount == 0)
						++angleCount;
					break;
				case EToken::RANGLE:
					if (parenCount == 0 && braceCount == 0)
						--angleCount;
					break;
				case EToken::GTGT:
					if (parenCount == 0 && braceCount == 0)
					{
						angleCount -= 2;
						t = EToken::RANGLE;
					}
					break;
				default: break;
			}
			if (t == target
				&& braceCount <= 0
				&& brackCount <= 0
				&& parenCount <= 0
				&& (target != EToken::RANGLE || angleCount <= 0))
			{
				if (target != EToken::COMMA || angleCount <= 0)
					return true;
				possible = index;
			}

			if (target == EToken::COMMA && t == EToken::EQ && possible != -1)
			{
				index = possible;
				return true;
			}

			if (braceCount < 0 || brackCount < 0 || parenCount < 0
				|| (target == EToken::RANGLE && angleCount < 0))
			{
				--index;
				break;
			}

			if (braceCount <= 0 && t == EToken::SEMIC)
			{
				// Abort on semicolon. Allow recovering bad template parsing (QTBUG-31218)
				break;
			}
		}

		if (target == EToken::COMMA && angleCount != 0 && possible != -1)
		{
			index = possible;
			return true;
		}

		return false;
	}

	void Moc::checkSuperClasses(ClassDef *def)
	{
		const std::string firstSuperclass = std::get<0>(def->superclassList[0]);

		if (!(knownQObjectClasses.find(firstSuperclass) != knownQObjectClasses.end()))
		{
			// enable once we /require/ include paths
#if 0
			const std::string msg
				= "Class "
				+ def->className
				+ " contains the Q_OBJECT macro and inherits from "
				+ def->superclassList.value(0)
				+ " but that is not a known QObject subclass. You may get compilation errors.";
			warning(msg.constData());
#endif
			return;
		}
		for (int i = 1; i < def->superclassList.size(); ++i)
		{
			const std::string superClass = std::get<0>(def->superclassList.at(i));
			if (knownQObjectClasses.find(superClass) != knownQObjectClasses.end())
			{
				const std::string msg
					= "Class "
					+ def->classname
					+ " inherits from two QObject subclasses "
					+ firstSuperclass
					+ " and "
					+ superClass
					+ ". This is not supported!";
				warning(msg.data());
			}

			if (interface2IdMap.find(superClass) != interface2IdMap.end())
			{
				bool registeredInterface = false;
				for (int i = 0; i < def->interfaceList.size(); ++i)
					if (def->interfaceList.at(i).front().className == superClass)
					{
						registeredInterface = true;
						break;
					}

				if (!registeredInterface)
				{
					const std::string msg
						= "Class "
						+ def->classname
						+ " implements the interface "
						+ superClass
						+ " but does not list it in Q_INTERFACES. qobject_cast to "
						+ superClass
						+ " will not work!";
					warning(msg.data());
				}
			}
		}
	}

	void Moc::checkProperties(ClassDef *cdef)
	{
		//
		// specify get function, for compatibiliy we accept functions
		// returning pointers, or const char * for std::string.
		//
		std::set<std::string> definedProperties;
		for (int i = 0; i < cdef->propertyList.size(); ++i)
		{
			PropertyDef &p = cdef->propertyList[i];
			if (p.read.empty() && p.member.empty())
				continue;
			if (definedProperties.find(p.name) != definedProperties.end())
			{
				std::string msg = "The property '" + p.name + "' is defined multiple times in class " + cdef->classname + ".";
				warning(msg.data());
			}
			definedProperties.insert(p.name);

			for (int j = 0; j < cdef->publicList.size(); ++j)
			{
				const FunctionDef &f = cdef->publicList.at(j);
				if (f.name != p.read)
					continue;
				if (!f.isConst) // get  functions must be const
					continue;
				if (f.arguments.size()) // and must not take any arguments
					continue;
				PropertyDef::Specification spec = PropertyDef::ValueSpec;
				std::string tmp = f.normalizedType;
				if (p.type == "std::string" && tmp == "const char *")
					tmp = "std::string";
				if (subset(tmp, 0, 6) == "const ")
					tmp = subset(tmp, 6);
				if (p.type != tmp && tmp.back() == '*')
				{
					tmp.erase(tmp.back(), 1);
					spec = PropertyDef::PointerSpec;
				}
				else if (f.type.name.back() == '&')
				{ // raw type, not normalized type
					spec = PropertyDef::ReferenceSpec;
				}
				if (p.type != tmp)
					continue;
				p.gspec = spec;
				break;
			}
			if (!p.notify.empty())
			{
				int notifyId = -1;
				for (int j = 0; j < cdef->signalList.size(); ++j)
				{
					const FunctionDef &f = cdef->signalList.at(j);
					if (f.name != p.notify)
					{
						continue;
					}
					else
					{
						notifyId = j /* Signal indexes start from 0 */;
						break;
					}
				}
				p.notifyId = notifyId;
				if (notifyId == -1)
				{
					std::string msg = "NOTIFY signal '" + p.notify + "' of property '" + p.name
						+ "' does not exist in class " + cdef->classname + ".";
					error(msg.data());
				}
			}
		}
	}
}
