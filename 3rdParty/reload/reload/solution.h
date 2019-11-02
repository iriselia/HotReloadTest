#pragma once
#include <filesystem>
#include <vector>
#include <unordered_set>

#include "reload/filesystem.h"

namespace reload
{
  struct source_file;
  namespace runtime
  {

  }

  namespace runtime
  {
    class solution
    {
    private:
      reload::filesystem_watcher filesystem_watcher;

      void filesystem_callback(const reload::filesystem::generic_file_data& file_data, const reload::filesystem_event_types event);

      void initialize_include_graph();



    public:
      solution()
        : root_dir(_ROOT_DIR), source_tree_cache_path(_BINARIES_DIR "/" _PROJECT_NAME ".reload")
      {
      }

      struct build_target
      {
        build_target()
        {
        }
        /*
        build_target()
        : optimization_level(optimization_levels::optimization_level_unknown)
        , optimization_level_string(CMAKE_INTDIR)
        , source_dir(_ROOT_DIR "/" _TARGET_PATH)
        , build_dir(_BUILD_DIR "/" _TARGET_PATH)
        , intermediate_dir(_BUILD_DIR "/" _TARGET_PATH "/" _PROJECT_NAME ".reload")
        , include_directories(_INCLUDE_DIRS)
        , link_libraries(_LINK_LIBS)
        , compiler_flags(_COMPILER_FLAGS)
        , linker_flags(_LINKER_FLAGS)
        {
        }
        */

      public:
        enum class optimization_levels
        {
          optimization_level_unknown = 0,
          optimization_level_debug,
          optimization_level_min_size_rel,
          optimization_level_release,
          optimization_level_rel_with_deb_info
        };

        optimization_levels optimization_level;
        std::string optimization_level_string;
        std::experimental::filesystem::path source_dir;
        std::experimental::filesystem::path build_dir;
        std::experimental::filesystem::path intermediate_dir;
        std::vector<std::experimental::filesystem::path> include_directories;
        std::vector<std::experimental::filesystem::path> link_libraries;
        std::vector<std::string> compiler_flags;
        std::vector<std::string> linker_flags;
        bool initialized = false;

        void initialize()
        {
          if (optimization_level_string == "Debug")
          {
            optimization_level = optimization_levels::optimization_level_debug;
          }
          else if (optimization_level_string == "MinSizeRel")
          {
            optimization_level = optimization_levels::optimization_level_min_size_rel;
          }
          else if (optimization_level_string == "Release")
          {
            optimization_level = optimization_levels::optimization_level_release;
          }
          else if (optimization_level_string == "RelWithDebInfo")
          {
            optimization_level = optimization_levels::optimization_level_rel_with_deb_info;
          }
          else
          {
            optimization_level = optimization_levels::optimization_level_unknown;
          }

          initialized = true;
        }
      };

      const std::experimental::filesystem::path root_dir;
      const std::experimental::filesystem::path source_tree_cache_path;
      std::vector<reload::source_file> source_files;
      std::vector<reload::source_file*> hot_reloadable_source_files;

      build_target build_target;

      // safe to use at all times
      static reload::runtime::solution& get();


      void initialize();
      void update_source_tree();
      std::vector<source_file>::iterator find_source_file(const std::experimental::filesystem::path& path);
    };
  }

  // don't use until main
  extern reload::runtime::solution solution;
}