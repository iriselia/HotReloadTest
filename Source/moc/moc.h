#ifndef MOC_H
#define MOC_H

#include "parser.h"
//#include <std::vector<std::string>.h>
#include <map>
#include <tuple>
//#include <qjsondocument.h>
//#include <qjsonarray.h>
//#include <qjsonobject.h>
#include <stdio.h>
#include <ctype.h>

/*
#if defined(__cplusplus)
#if QT_HAS_CPP_ATTRIBUTE(fallthrough)
#  define Q_FALLTHROUGH() [[fallthrough]]
#elif QT_HAS_CPP_ATTRIBUTE(clang::fallthrough)
#    define Q_FALLTHROUGH() [[clang::fallthrough]]
#elif QT_HAS_CPP_ATTRIBUTE(gnu::fallthrough)
#    define Q_FALLTHROUGH() [[gnu::fallthrough]]
#endif
#endif
*/
#ifndef Q_FALLTHROUGH
#  if (defined(Q_CC_GNU) && Q_CC_GNU >= 700) && !defined(Q_CC_INTEL)
#    define Q_FALLTHROUGH() __attribute__((fallthrough))
#  else
#    define Q_FALLTHROUGH() (void)0
#endif
#endif

namespace header_tool
{

	struct QMetaObject;

	struct Type
	{
		enum ReferenceType
		{
			NoReference, Reference, RValueReference, Pointer
		};

		inline Type() : isVolatile(false), isScoped(false), firstToken(EToken::NULL_TOKEN), referenceType(NoReference)
		{}
		inline explicit Type(const std::string &_name)
			: name(_name), rawName(name), isVolatile(false), isScoped(false), firstToken(EToken::NULL_TOKEN), referenceType(NoReference)
		{}
		std::string name;
		//std::string name;
		
		//When used as a return type, the type name may be modified to remove the references.
		// rawName is the type as found in the function signature
		std::string rawName;
		//std::string rawName;
		uint32 isVolatile : 1;
		uint32 isScoped : 1;
		EToken firstToken;
		ReferenceType referenceType;
	};
	//Q_DECLARE_TYPEINFO(Type, Q_MOVABLE_TYPE);

	struct EnumDef
	{
		std::string name;
		std::vector<std::string> values;
		bool isEnumClass; // c++11 enum class
		EnumDef() : isEnumClass(false)
		{}
	};
	//Q_DECLARE_TYPEINFO(EnumDef, Q_MOVABLE_TYPE);

	struct ArgumentDef
	{
		ArgumentDef() : isDefault(false)
		{}
		Type type;
		std::string rightType, normalizedType, name;
		std::string typeNameForCast; // type name to be used in cast from void * in metacall
		bool isDefault;
	};
	//Q_DECLARE_TYPEINFO(ArgumentDef, Q_MOVABLE_TYPE);

	struct FunctionDef
	{
		FunctionDef() : returnTypeIsVolatile(false), access(Private), isConst(false), isVirtual(false), isStatic(false),
			inlineCode(false), wasCloned(false), isCompat(false), isInvokable(false),
			isScriptable(false), isSlot(false), isSignal(false), isPrivateSignal(false),
			isConstructor(false), isDestructor(false), isAbstract(false), revision(0)
		{}
		Type type;
		std::string normalizedType;
		std::string tag;
		std::string name;
		bool returnTypeIsVolatile;

		std::vector<ArgumentDef> arguments;

		enum Access
		{
			Private, Protected, Public
		};
		Access access;
		bool isConst;
		bool isVirtual;
		bool isStatic;
		bool inlineCode;
		bool wasCloned;

		std::string inPrivateClass;
		bool isCompat;
		bool isInvokable;
		bool isScriptable;
		bool isSlot;
		bool isSignal;
		bool isPrivateSignal;
		bool isConstructor;
		bool isDestructor;
		bool isAbstract;

		int revision;
	};
	//Q_DECLARE_TYPEINFO(FunctionDef, Q_MOVABLE_TYPE);

	struct PropertyDef
	{
		PropertyDef() :notifyId(-1), constant(false), final(false), gspec(ValueSpec), revision(0)
		{}
		std::string name, type, member, read, write, reset, designable, scriptable, editable, stored, user, notify, inPrivateClass;
		int notifyId;
		bool constant;
		bool final;
		enum Specification
		{
			ValueSpec, ReferenceSpec, PointerSpec
		};
		Specification gspec;
		bool stdCppSet() const
		{
			std::string s("set");
			s += toupper(name[0]);
			s.append(name.begin() + 1, name.end());
			return (s == write);
		}
		int revision;
	};
	//Q_DECLARE_TYPEINFO(PropertyDef, Q_MOVABLE_TYPE);


