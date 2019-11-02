#include "generator.h"
#include "utils.h"
#if 0
#include <QtCore/metatype.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qplugin.h>

#include <private/qmetaobject_p.h> //for the flags.
#endif
#include <stdio.h>

#include <algorithm>


namespace header_tool
{
	struct metatype
	{
		/*
		static const struct
		{
			const char * typeName; int typeNameLength; int type; } types[] =
			{
				QT_FOR_EACH_STATIC_TYPE( QT_ADD_STATIC_METATYPE )
				QT_FOR_EACH_STATIC_ALIAS_TYPE( QT_ADD_STATIC_METATYPE_ALIASES_ITER )
				QT_FOR_EACH_STATIC_HACKS_TYPE( QT_ADD_STATIC_METATYPE_HACKS_ITER )
				{
				0, 0, QMetaType::UnknownType
				}
			};
		*/

		static int type( const char* type_name )
		{
			return 0;
		}

	};

	uint32 nameToBuiltinType( const std::string &name )
	{
		if ( name.empty() )
			return 0;

		/*
		uint32 tp = metatype::type( name.data() );
		return tp < uint32( metatype::User ) ? tp : uint32( metatype::UnknownType );
		*/

		// TODO:
		return 0;
	}

	/*
	  Returns \c true if the type is a built-in type.
	*/
	bool isBuiltinType( const std::string &type )
	{
		// TODO:
		/*
		int id = metatype::type( type.data() );
		if ( id == metatype::UnknownType )
			return false;
		return (id < metatype::User);
		*/
		return false;
	}

	static const char *metaTypeEnumValueString( int type )
	{
		// TODO
		/*
#define RETURN_METATYPENAME_STRING(MetaTypeName, MetaTypeId, RealType) \
    case metatype::MetaTypeName: return #MetaTypeName;

		switch ( type )
		{
			QT_FOR_EACH_STATIC_TYPE( RETURN_METATYPENAME_STRING )
		}
#undef RETURN_METATYPENAME_STRING
		*/
		return 0;
	}
#if 0
#endif

	Generator::Generator( ClassDef *classDef, const std::vector<std::string> &metaTypes, const std::unordered_map<std::string, std::string> &knownQObjectClasses, const std::unordered_map<std::string, std::string> &knownGadgets, FILE *outfile )
		: out( outfile ), cdef( classDef ), metaTypes( metaTypes ), knownQObjectClasses( knownQObjectClasses )
		, knownGadgets( knownGadgets )
	{
		if ( cdef->superclassList.size() )
			purestSuperClass = std::get<0>( cdef->superclassList.front() );
	}

	static inline uint64 lengthOfEscapeSequence( const std::string &s, uint64 i )
	{
		if ( s.at( i ) != '\\' || i >= s.length() - 1 )
			return 1;
		const uint64 startPos = i;
		++i;
		char ch = s.at( i );
		if ( ch == 'x' )
		{
			++i;
			while ( i < s.length() && is_hex_char( s.at( i ) ) )
				++i;
		}
		else if ( is_octal_char( ch ) )
		{
			while ( i < startPos + 4
				&& i < s.length()
				&& is_octal_char( s.at( i ) ) )
			{
				++i;
			}
		}
		else
		{ // single character escape sequence
			i = std::min( (uint64)i + 1, s.length() );
		}
		return i - startPos;
	}

	void Generator::strreg( const std::string &s )
	{
		if ( std::find(strings.begin(), strings.end(), s ) == strings.end() )
			strings.push_back( s );
	}

	uint64 Generator::stridx( const std::string & s )
	{
		uint64 i = std::distance(strings.begin(), std::find(strings.begin(), strings.end(), s ));
		//Q_ASSERT_X( i != -1, Q_FUNC_INFO, "We forgot to register some strings" );
		return i;
	}

	// Returns the sum of all parameters (including return type) for the given
	// \a list of methods. This is needed for calculating the size of the methods'
	// parameter type/name meta-data.
	static uint64 aggregateParameterCount( const std::vector<FunctionDef> &list )
	{
		uint64 sum = 0;
		for ( uint64 i = 0; i < list.size(); ++i )
			sum += list.at( i ).arguments.size() + 1; // +1 for return type
		return sum;
	}

	bool Generator::registerableMetaType( const std::string &propertyType )
	{
		if ( std::find(metaTypes.begin(), metaTypes.end(), propertyType) != metaTypes.end() )
			return true;

		if ( propertyType.back() == '*' )
		{
			std::string objectPointerType = propertyType;
			// The objects container stores class names, such as 'QState', 'QLabel' etc,
			// not 'QState*', 'QLabel*'. The propertyType does contain the '*', so we need
			// to chop it to find the class type in the known QObjects list.
			objectPointerType.erase( objectPointerType.back(), 1 );
			if ( knownQObjectClasses.find( objectPointerType ) !=knownQObjectClasses.end() )
				return true;
		}

		// TODO
		static const std::vector<std::string> smartPointers = std::vector<std::string>();
		/*
#define STREAM_SMART_POINTER(SMART_POINTER) << #SMART_POINTER
			QT_FOR_EACH_AUTOMATIC_TEMPLATE_SMART_POINTER( STREAM_SMART_POINTER )
#undef STREAM_SMART_POINTER
			;
		*/

		for ( const std::string &smartPointer : smartPointers )
		{
			if ( propertyType.compare(0, smartPointer.length() + 1, (smartPointer + "<")) == 0 && !(propertyType.back() == '&') )
				return knownQObjectClasses.find(subset(propertyType, smartPointer.size() + 1, propertyType.size() - smartPointer.size() - 1 - 1)) != knownQObjectClasses.end();
		}

		// TODO
		static const std::vector<std::string> oneArgTemplates = std::vector<std::string>();
		/*
#define STREAM_1ARG_TEMPLATE(TEMPLATENAME) << #TEMPLATENAME
			QT_FOR_EACH_AUTOMATIC_TEMPLATE_1ARG( STREAM_1ARG_TEMPLATE )
#undef STREAM_1ARG_TEMPLATE
			;
		*/

		for ( const std::string &oneArgTemplateType : oneArgTemplates )
		{
			if ( propertyType.compare( 0, oneArgTemplateType.length() + 1, (oneArgTemplateType + "<") ) == 0 && propertyType.back() == '>' )
			{
				const uint64 argumentSize = propertyType.size() - oneArgTemplateType.size() - 1
					// The closing '>'
					- 1
					// templates inside templates have an extra whitespace char to strip.
					- (propertyType.at( propertyType.size() - 2 ) == ' ' ? 1 : 0);
				const std::string templateArg = subset(propertyType, oneArgTemplateType.size() + 1, argumentSize );
				return isBuiltinType( templateArg ) || registerableMetaType( templateArg );
			}
		}
		return false;
	}

	/* returns \c true if name and qualifiedName refers to the same name.
	 * If qualified name is "A::B::C", it returns \c true for "C", "B::C" or "A::B::C" */
	static bool qualifiedNameEquals( const std::string &qualifiedName, const std::string &name )
	{
		if ( qualifiedName == name )
			return true;
		uint64 index = qualifiedName.find( "::" );
		if ( index == std::string::npos )
			return false;
		return qualifiedNameEquals( subset(qualifiedName, index + 2 ), name );
	}

