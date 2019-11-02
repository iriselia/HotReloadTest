#pragma once

#include <filesystem>
#include "generic_compiler.h"
#include "stdio_logger.h"

namespace reload
{
    namespace runtime
    {
      class generic_build_tool
      {
      public:
        generic_build_tool() = default;
        ~generic_build_tool() = default;

        void initialize(logging::generic_logger * pLogger);

        // clean - cleans up the intermediate files
        void clean(const std::experimental::filesystem::path& temporaryPath_) const;

        struct file_to_build
        {
          file_to_build(const std::experimental::filesystem::path& filePath_)
            : filePath(filePath_)
            , forceCompile(false)
          {
          }
          file_to_build(const std::experimental::filesystem::path& filePath_, bool forceCompile_)
            : filePath(filePath_)
            , forceCompile(forceCompile_)
          {
          }
          std::experimental::filesystem::path	filePath;
          bool forceCompile = false; //if true the file is compiled even if object file is present
        };

        void build_module(const std::vector<file_to_build>& buildFileList_, const generic_compiler_options& compiler_options_,
          std::vector<std::experimental::filesystem::path> linkLibraryList_, const std::experimental::filesystem::path& moduleName_);

        bool is_complete()
        {
          return false;// return compiler.is_complete();
        }

        void set_fast_compile_mode(bool bFast)
        {
          //compiler.set_fast_compile_mode(bFast);
        }

        // safe to use at all times
        static reload::runtime::generic_build_tool& get();


      private:
        logging::generic_logger*	logger;
      };
    }

    // don't use until main
    extern reload::runtime::generic_build_tool generic_build_tool;

  }