	struct ClassInfoDef
	{
		std::string name;
		std::string value;
	};
	//Q_DECLARE_TYPEINFO(ClassInfoDef, Q_MOVABLE_TYPE);

	struct BaseDef
	{
		std::string classname;
		std::string qualified;
		std::vector<ClassInfoDef> classInfoList;
		std::map<std::string, bool> enumDeclarations;
		std::vector<EnumDef> enumList;
		std::map<std::string, std::string> flagAliases;
		uint64 begin = 0;
		uint64 end = 0;
	};

	struct ClassDef : BaseDef
	{
		std::vector<std::tuple<std::string, FunctionDef::Access> > superclassList;

		struct Interface
		{
			Interface()
			{} // for std::vector, don't use
			inline explicit Interface(const std::string &_className)
				: className(_className)
			{}
			std::string className;
			std::string interfaceId;
		};
		std::vector<std::vector<Interface> >interfaceList;

		bool hasQObject = false;
		bool hasQGadget = false;

		struct PluginData
		{
			std::string iid;
			// TODO:
			//std::map<std::string, QJsonArray> metaArgs;
			//QJsonDocument metaData;
		} pluginData;

		std::vector<FunctionDef> constructorList;
		std::vector<FunctionDef> signalList, slotList, methodList, publicList;
		int notifyableProperties = 0;
		std::vector<PropertyDef> propertyList;
		int revisionedMethods = 0;
		int revisionedProperties = 0;

	};
	//Q_DECLARE_TYPEINFO(ClassDef, Q_MOVABLE_TYPE);
	//Q_DECLARE_TYPEINFO(ClassDef::Interface, Q_MOVABLE_TYPE);

	struct NamespaceDef : BaseDef
	{
		bool hasQNamespace = false;
	};
	//Q_DECLARE_TYPEINFO(NamespaceDef, Q_MOVABLE_TYPE);

	class Moc : public Parser
	{
	public:
		Moc()
			: noInclude(false), mustIncludeQPluginH(false)
		{}

		std::string										filename;

		bool											noInclude;
		bool											mustIncludeQPluginH;
		std::string										includePath;
		std::vector<std::string>						includeFiles;
		std::vector<ClassDef>							classList;
		std::map<std::string, std::string>				interface2IdMap;
		std::vector<std::string>						metaTypes;
		// map from class name to fully qualified name
		std::unordered_map<std::string, std::string>	knownQObjectClasses;
		std::unordered_map<std::string, std::string>	knownGadgets;
		// TODO:
		// std::map<std::string, QJsonArray> metaArgs;

		void parse(std::vector<Symbol>& symbols);
		void generate(FILE *out);

		bool parseClassHead(ClassDef *def);
		inline bool inClass(const ClassDef *def) const
		{
			return index > def->begin && index < def->end - 1;
		}

		inline bool inNamespace(const NamespaceDef *def) const
		{
			return index > def->begin && index < def->end - 1;
		}

		Type parseType();

		bool parseEnum(EnumDef *def);

		bool parseFunction(FunctionDef *def, bool inMacro = false);
		bool parseMaybeFunction(const ClassDef *cdef, FunctionDef *def);

		void parseSlots(ClassDef *def, FunctionDef::Access access);
		void parseSignals(ClassDef *def);
		void parseProperty(ClassDef *def);
		void parsePluginData(ClassDef *def);
		void createPropertyDef(PropertyDef &def);
		void parseEnumOrFlag(BaseDef *def, bool isFlag);
		void parseFlag(BaseDef *def);
		void parseClassInfo(BaseDef *def);
		void parseInterfaces(ClassDef *def);
		void parseDeclareInterface();
		void parseDeclareMetatype();
		void parseSlotInPrivate(ClassDef *def, FunctionDef::Access access);
		void parsePrivateProperty(ClassDef *def);

		void parseFunctionArguments(FunctionDef *def);

		std::string lexemUntil(EToken);
		bool until(EToken);

		// test for Q_INVOCABLE, Q_SCRIPTABLE, etc. and set the flags
		// in FunctionDef accordingly
		bool testFunctionAttribute(FunctionDef *def);
		bool testFunctionAttribute(EToken tok, FunctionDef *def);
		bool testFunctionRevision(FunctionDef *def);

		void checkSuperClasses(ClassDef *def);
		void checkProperties(ClassDef* cdef);
	};

	inline std::string noRef(const std::string &type)
	{
		if (*type.end() == '&')
		{
			if (*(type.end()--) == '&')
			{
				return type.substr(0, type.length() - 2);
			}
			return type.substr(0, type.length() - 1);
		}
		return type;
	}

}

#endif // MOC_H