	void Generator::generateCode()
	{
		bool isQt = (cdef->classname == "Qt");
		bool isQObject = (cdef->classname == "QObject");
		bool isConstructible = !cdef->constructorList.empty();

		// filter out undeclared enumerators and sets
		{
			std::vector<EnumDef> enumList;
			for ( int i = 0; i < cdef->enumList.size(); ++i )
			{
				EnumDef def = cdef->enumList.at( i );
				if ( cdef->enumDeclarations.find( def.name ) != cdef->enumDeclarations.end() )
				{
					enumList.push_back(def);
				}
				std::string alias = cdef->flagAliases[def.name];
				if ( cdef->enumDeclarations.find( alias ) != cdef->enumDeclarations.end())
				{
					def.name = alias;
					enumList.push_back(def);
				}
			}
			cdef->enumList = enumList;
		}

		//
		// Register all strings used in data section
		//
		strreg( cdef->qualified );
		registerClassInfoStrings();
		registerFunctionStrings( cdef->signalList );
		registerFunctionStrings( cdef->slotList );
		registerFunctionStrings( cdef->methodList );
		registerFunctionStrings( cdef->constructorList );
		registerPropertyStrings();
		registerEnumStrings();

		std::string qualifiedClassNameIdentifier = cdef->qualified;
		std::replace(qualifiedClassNameIdentifier.begin(), qualifiedClassNameIdentifier.end(), ':', '_' );

		//
		// Build stringdata struct
		//
		const int constCharArraySizeLimit = 65535;
		fprintf( out, "struct qt_meta_stringdata_%s_t {\n", qualifiedClassNameIdentifier.data() );
		fprintf( out, "    std::stringData data[%lld];\n", strings.size() );
		{
			uint64 stringDataLength = 0;
			uint64 stringDataCounter = 0;
			for ( uint64 i = 0; i < strings.size(); ++i )
			{
				uint64 thisLength = strings.at( i ).length() + 1;
				stringDataLength += thisLength;
				if ( stringDataLength / constCharArraySizeLimit )
				{
					// save previous stringdata and start computing the next one.
					fprintf( out, "    char stringdata%lld[%lld];\n", stringDataCounter++, stringDataLength - thisLength );
					stringDataLength = thisLength;
				}
			}
			fprintf( out, "    char stringdata%lld[%lld];\n", stringDataCounter, stringDataLength );

		}
		fprintf( out, "};\n" );

		// Macro that expands into a std::stringData. The offset member is
		// calculated from 1) the offset of the actual characters in the
		// stringdata.stringdata member, and 2) the stringdata.data index of the
		// std::stringData being defined. This calculation relies on the
		// std::stringData::data() implementation returning simply "this + offset".
		fprintf( out, "#define QT_MOC_LITERAL(idx, ofs, len) \\\n"
			"    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \\\n"
			"    qptrdiff(offsetof(qt_meta_stringdata_%s_t, stringdata0) + ofs \\\n"
			"        - idx * sizeof(std::stringData)) \\\n"
			"    )\n",
			qualifiedClassNameIdentifier.data() );

		fprintf( out, "static const qt_meta_stringdata_%s_t qt_meta_stringdata_%s = {\n",
			qualifiedClassNameIdentifier.data(), qualifiedClassNameIdentifier.data() );
		fprintf( out, "    {\n" );
		{
			uint64 idx = 0;
			for ( uint64 i = 0; i < strings.size(); ++i )
			{
				const std::string &str = strings.at( i );
				fprintf( out, "QT_MOC_LITERAL(%lld, %lld, %lld)", i, idx, str.length() );
				if ( i != strings.size() - 1 )
					fputc( ',', out );
				const std::string comment = str.length() > 32 ? subset(str, 0, 29 ) + "..." : str;
				fprintf( out, " // \"%s\"\n", comment.data() );
				idx += str.length() + 1;
				for ( uint64 j = 0; j < str.length(); ++j )
				{
					if ( str.at( j ) == '\\' )
					{
						uint64 cnt = lengthOfEscapeSequence( str, j ) - 1;
						idx -= cnt;
						j += cnt;
					}
				}
			}
			fprintf( out, "\n    },\n" );
		}

		//
		// Build stringdata array
		//
		fprintf( out, "    \"" );
		uint64 col = 0;
		uint64 len = 0;
		uint64 stringDataLength = 0;
		for ( uint64 i = 0; i < strings.size(); ++i )
		{
			std::string s = strings.at( i );
			len = s.length();
			stringDataLength += len + 1;
			if ( stringDataLength >= constCharArraySizeLimit )
			{
				fprintf( out, "\",\n    \"" );
				stringDataLength = len + 1;
				col = 0;
			}
			else if ( i )
				fputs( "\\0", out ); // add \0 at the end of each string

			if ( col && col + len >= 72 )
			{
				fprintf( out, "\"\n    \"" );
				col = 0;
			}
			else if ( len && s.at( 0 ) >= '0' && s.at( 0 ) <= '9' )
			{
				fprintf( out, "\"\"" );
				len += 2;
			}
			uint64 idx = 0;
			while ( idx < s.length() )
			{
				if ( idx > 0 )
				{
					col = 0;
					fprintf( out, "\"\n    \"" );
				}
				uint64 spanLen = std::min( 70ull, s.length() - idx );
				// don't cut escape sequences at the end of a line
				uint64 backSlashPos = s.find_last_of( '\\', idx + spanLen - 1 );
				if ( backSlashPos >= idx )
				{
					uint64 escapeLen = lengthOfEscapeSequence( s, backSlashPos );
					spanLen = std::max( spanLen, std::min( backSlashPos + escapeLen - idx, s.length() - idx ) );
				}
				fprintf( out, "%.*s", spanLen, s.data() + idx );
				idx += spanLen;
				col += spanLen;
			}
			col += len + 2;
		}

		// Terminate stringdata struct
		fprintf( out, "\"\n};\n" );
		fprintf( out, "#undef QT_MOC_LITERAL\n\n" );

		//
		// build the data array
		//

		// todo
		uint64 index = 0;// MetaObjectPrivateFieldCount;
		fprintf( out, "static const uint32 qt_meta_data_%s[] = {\n", qualifiedClassNameIdentifier.data() );
		fprintf( out, "\n // content:\n" );
		// todo
		fprintf( out, "    %4d,       // revision\n", 0 ); // int( QMetaObjectPrivate::OutputRevision ) );
		fprintf( out, "    %4lld,       // classname\n", stridx( cdef->qualified ) );
		fprintf( out, "    %4lld, %4lld, // classinfo\n", cdef->classInfoList.size(), cdef->classInfoList.size() ? index : 0 );
		index += cdef->classInfoList.size() * 2;

		uint64 methodCount = cdef->signalList.size() + cdef->slotList.size() + cdef->methodList.size();
		fprintf( out, "    %4lld, %4lld, // methods\n", methodCount, methodCount ? index : 0 );
		index += methodCount * 5;
		if ( cdef->revisionedMethods )
			index += methodCount;
		uint64 paramsIndex = index;
		uint64 totalParameterCount = aggregateParameterCount( cdef->signalList )
			+ aggregateParameterCount( cdef->slotList )
			+ aggregateParameterCount( cdef->methodList )
			+ aggregateParameterCount( cdef->constructorList );
		index += totalParameterCount * 2 // types and parameter names
			- methodCount // return "parameters" don't have names
			- cdef->constructorList.size(); // "this" parameters don't have names

		fprintf( out, "    %4lld, %4lld, // properties\n", cdef->propertyList.size(), cdef->propertyList.size() ? index : 0 );
		index += cdef->propertyList.size() * 3;
		if ( cdef->notifyableProperties )
			index += cdef->propertyList.size();
		if ( cdef->revisionedProperties )
			index += cdef->propertyList.size();
		fprintf( out, "    %4lld, %4lld, // enums/sets\n", cdef->enumList.size(), cdef->enumList.size() ? index : 0 );

		uint64 enumsIndex = index;
		for ( int i = 0; i < cdef->enumList.size(); ++i )
			index += 4 + (cdef->enumList.at( i ).values.size() * 2);
		fprintf( out, "    %4lld, %4lld, // constructors\n", isConstructible ? cdef->constructorList.size() : 0,
			isConstructible ? index : 0 );

		int flags = 0;
		if ( cdef->hasQGadget )
		{
			// Ideally, all the classes could have that flag. But this broke classes generated
			// by qdbusxml2cpp which generate code that require that we call qt_metacall for properties

			// todo
			// flags |= PropertyAccessInStaticMetaCall;
		}
		fprintf( out, "    %4d,       // flags\n", flags );
		fprintf( out, "    %4lld,       // signalCount\n", cdef->signalList.size() );


		//
		// Build classinfo array
		//
		generateClassInfos();

		// TODO
		//
		// Build signals array first, otherwise the signal indices would be wrong
		//
		//generateFunctions( cdef->signalList, "signal", MethodSignal, paramsIndex );

		//
		// Build slots array
		//
		//generateFunctions( cdef->slotList, "slot", MethodSlot, paramsIndex );

		//
		// Build method array
		//
		//generateFunctions( cdef->methodList, "method", MethodMethod, paramsIndex );

		//
		// Build method version arrays
		//
		if ( cdef->revisionedMethods )
		{
			generateFunctionRevisions( cdef->signalList, "signal" );
			generateFunctionRevisions( cdef->slotList, "slot" );
			generateFunctionRevisions( cdef->methodList, "method" );
		}

		//
		// Build method parameters array
		//
		generateFunctionParameters( cdef->signalList, "signal" );
		generateFunctionParameters( cdef->slotList, "slot" );
		generateFunctionParameters( cdef->methodList, "method" );
		if ( isConstructible )
			generateFunctionParameters( cdef->constructorList, "constructor" );

		//
		// Build property array
		//
		generateProperties();

		//
		// Build enums array
		//
		generateEnums( enumsIndex );

		//
		// Build constructors array
		//

		// TODO
		// if ( isConstructible )
		// 	generateFunctions( cdef->constructorList, "constructor", MethodConstructor, paramsIndex );

		//
		// Terminate data array
		//
		fprintf( out, "\n       0        // eod\n};\n\n" );

		//
		// Generate internal qt_static_metacall() function
		//
		const bool hasStaticMetaCall = !isQt &&
			(cdef->hasQObject || !cdef->methodList.empty()
				|| !cdef->propertyList.empty() || !cdef->constructorList.empty());
		if ( hasStaticMetaCall )
			generateStaticMetacall();

		//
		// Build extra array
		//
		std::vector<std::string> extraList;
		std::unordered_map<std::string, std::string> knownExtraMetaObject = knownGadgets;
		knownExtraMetaObject.insert( knownQObjectClasses.begin(), knownQObjectClasses.end() );

		for ( int i = 0; i < cdef->propertyList.size(); ++i )
		{
			const PropertyDef &p = cdef->propertyList.at( i );
			if ( isBuiltinType( p.type ) )
				continue;

			if ( p.type.find( '*' ) != std::string::npos || p.type.find( '<' ) != std::string::npos || p.type.find( '>' ) != std::string::npos )
				continue;

			uint64 s = p.type.find_last_of( "::" );
			if ( s <= 0 )
				continue;

			std::string unqualifiedScope = subset(p.type, 0, s );

			// The scope may be a namespace for example, so it's only safe to include scopes that are known QObjects (QTBUG-2151)
			std::unordered_map<std::string, std::string>::const_iterator scopeIt;

			std::string thisScope = cdef->qualified;
			do
			{
				uint64 s = thisScope.find_last_of( "::" );
				thisScope = subset(thisScope, 0, s );
				std::string currentScope = thisScope.empty() ? unqualifiedScope : thisScope + "::" + unqualifiedScope;
				scopeIt = knownExtraMetaObject.find( currentScope );
			}
			while ( !thisScope.empty() && scopeIt == knownExtraMetaObject.end() );

			if ( scopeIt == knownExtraMetaObject.end() )
				continue;

			// TODO
			const std::string &scope = (*scopeIt).first;

			if ( scope == "Qt" )
				continue;
			if ( qualifiedNameEquals( cdef->qualified, scope ) )
				continue;

			if ( std::find(extraList.begin(), extraList.end(), scope ) == extraList.end())
				extraList.push_back(scope);
		}

		// QTBUG-20639 - Accept non-local enums for QML signal/slot parameters.
		// Look for any scoped enum declarations, and add those to the list
		// of extra/related metaobjects for this object.
		for ( auto it = cdef->enumDeclarations.begin(),
			end = cdef->enumDeclarations.end(); it != end; ++it )
		{
			const std::string &enumKey = (*it).first;
			uint64 s = enumKey.find_last_of( "::" );
			if ( s > 0 )
			{
				std::string scope = subset(enumKey, 0, s );
				if ( scope != "Qt" && !qualifiedNameEquals( cdef->qualified, scope ) && std::find(extraList.begin(), extraList.end(), scope ) == extraList.end() )
					extraList.push_back(scope);
			}
		}

		if ( !extraList.empty() )
		{
			fprintf( out, "static const QMetaObject * const qt_meta_extradata_%s[] = {\n    ", qualifiedClassNameIdentifier.data() );
			for ( int i = 0; i < extraList.size(); ++i )
			{
				fprintf( out, "    &%s::staticMetaObject,\n", extraList.at( i ).data() );
			}
			fprintf( out, "    nullptr\n};\n\n" );
		}

		//
		// Finally create and initialize the static meta object
		//
		if ( isQt )
			fprintf( out, "const QMetaObject QObject::staticQtMetaObject = {\n" );
		else
			fprintf( out, "const QMetaObject %s::staticMetaObject = {\n", cdef->qualified.data() );

		if ( isQObject )
			fprintf( out, "    { nullptr, " );
		else if ( cdef->superclassList.size() && (!cdef->hasQGadget || knownGadgets.find( purestSuperClass ) != knownGadgets.end()) )
			fprintf( out, "    { &%s::staticMetaObject, ", purestSuperClass.data() );
		else
			fprintf( out, "    { nullptr, " );
		fprintf( out, "qt_meta_stringdata_%s.data,\n"
			"      qt_meta_data_%s, ", qualifiedClassNameIdentifier.data(),
			qualifiedClassNameIdentifier.data() );
		if ( hasStaticMetaCall )
			fprintf( out, " qt_static_metacall, " );
		else
			fprintf( out, " nullptr, " );

		if ( extraList.empty() )
			fprintf( out, "nullptr, " );
		else
			fprintf( out, "qt_meta_extradata_%s, ", qualifiedClassNameIdentifier.data() );
		fprintf( out, "nullptr}\n};\n\n" );

		if ( isQt )
			return;

		if ( !cdef->hasQObject )
			return;

		fprintf( out, "\nconst QMetaObject *%s::metaObject() const\n{\n    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;\n}\n",
			cdef->qualified.data() );

		//
		// Generate smart cast function
		//
		fprintf( out, "\nvoid *%s::qt_metacast(const char *_clname)\n{\n", cdef->qualified.data() );
		fprintf( out, "    if (!_clname) return nullptr;\n" );
		fprintf( out, "    if (!strcmp(_clname, qt_meta_stringdata_%s.stringdata0))\n"
			"        return static_cast<void*>(const_cast< %s*>(this));\n",
			qualifiedClassNameIdentifier.data(), cdef->classname.data() );
		for ( int i = 1; i < cdef->superclassList.size(); ++i )
		{ // for all superclasses but the first one
			if ( std::get<1>(cdef->superclassList.at( i )) == FunctionDef::Private )
				continue;
			const char *cname = std::get<0>(cdef->superclassList.at( i )).data();
			fprintf( out, "    if (!strcmp(_clname, \"%s\"))\n        return static_cast< %s*>(const_cast< %s*>(this));\n",
				cname, cname, cdef->classname.data() );
		}
		for ( int i = 0; i < cdef->interfaceList.size(); ++i )
		{
			const std::vector<ClassDef::Interface> &iface = cdef->interfaceList.at( i );
			for ( int j = 0; j < iface.size(); ++j )
			{
				fprintf( out, "    if (!strcmp(_clname, %s))\n        return ", iface.at( j ).interfaceId.data() );
				for ( int k = j; k >= 0; --k )
					fprintf( out, "static_cast< %s*>(", iface.at( k ).className.data() );
				fprintf( out, "const_cast< %s*>(this)%s;\n",
					cdef->classname.data(), std::string( j + 1, ')' ).data() );
			}
		}
		if ( !purestSuperClass.empty() && !isQObject )
		{
			std::string superClass = purestSuperClass;
			fprintf( out, "    return %s::qt_metacast(_clname);\n", superClass.data() );
		}
		else
		{
			fprintf( out, "    return nullptr;\n" );
		}
		fprintf( out, "}\n" );

		//
		// Generate internal qt_metacall()  function
		//
		generateMetacall();

		//
		// Generate internal signal functions
		//
		for ( int signalindex = 0; signalindex < cdef->signalList.size(); ++signalindex )
			generateSignal( &cdef->signalList[signalindex], signalindex );

		//
		// Generate plugin meta data
		//
		generatePluginMetaData();
	}


