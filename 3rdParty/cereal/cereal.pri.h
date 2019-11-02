#include "generic_object.h"
#include "generic_factory.h"
#include "solution.h"
#include "source_file.h"

#define ENABLE_HOT_RELOAD() \
  reload::solution.build_target.optimization_level_string = CMAKE_INTDIR; \
  reload::solution.build_target.source_dir = _ROOT_DIR "/" _TARGET_PATH; \
  reload::solution.build_target.build_dir = _BUILD_DIR "/" _TARGET_PATH; \
  reload::solution.build_target.intermediate_dir = _BUILD_DIR "/" _TARGET_PATH "/" _PROJECT_NAME ".reload"; \
  reload::solution.build_target.include_directories = _INCLUDE_DIRS; \
  reload::solution.build_target.link_libraries = _LINK_LIBS; \
  reload::solution.build_target.compiler_flags = _COMPILER_FLAGS; \
  reload::solution.build_target.linker_flags = _LINKER_FLAGS; \
  \
  reload::solution.build_target.initialize();

struct hot_reloadable_source_file
{
    hot_reloadable_source_file(const char* file_name)
    {
        auto source_itr = reload::runtime::solution::get().find_source_file(file_name);
        if (source_itr != reload::runtime::solution::get().source_files.end())
        {
            auto& source_file = *source_itr;
            reload::runtime::solution::get().hot_reloadable_source_files.push_back(&source_file);
        }
    }
};

#define _RELOAD_CONCAT_(x, y)  x ## y
#define _RELOAD_CONCAT(x, y) _RELOAD_CONCAT_(x, y)
#define register_file(x) hot_reloadable_source_file _RELOAD_CONCAT(hot_reloadable_source_file_, __COUNTER__)(x)

#define enable_hot_reload(class_name, alias) \
    static_list_new(reload::runtime::generic_factory::constructor_index); \
    reload::runtime::generic_constructor reload::util::static_list< \
      reload::runtime::generic_constructor, \
      reload::runtime::generic_factory::constructor_index>::data = reload::runtime::constructor<class_name>(alias); \
    \
    namespace reload\
    {\
        namespace runtime\
        {\
            register_file(__FILE__); \
        }\
    }

#define enable_hot_reload_ex(class_name) \
    static_list_new(reload::runtime::generic_factory::constructor_index); \
    reload::runtime::generic_constructor reload::util::static_list< \
      reload::runtime::generic_constructor, \
      reload::runtime::generic_factory::constructor_index>::data = reload::runtime::constructor<class_name>();
