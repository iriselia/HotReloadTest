SET( DEFINE
)
SET( INCLUDE
Core
rapidjson
cereal
)
SET( LINK
Core
rapidjson
cereal
)
create_project(STATIC DEFINE INCLUDE LINK)