	void Generator::registerClassInfoStrings()
	{
		for ( int i = 0; i < cdef->classInfoList.size(); ++i )
		{
			const ClassInfoDef &c = cdef->classInfoList.at( i );
			strreg( c.name );
			strreg( c.value );
		}
	}

	void Generator::generateClassInfos()
	{
		if ( cdef->classInfoList.empty() )
			return;

		fprintf( out, "\n // classinfo: key, value\n" );

		for ( uint64 i = 0; i < cdef->classInfoList.size(); ++i )
		{
			const ClassInfoDef &c = cdef->classInfoList.at( i );
			fprintf( out, "    %4lld, %4lld,\n", stridx( c.name ), stridx( c.value ) );
		}
	}

	void Generator::registerFunctionStrings( const std::vector<FunctionDef>& list )
	{
		for ( int i = 0; i < list.size(); ++i )
		{
			const FunctionDef &f = list.at( i );

			strreg( f.name );
			if ( !isBuiltinType( f.normalizedType ) )
				strreg( f.normalizedType );
			strreg( f.tag );

			uint64 argsCount = f.arguments.size();
			for ( int j = 0; j < argsCount; ++j )
			{
				const ArgumentDef &a = f.arguments.at( j );
				if ( !isBuiltinType( a.normalizedType ) )
					strreg( a.normalizedType );
				strreg( a.name );
			}
		}
	}

