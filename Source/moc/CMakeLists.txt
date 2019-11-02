SET( DEFINE
_CRT_SECURE_NO_WARNINGS
)
SET( INCLUDE
docopt
rapidjson
Core
)
SET( LINK
docopt
rapidjson
Core
)

create_project(CONSOLE DEFINE INCLUDE LINK)

if(MSVC)
	add_definitions(/std:c++latest)
	add_definitions(/permissive-)
endif()