#pragma once


#include <filesystem>
#include "generic_logger.h"
#include "build_configurations.h"
#include "generic_platform.h"

#include "source_file.h"

namespace reload
{
  namespace runtime
  {
    struct generic_compiler_options
    {
      std::vector<std::experimental::filesystem::path> include_dir_list;
      std::vector<std::experimental::filesystem::path> library_dir_list;
      reload::tstring compile_options;
      reload::tstring link_options;
      optimization_levels optimizationLevel;
      std::experimental::filesystem::path base_intermediate_path;
      std::experimental::filesystem::path intermediate_path;
      std::experimental::filesystem::path compiler_location;
    };

    class generic_compiler
    {
    public:
      bool is_fast_compile_mode;

      generic_compiler()
      {
      }

      virtual ~generic_compiler()
      {
      }

      virtual void initialize(logging::generic_logger * pLogger)
      {
      }

      virtual void compile(const std::vector<source_file*>&	source_files,
        const std::experimental::filesystem::path& module_name)
      {
      }



      // On Win32 the compile command line process can be preserved in between compiles for improved performance,
      // however this can result in Zombie processes and also prevent handles such as sockets from being closed.
      // This function is safe to call at any time, but will only have an effect on Win32 compiles from the second
      // compile on after the call (as the first must launch the process and set the VS environment).
      //
      // Defaults to is_fast_compile_mode = false
      void set_fast_compile_mode(bool is_fast)
      {
        is_fast_compile_mode = is_fast;

        // call GetIsComplete() to ensure this stops process
        is_complete();
      }
      bool is_complete() const;
    };
  }
}