	void Generator::generateFunctions( const std::vector<FunctionDef> &list, const char *functype, int type, uint64& paramsIndex )
	{
		if ( list.empty() )
			return;
		fprintf( out, "\n // %ss: name, argc, parameters, tag, flags\n", functype );

		for ( int i = 0; i < list.size(); ++i )
		{
			const FunctionDef &f = list.at( i );

			std::string comment;
			unsigned char flags = type;
			if ( f.access == FunctionDef::Private )
			{
				// todo
				// flags |= AccessPrivate;
				comment.append( "Private" );
			}
			else if ( f.access == FunctionDef::Public )
			{
				// todo
				// flags |= AccessPublic;
				comment.append( "Public" );
			}
			else if ( f.access == FunctionDef::Protected )
			{
				// todo
				// flags |= AccessProtected;
				comment.append( "Protected" );
			}
			if ( f.isCompat )
			{
				// todo
				// flags |= MethodCompatibility;
				comment.append( " | MethodCompatibility" );
			}
			if ( f.wasCloned )
			{
				// todo
				// flags |= MethodCloned;
				comment.append( " | MethodCloned" );
			}
			if ( f.isScriptable )
			{
				// todo
				// flags |= MethodScriptable;
				comment.append( " | isScriptable" );
			}
			if ( f.revision > 0 )
			{
				// todo
				// flags |= MethodRevisioned;
				comment.append( " | MethodRevisioned" );
			}

			uint64 argc = f.arguments.size();
			fprintf( out, "    %4lld, %4lld, %4lld, %4lld, 0x%02x /* %s */,\n",
				stridx( f.name ), argc, paramsIndex, stridx( f.tag ), flags, comment.data() );

			paramsIndex += 1 + argc * 2;
		}
	}

	void Generator::generateFunctionRevisions( const std::vector<FunctionDef>& list, const char *functype )
	{
		if ( list.size() )
			fprintf( out, "\n // %ss: revision\n", functype );
		for ( int i = 0; i < list.size(); ++i )
		{
			const FunctionDef &f = list.at( i );
			fprintf( out, "    %4d,\n", f.revision );
		}
	}

	void Generator::generateFunctionParameters( const std::vector<FunctionDef>& list, const char *functype )
	{
		if ( list.empty() )
			return;
		fprintf( out, "\n // %ss: parameters\n", functype );
		for ( int i = 0; i < list.size(); ++i )
		{
			const FunctionDef &f = list.at( i );
			fprintf( out, "    " );

			// Types
			uint64 argsCount = f.arguments.size();
			for ( int j = -1; j < argsCount; ++j )
			{
				if ( j > -1 )
					fputc( ' ', out );
				const std::string &typeName = (j < 0) ? f.normalizedType : f.arguments.at( j ).normalizedType;
				generateTypeInfo( typeName, /*allowEmptyName=*/f.isConstructor );
				fputc( ',', out );
			}

			// Parameter names
			for ( int j = 0; j < argsCount; ++j )
			{
				const ArgumentDef &arg = f.arguments.at( j );
				fprintf( out, " %4lld,", stridx( arg.name ) );
			}

			fprintf( out, "\n" );
		}
	}

	void Generator::generateTypeInfo( const std::string &typeName, bool allowEmptyName )
	{
		//Q_UNUSED( allowEmptyName );
		if ( isBuiltinType( typeName ) )
		{
			int type;
			const char *valueString;
			if ( typeName == "qreal" )
			{
				// todo
				//type = metatype::UnknownType;
				valueString = "QReal";
			}
			else
			{
				type = nameToBuiltinType( typeName );
				valueString = metaTypeEnumValueString( type );
			}
			if ( valueString )
			{
				fprintf( out, "metatype::%s", valueString );
			}
			else
			{
				// todo
				//Q_ASSERT( type != metatype::UnknownType );
				fprintf( out, "%4d", type );
			}
		}
		else
		{
			// todo
			//Q_ASSERT( !typeName.empty() || allowEmptyName );
			//fprintf( out, "0x%.8x | %d", IsUnresolvedType, stridx( typeName ) );
		}
	}

	void Generator::registerPropertyStrings()
	{
		for ( int i = 0; i < cdef->propertyList.size(); ++i )
		{
			const PropertyDef &p = cdef->propertyList.at( i );
			strreg( p.name );
			if ( !isBuiltinType( p.type ) )
				strreg( p.type );
		}
	}

	void Generator::generateProperties()
	{
		//
		// Create meta data
		//

		if ( cdef->propertyList.size() )
			fprintf( out, "\n // properties: name, type, flags\n" );
		for ( int i = 0; i < cdef->propertyList.size(); ++i )
		{
			const PropertyDef &p = cdef->propertyList.at( i );

			// TODO
			uint32 flags = 0; // Invalid;
			/*
			if ( !isBuiltinType( p.type ) )
				flags |= EnumOrFlag;
			if ( !p.member.empty() && !p.constant )
				flags |= Writable;
			if ( !p.read.empty() || !p.member.empty() )
				flags |= Readable;
			if ( !p.write.empty() )
			{
				flags |= Writable;
				if ( p.stdCppSet() )
					flags |= StdCppSet;
			}
			if ( !p.reset.empty() )
				flags |= Resettable;

			//         if (p.override)
			//             flags |= Override;

			if ( p.designable.empty() )
				flags |= ResolveDesignable;
			else if ( p.designable != "false" )
				flags |= Designable;

			if ( p.scriptable.empty() )
				flags |= ResolveScriptable;
			else if ( p.scriptable != "false" )
				flags |= Scriptable;

			if ( p.stored.empty() )
				flags |= ResolveStored;
			else if ( p.stored != "false" )
				flags |= Stored;

			if ( p.editable.empty() )
				flags |= ResolveEditable;
			else if ( p.editable != "false" )
				flags |= Editable;

			if ( p.user.empty() )
				flags |= ResolveUser;
			else if ( p.user != "false" )
				flags |= User;

			if ( p.notifyId != -1 )
				flags |= Notify;

			if ( p.revision > 0 )
				flags |= Revisioned;

			if ( p.constant )
				flags |= Constant;
			if ( p.final )
				flags |= Final;
			*/
			fprintf( out, "    %4lld, ", stridx( p.name ) );
			generateTypeInfo( p.type );
			fprintf( out, ", 0x%.8x,\n", flags );
		}

		if ( cdef->notifyableProperties )
		{
			fprintf( out, "\n // properties: notify_signal_id\n" );
			for ( int i = 0; i < cdef->propertyList.size(); ++i )
			{
				const PropertyDef &p = cdef->propertyList.at( i );
				if ( p.notifyId == -1 )
					fprintf( out, "    %4d,\n",
						0 );
				else
					fprintf( out, "    %4d,\n",
						p.notifyId );
			}
		}
		if ( cdef->revisionedProperties )
		{
			fprintf( out, "\n // properties: revision\n" );
			for ( int i = 0; i < cdef->propertyList.size(); ++i )
			{
				const PropertyDef &p = cdef->propertyList.at( i );
				fprintf( out, "    %4d,\n", p.revision );
			}
		}
	}

	void Generator::registerEnumStrings()
	{
		for ( int i = 0; i < cdef->enumList.size(); ++i )
		{
			const EnumDef &e = cdef->enumList.at( i );
			strreg( e.name );
			for ( int j = 0; j < e.values.size(); ++j )
				strreg( e.values.at( j ) );
		}
	}

	void Generator::generateEnums( uint64 index )
	{
		if ( cdef->enumDeclarations.empty() )
			return;

		fprintf( out, "\n // enums: name, flags, count, data\n" );
		index += 4 * cdef->enumList.size();
		int i;
		for ( i = 0; i < cdef->enumList.size(); ++i )
		{
			const EnumDef &e = cdef->enumList.at( i );
			int flags = 0;
			// todo
			/*
			if ( cdef->enumDeclarations[e.name] )
				flags |= EnumIsFlag;
			if ( e.isEnumClass )
				flags |= EnumIsScoped;
				*/
			fprintf( out, "    %4lld, 0x%.1x, %4lld, %4lld,\n",
				stridx( e.name ),
				flags,
				e.values.size(),
				index );
			index += e.values.size() * 2;
		}

		fprintf( out, "\n // enum data: key, value\n" );
		for ( i = 0; i < cdef->enumList.size(); ++i )
		{
			const EnumDef &e = cdef->enumList.at( i );
			for ( int j = 0; j < e.values.size(); ++j )
			{
				const std::string &val = e.values.at( j );
				std::string code = cdef->qualified.data();
				if ( e.isEnumClass )
					code += "::" + e.name;
				code += "::" + val;
				fprintf( out, "    %4lld, uint(%s),\n",
					stridx( val ), code.data() );
			}
		}
	}

	void Generator::generateMetacall()
	{
		bool isQObject = (cdef->classname == "QObject");

		fprintf( out, "\nint %s::qt_metacall(QMetaObject::Call _c, int _id, void **_a)\n{\n",
			cdef->qualified.data() );

		if ( !purestSuperClass.empty() && !isQObject )
		{
			std::string superClass = purestSuperClass;
			fprintf( out, "    _id = %s::qt_metacall(_c, _id, _a);\n", superClass.data() );
		}


		bool needElse = false;
		std::vector<FunctionDef> methodList;
		methodList.insert( methodList.begin(), cdef->signalList.begin(), cdef->signalList.end() );
		methodList.insert( methodList.begin(), cdef->slotList.begin(), cdef->slotList.end() );
		methodList.insert( methodList.begin(), cdef->methodList.begin(), cdef->methodList.end() );

		// If there are no methods or properties, we will return _id anyway, so
		// don't emit this comparison -- it is unnecessary, and it makes coverity
		// unhappy.
		if ( methodList.size() || cdef->propertyList.size() )
		{
			fprintf( out, "    if (_id < 0)\n        return _id;\n" );
		}

		fprintf( out, "    " );

		if ( methodList.size() )
		{
			needElse = true;
			fprintf( out, "if (_c == QMetaObject::InvokeMetaMethod) {\n" );
			fprintf( out, "        if (_id < %lld)\n", methodList.size() );
			fprintf( out, "            qt_static_metacall(this, _c, _id, _a);\n" );
			fprintf( out, "        _id -= %lld;\n    }", methodList.size() );

			fprintf( out, " else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n" );
			fprintf( out, "        if (_id < %lld)\n", methodList.size() );

			if ( methodsWithAutomaticTypesHelper( methodList ).empty() )
				fprintf( out, "            *reinterpret_cast<int*>(_a[0]) = -1;\n" );
			else
				fprintf( out, "            qt_static_metacall(this, _c, _id, _a);\n" );
			fprintf( out, "        _id -= %lld;\n    }", methodList.size() );

		}

		if ( cdef->propertyList.size() )
		{
			bool needDesignable = false;
			bool needScriptable = false;
			bool needStored = false;
			bool needEditable = false;
			bool needUser = false;
			for ( int i = 0; i < cdef->propertyList.size(); ++i )
			{
				const PropertyDef &p = cdef->propertyList.at( i );
				needDesignable |= p.designable.back() == ')';
				needScriptable |= p.scriptable.back() == ')';
				needStored |= p.stored.back() == ')';
				needEditable |= p.editable.back() == ')';
				needUser |= p.user.back() == ')';
			}

			fprintf( out, "\n#ifndef QT_NO_PROPERTIES\n   " );
			if ( needElse )
				fprintf( out, "else " );
			fprintf( out,
				"if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty\n"
				"            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {\n"
				"        qt_static_metacall(this, _c, _id, _a);\n"
				"        _id -= %lld;\n    }", cdef->propertyList.size() );

			fprintf( out, " else " );
			fprintf( out, "if (_c == QMetaObject::QueryPropertyDesignable) {\n" );
			if ( needDesignable )
			{
				fprintf( out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( !(p.designable.back() == ')') )
						continue;
					fprintf( out, "        case %d: *_b = %s; break;\n",
						propindex, p.designable.data() );
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}
			fprintf( out,
				"        _id -= %lld;\n"
				"    }", cdef->propertyList.size() );

			fprintf( out, " else " );
			fprintf( out, "if (_c == QMetaObject::QueryPropertyScriptable) {\n" );
			if ( needScriptable )
			{
				fprintf( out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( !(p.scriptable.back() == ')') )
						continue;
					fprintf( out, "        case %d: *_b = %s; break;\n",
						propindex, p.scriptable.data() );
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}
			fprintf( out,
				"        _id -= %lld;\n"
				"    }", cdef->propertyList.size() );

			fprintf( out, " else " );
			fprintf( out, "if (_c == QMetaObject::QueryPropertyStored) {\n" );
			if ( needStored )
			{
				fprintf( out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( !(p.stored.back() == ')') )
						continue;
					fprintf( out, "        case %d: *_b = %s; break;\n",
						propindex, p.stored.data() );
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}
			fprintf( out,
				"        _id -= %lld;\n"
				"    }", cdef->propertyList.size() );

			fprintf( out, " else " );
			fprintf( out, "if (_c == QMetaObject::QueryPropertyEditable) {\n" );
			if ( needEditable )
			{
				fprintf( out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( !(p.editable.back() == ')') )
						continue;
					fprintf( out, "        case %d: *_b = %s; break;\n",
						propindex, p.editable.data() );
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}
			fprintf( out,
				"        _id -= %lld;\n"
				"    }", cdef->propertyList.size() );


			fprintf( out, " else " );
			fprintf( out, "if (_c == QMetaObject::QueryPropertyUser) {\n" );
			if ( needUser )
			{
				fprintf( out, "        bool *_b = reinterpret_cast<bool*>(_a[0]);\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( !(p.user.back() == ')') )
						continue;
					fprintf( out, "        case %d: *_b = %s; break;\n",
						propindex, p.user.data() );
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}
			fprintf( out,
				"        _id -= %lld;\n"
				"    }", cdef->propertyList.size() );

			fprintf( out, "\n#endif // QT_NO_PROPERTIES" );
		}
		if ( methodList.size() || cdef->propertyList.size() )
			fprintf( out, "\n    " );
		fprintf( out, "return _id;\n}\n" );
	}


	std::multimap<std::string, int> Generator::automaticPropertyMetaTypesHelper()
	{
		std::multimap<std::string, int> automaticPropertyMetaTypes;
		for ( int i = 0; i < cdef->propertyList.size(); ++i )
		{
			const std::string propertyType = cdef->propertyList.at( i ).type;
			if ( registerableMetaType( propertyType ) && !isBuiltinType( propertyType ) )
				automaticPropertyMetaTypes.emplace( propertyType, i );
		}
		return automaticPropertyMetaTypes;
	}

	std::map<int, std::multimap<std::string, int> > Generator::methodsWithAutomaticTypesHelper( const std::vector<FunctionDef> &methodList )
	{
		std::map<int, std::multimap<std::string, int> > methodsWithAutomaticTypes;
		for ( int i = 0; i < methodList.size(); ++i )
		{
			const FunctionDef &f = methodList.at( i );
			for ( int j = 0; j < f.arguments.size(); ++j )
			{
				const std::string argType = f.arguments.at( j ).normalizedType;
				if ( registerableMetaType( argType ) && !isBuiltinType( argType ) )
					methodsWithAutomaticTypes[i].emplace( argType, j );
			}
		}
		return methodsWithAutomaticTypes;
	}

	void Generator::generateStaticMetacall()
	{
		fprintf( out, "void %s::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)\n{\n",
			cdef->qualified.data() );

		bool needElse = false;
		bool isUsed_a = false;

		if ( !cdef->constructorList.empty() )
		{
			fprintf( out, "    if (_c == QMetaObject::CreateInstance) {\n" );
			fprintf( out, "        switch (_id) {\n" );
			for ( int ctorindex = 0; ctorindex < cdef->constructorList.size(); ++ctorindex )
			{
				fprintf( out, "        case %d: { %s *_r = new %s(", ctorindex,
					cdef->classname.data(), cdef->classname.data() );
				const FunctionDef &f = cdef->constructorList.at( ctorindex );
				int offset = 1;

				uint64 argsCount = f.arguments.size();
				for ( uint64 j = 0; j < argsCount; ++j )
				{
					const ArgumentDef &a = f.arguments.at( j );
					if ( j )
						fprintf( out, "," );
					fprintf( out, "(*reinterpret_cast< %s>(_a[%d]))", a.typeNameForCast.data(), offset++ );
				}
				if ( f.isPrivateSignal )
				{
					if ( argsCount > 0 )
						fprintf( out, ", " );
					fprintf( out, "%s", std::string( "QPrivateSignal()" ).data() );
				}
				fprintf( out, ");\n" );
				fprintf( out, "            if (_a[0]) *reinterpret_cast<%s**>(_a[0]) = _r; } break;\n",
					cdef->hasQGadget ? "void" : "QObject" );
			}
			fprintf( out, "        default: break;\n" );
			fprintf( out, "        }\n" );
			fprintf( out, "    }" );
			needElse = true;
			isUsed_a = true;
		}

		std::vector<FunctionDef> methodList;
		methodList.insert( methodList.begin(), cdef->signalList.begin(), cdef->signalList.end() );
		methodList.insert( methodList.begin(), cdef->slotList.begin(), cdef->slotList.end() );
		methodList.insert( methodList.begin(), cdef->methodList.begin(), cdef->methodList.end() );

		if ( !methodList.empty() )
		{
			if ( needElse )
				fprintf( out, " else " );
			else
				fprintf( out, "    " );
			fprintf( out, "if (_c == QMetaObject::InvokeMetaMethod) {\n" );
			if ( cdef->hasQObject )
			{
#ifndef QT_NO_DEBUG
				fprintf( out, "        Q_ASSERT(staticMetaObject.cast(_o));\n" );
#endif
				fprintf( out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
			}
			else
			{
				fprintf( out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
			}
			fprintf( out, "        Q_UNUSED(_t)\n" );
			fprintf( out, "        switch (_id) {\n" );
			for ( int methodindex = 0; methodindex < methodList.size(); ++methodindex )
			{
				const FunctionDef &f = methodList.at( methodindex );
				// TODO
				//Q_ASSERT( !f.normalizedType.empty() );
				fprintf( out, "        case %d: ", methodindex );
				if ( f.normalizedType != "void" )
					fprintf( out, "{ %s _r = ", noRef( f.normalizedType ).data() );
				fprintf( out, "_t->" );
				if ( f.inPrivateClass.size() )
					fprintf( out, "%s->", f.inPrivateClass.data() );
				fprintf( out, "%s(", f.name.data() );
				int offset = 1;

				uint64 argsCount = f.arguments.size();
				for ( uint64 j = 0; j < argsCount; ++j )
				{
					const ArgumentDef &a = f.arguments.at( j );
					if ( j )
						fprintf( out, "," );
					fprintf( out, "(*reinterpret_cast< %s>(_a[%d]))", a.typeNameForCast.data(), offset++ );
					isUsed_a = true;
				}
				if ( f.isPrivateSignal )
				{
					if ( argsCount > 0 )
						fprintf( out, ", " );
					fprintf( out, "%s", "QPrivateSignal()" );
				}
				fprintf( out, ");" );
				if ( f.normalizedType != "void" )
				{
					fprintf( out, "\n            if (_a[0]) *reinterpret_cast< %s*>(_a[0]) = std::move(_r); } ",
						noRef( f.normalizedType ).data() );
					isUsed_a = true;
				}
				fprintf( out, " break;\n" );
			}
			fprintf( out, "        default: ;\n" );
			fprintf( out, "        }\n" );
			fprintf( out, "    }" );
			needElse = true;

			std::map<int, std::multimap<std::string, int> > methodsWithAutomaticTypes = methodsWithAutomaticTypesHelper( methodList );

			if ( !methodsWithAutomaticTypes.empty() )
			{
				fprintf( out, " else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {\n" );
				fprintf( out, "        switch (_id) {\n" );
				fprintf( out, "        default: *reinterpret_cast<int*>(_a[0]) = -1; break;\n" );
				std::map<int, std::multimap<std::string, int> >::const_iterator it = methodsWithAutomaticTypes.begin();
				const std::map<int, std::multimap<std::string, int> >::const_iterator end = methodsWithAutomaticTypes.end();
				for ( ; it != end; ++it )
				{
					fprintf( out, "        case %d:\n", it->first );
					fprintf( out, "            switch (*reinterpret_cast<int*>(_a[1])) {\n" );
					fprintf( out, "            default: *reinterpret_cast<int*>(_a[0]) = -1; break;\n" );
					auto jt = (it->second).begin();
					const auto jend = (it->second).end();
					while ( jt != jend )
					{
						fprintf( out, "            case %d:\n", jt->second );
						const std::string &lastKey = jt->first;
						++jt;
						if ( jt == jend || jt->first != lastKey )
							fprintf( out, "                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< %s >(); break;\n", lastKey.data() );
					}
					fprintf( out, "            }\n" );
					fprintf( out, "            break;\n" );
				}
				fprintf( out, "        }\n" );
				fprintf( out, "    }" );
				isUsed_a = true;
			}

		}
		if ( !cdef->signalList.empty() )
		{
			// TODO
			//Q_ASSERT( needElse ); // if there is signal, there was method.
			fprintf( out, " else if (_c == QMetaObject::IndexOfMethod) {\n" );
			fprintf( out, "        int *result = reinterpret_cast<int *>(_a[0]);\n" );
			fprintf( out, "        void **func = reinterpret_cast<void **>(_a[1]);\n" );
			bool anythingUsed = false;
			for ( int methodindex = 0; methodindex < cdef->signalList.size(); ++methodindex )
			{
				const FunctionDef &f = cdef->signalList.at( methodindex );
				if ( f.wasCloned || !f.inPrivateClass.empty() || f.isStatic )
					continue;
				anythingUsed = true;
				fprintf( out, "        {\n" );
				fprintf( out, "            typedef %s (%s::*_t)(", f.type.rawName.data(), cdef->classname.data() );

				uint64 argsCount = f.arguments.size();
				for ( uint64 j = 0; j < argsCount; ++j )
				{
					const ArgumentDef &a = f.arguments.at( j );
					if ( j )
						fprintf( out, ", " );
					fprintf( out, "%s", std::string( a.type.name + ' ' + a.rightType ).data() );
				}
				if ( f.isPrivateSignal )
				{
					if ( argsCount > 0 )
						fprintf( out, ", " );
					fprintf( out, "%s", "QPrivateSignal" );
				}
				if ( f.isConst )
					fprintf( out, ") const;\n" );
				else
					fprintf( out, ");\n" );
				fprintf( out, "            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&%s::%s)) {\n",
					cdef->classname.data(), f.name.data() );
				fprintf( out, "                *result = %d;\n", methodindex );
				fprintf( out, "                return;\n" );
				fprintf( out, "            }\n        }\n" );
			}
			if ( !anythingUsed )
				fprintf( out, "        Q_UNUSED(result);\n        Q_UNUSED(func);\n" );
			fprintf( out, "    }" );
			needElse = true;
		}

		const std::multimap<std::string, int> automaticPropertyMetaTypes = automaticPropertyMetaTypesHelper();

		if ( !automaticPropertyMetaTypes.empty() )
		{
			if ( needElse )
				fprintf( out, " else " );
			else
				fprintf( out, "    " );
			fprintf( out, "if (_c == QMetaObject::RegisterPropertyMetaType) {\n" );
			fprintf( out, "        switch (_id) {\n" );
			fprintf( out, "        default: *reinterpret_cast<int*>(_a[0]) = -1; break;\n" );
			auto it = automaticPropertyMetaTypes.begin();
			const auto end = automaticPropertyMetaTypes.end();
			while ( it != end )
			{
				fprintf( out, "        case %d:\n", it->second );
				const std::string &lastKey = it->first;
				++it;
				if ( it == end || it->first != lastKey )
					fprintf( out, "            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< %s >(); break;\n", lastKey.data() );
			}
			fprintf( out, "        }\n" );
			fprintf( out, "    }\n" );
			isUsed_a = true;
			needElse = true;
		}

		if ( !cdef->propertyList.empty() )
		{
			bool needGet = false;
			bool needTempVarForGet = false;
			bool needSet = false;
			bool needReset = false;
			for ( int i = 0; i < cdef->propertyList.size(); ++i )
			{
				const PropertyDef &p = cdef->propertyList.at( i );
				needGet |= !p.read.empty() || !p.member.empty();
				if ( !p.read.empty() || !p.member.empty() )
					needTempVarForGet |= (p.gspec != PropertyDef::PointerSpec
						&& p.gspec != PropertyDef::ReferenceSpec);

				needSet |= !p.write.empty() || (!p.member.empty() && !p.constant);
				needReset |= !p.reset.empty();
			}
			fprintf( out, "\n#ifndef QT_NO_PROPERTIES\n    " );

			if ( needElse )
				fprintf( out, "else " );
			fprintf( out, "if (_c == QMetaObject::ReadProperty) {\n" );
			if ( needGet )
			{
				if ( cdef->hasQObject )
				{
#ifndef QT_NO_DEBUG
					fprintf( out, "        Q_ASSERT(staticMetaObject.cast(_o));\n" );
#endif
					fprintf( out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
				}
				else
				{
					fprintf( out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
				}
				fprintf( out, "        Q_UNUSED(_t)\n" );
				if ( needTempVarForGet )
					fprintf( out, "        void *_v = _a[0];\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( p.read.empty() && p.member.empty() )
						continue;
					std::string prefix = "_t->";
					if ( p.inPrivateClass.size() )
					{
						prefix += p.inPrivateClass + "->";
					}
					if ( p.gspec == PropertyDef::PointerSpec )
						fprintf( out, "        case %d: _a[0] = const_cast<void*>(reinterpret_cast<const void*>(%s%s())); break;\n",
							propindex, prefix.data(), p.read.data() );
					else if ( p.gspec == PropertyDef::ReferenceSpec )
						fprintf( out, "        case %d: _a[0] = const_cast<void*>(reinterpret_cast<const void*>(&%s%s())); break;\n",
							propindex, prefix.data(), p.read.data() );
					else if ( cdef->enumDeclarations.find(p.type) != cdef->enumDeclarations.end() ? cdef->enumDeclarations[p.type] : false )
						fprintf( out, "        case %d: *reinterpret_cast<int*>(_v) = QFlag(%s%s()); break;\n",
							propindex, prefix.data(), p.read.data() );
					else if ( !p.read.empty() )
						fprintf( out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s(); break;\n",
							propindex, p.type.data(), prefix.data(), p.read.data() );
					else
						fprintf( out, "        case %d: *reinterpret_cast< %s*>(_v) = %s%s; break;\n",
							propindex, p.type.data(), prefix.data(), p.member.data() );
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}

			fprintf( out, "    }" );

			fprintf( out, " else " );
			fprintf( out, "if (_c == QMetaObject::WriteProperty) {\n" );

			if ( needSet )
			{
				if ( cdef->hasQObject )
				{
#ifndef QT_NO_DEBUG
					fprintf( out, "        Q_ASSERT(staticMetaObject.cast(_o));\n" );
#endif
					fprintf( out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
				}
				else
				{
					fprintf( out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
				}
				fprintf( out, "        Q_UNUSED(_t)\n" );
				fprintf( out, "        void *_v = _a[0];\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( p.constant )
						continue;
					if ( p.write.empty() && p.member.empty() )
						continue;
					std::string prefix = "_t->";
					if ( p.inPrivateClass.size() )
					{
						prefix += p.inPrivateClass + "->";
					}
					if ( cdef->enumDeclarations.find( p.type ) != cdef->enumDeclarations.end() ? cdef->enumDeclarations[p.type] : false )
					{
						fprintf( out, "        case %d: %s%s(QFlag(*reinterpret_cast<int*>(_v))); break;\n",
							propindex, prefix.data(), p.write.data() );
					}
					else if ( !p.write.empty() )
					{
						fprintf( out, "        case %d: %s%s(*reinterpret_cast< %s*>(_v)); break;\n",
							propindex, prefix.data(), p.write.data(), p.type.data() );
					}
					else
					{
						fprintf( out, "        case %d:\n", propindex );
						fprintf( out, "            if (%s%s != *reinterpret_cast< %s*>(_v)) {\n",
							prefix.data(), p.member.data(), p.type.data() );
						fprintf( out, "                %s%s = *reinterpret_cast< %s*>(_v);\n",
							prefix.data(), p.member.data(), p.type.data() );
						if ( !p.notify.empty() && p.notifyId != -1 )
						{
							const FunctionDef &f = cdef->signalList.at( p.notifyId );
							if ( f.arguments.size() == 0 )
								fprintf( out, "                Q_EMIT _t->%s();\n", p.notify.data() );
							else if ( f.arguments.size() == 1 && f.arguments.at( 0 ).normalizedType == p.type )
								fprintf( out, "                Q_EMIT _t->%s(%s%s);\n",
									p.notify.data(), prefix.data(), p.member.data() );
						}
						fprintf( out, "            }\n" );
						fprintf( out, "            break;\n" );
					}
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}

			fprintf( out, "    }" );

			fprintf( out, " else " );
			fprintf( out, "if (_c == QMetaObject::ResetProperty) {\n" );
			if ( needReset )
			{
				if ( cdef->hasQObject )
				{
#ifndef QT_NO_DEBUG
					fprintf( out, "        Q_ASSERT(staticMetaObject.cast(_o));\n" );
#endif
					fprintf( out, "        %s *_t = static_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
				}
				else
				{
					fprintf( out, "        %s *_t = reinterpret_cast<%s *>(_o);\n", cdef->classname.data(), cdef->classname.data() );
				}
				fprintf( out, "        Q_UNUSED(_t)\n" );
				fprintf( out, "        switch (_id) {\n" );
				for ( int propindex = 0; propindex < cdef->propertyList.size(); ++propindex )
				{
					const PropertyDef &p = cdef->propertyList.at( propindex );
					if ( !(p.reset.back() == ')') )
						continue;
					std::string prefix = "_t->";
					if ( p.inPrivateClass.size() )
					{
						prefix += p.inPrivateClass + "->";
					}
					fprintf( out, "        case %d: %s%s; break;\n",
						propindex, prefix.data(), p.reset.data() );
				}
				fprintf( out, "        default: break;\n" );
				fprintf( out, "        }\n" );
			}
			fprintf( out, "    }" );
			fprintf( out, "\n#endif // QT_NO_PROPERTIES" );
			needElse = true;
		}

		if ( needElse )
			fprintf( out, "\n" );

		if ( methodList.empty() )
		{
			fprintf( out, "    Q_UNUSED(_o);\n" );
			if ( cdef->constructorList.empty() && automaticPropertyMetaTypes.empty() && methodsWithAutomaticTypesHelper( methodList ).empty() )
			{
				fprintf( out, "    Q_UNUSED(_id);\n" );
				fprintf( out, "    Q_UNUSED(_c);\n" );
			}
		}
		if ( !isUsed_a )
			fprintf( out, "    Q_UNUSED(_a);\n" );

		fprintf( out, "}\n\n" );
	}

	void Generator::generateSignal( FunctionDef *def, int index )
	{
		if ( def->wasCloned || def->isAbstract )
			return;
		fprintf( out, "\n// SIGNAL %d\n%s %s::%s(",
			index, def->type.name.data(), cdef->qualified.data(), def->name.data() );

		std::string thisPtr = "this";
		const char *constQualifier = "";

		if ( def->isConst )
		{
			thisPtr = "const_cast< " + cdef->qualified + " *>(this)";
			constQualifier = "const";
		}

		// todo
		//Q_ASSERT( !def->normalizedType.empty() );
		if ( def->arguments.empty() && def->normalizedType == "void" && !def->isPrivateSignal )
		{
			fprintf( out, ")%s\n{\n"
				"    QMetaObject::activate(%s, &staticMetaObject, %d, nullptr);\n"
				"}\n", constQualifier, thisPtr.data(), index );
			return;
		}

		int offset = 1;
		for ( int j = 0; j < def->arguments.size(); ++j )
		{
			const ArgumentDef &a = def->arguments.at( j );
			if ( j )
				fprintf( out, ", " );
			fprintf( out, "%s _t%d%s", a.type.name.data(), offset++, a.rightType.data() );
		}
		if ( def->isPrivateSignal )
		{
			if ( !def->arguments.empty() )
				fprintf( out, ", " );
			fprintf( out, "QPrivateSignal _t%d", offset++ );
		}

		fprintf( out, ")%s\n{\n", constQualifier );
		if ( def->type.name.size() && def->normalizedType != "void" )
		{
			std::string returnType = noRef( def->normalizedType );
			fprintf( out, "    %s _t0{};\n", returnType.data() );
		}

		fprintf( out, "    void *_a[] = { " );
		if ( def->normalizedType == "void" )
		{
			fprintf( out, "nullptr" );
		}
		else
		{
			if ( def->returnTypeIsVolatile )
				fprintf( out, "const_cast<void*>(reinterpret_cast<const volatile void*>(&_t0))" );
			else
				fprintf( out, "const_cast<void*>(reinterpret_cast<const void*>(&_t0))" );
		}
		int i;
		for ( i = 1; i < offset; ++i )
			if ( i <= def->arguments.size() && def->arguments.at( i - 1 ).type.isVolatile )
				fprintf( out, ", const_cast<void*>(reinterpret_cast<const volatile void*>(&_t%d))", i );
			else
				fprintf( out, ", const_cast<void*>(reinterpret_cast<const void*>(&_t%d))", i );
		fprintf( out, " };\n" );
		fprintf( out, "    QMetaObject::activate(%s, &staticMetaObject, %d, _a);\n", thisPtr.data(), index );
		if ( def->normalizedType != "void" )
			fprintf( out, "    return _t0;\n" );
		fprintf( out, "}\n" );
	}
	/*
	static void writePluginMetaData( FILE *out, const QJsonObject &data )
	{
		const QJsonDocument doc( data );

		fputs( "\nQT_PLUGIN_METADATA_SECTION\n"
			"static const unsigned char qt_pluginMetaData[] = {\n"
			"    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', ' ',\n   ", out );
#if 0
		fprintf( out, "\"%s\";\n", doc.toJson().data() );
#else
		const std::string binary = doc.toBinaryData();
		const int last = binary.size() - 1;
		for ( int i = 0; i < last; ++i )
		{
			uchar c = (uchar)binary.at( i );
			if ( c < 0x20 || c >= 0x7f )
				fprintf( out, " 0x%02x,", c );
			else if ( c == '\'' || c == '\\' )
				fprintf( out, " '\\%c',", c );
			else
				fprintf( out, " '%c', ", c );
			if ( !((i + 1) % 8) )
				fputs( "\n   ", out );
		}
		fprintf( out, " 0x%02x\n};\n", (uchar)binary.at( last ) );
#endif
	}
	*/

	void Generator::generatePluginMetaData()
	{
		/*
		if ( cdef->pluginData.iid.empty() )
			return;

		// Write plugin meta data #ifdefed QT_NO_DEBUG with debug=false,
		// true, respectively.

		QJsonObject data;
		const std::string debugKey = std::stringLiteral( "debug" );
		data.insert( std::stringLiteral( "IID" ), QLatin1String( cdef->pluginData.iid.data() ) );
		data.insert( std::stringLiteral( "className" ), QLatin1String( cdef->classname.data() ) );
		data.insert( std::stringLiteral( "version" ), (int)QT_VERSION );
		data.insert( debugKey, QJsonValue( false ) );
		data.insert( std::stringLiteral( "MetaData" ), cdef->pluginData.metaData.object() );

		// Add -M args from the command line:
		for ( auto it = cdef->pluginData.metaArgs.cbegin(), end = cdef->pluginData.metaArgs.cend(); it != end; ++it )
			data.insert( it.key(), it.value() );

		fputs( "\nQT_PLUGIN_METADATA_SECTION const uint32 qt_section_alignment_dummy = 42;\n\n"
			"#ifdef QT_NO_DEBUG\n", out );
		writePluginMetaData( out, data );

		fputs( "\n#else // QT_NO_DEBUG\n", out );

		data.remove( debugKey );
		data.insert( debugKey, QJsonValue( true ) );
		writePluginMetaData( out, data );

		fputs( "#endif // QT_NO_DEBUG\n\n", out );

		// 'Use' all namespaces.
		int pos = cdef->qualified.indexOf( "::" );
		for ( ; pos != -1; pos = cdef->qualified.indexOf( "::", pos + 2 ) )
			fprintf( out, "using namespace %s;\n", cdef->qualified.left( pos ).data() );
		fprintf( out, "QT_MOC_EXPORT_PLUGIN(%s, %s)\n\n",
			cdef->qualified.data(), cdef->classname.data() );
		*/
	}
}
