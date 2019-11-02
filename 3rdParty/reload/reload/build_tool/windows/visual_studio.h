#pragma once

#include "Windows.h"


#include "stdio_logger.h"
#include "assert.h"

#include "generic_compiler.h"
#include "windows_commandline.h"

namespace reload
{
  namespace runtime
  {
    class visual_studio : public generic_compiler
    {
    private:
      const reload::tstring object_file_extension = TEXT(".obj");
      volatile bool is_compilation_complete;

      logging::generic_logger* logger;
      windows_commandline windows_commandline;

    public:
      std::experimental::filesystem::path::string_type name;
      std::experimental::filesystem::path::string_type ide_version;
      int compiler_version;
      std::experimental::filesystem::path root_dir;
      std::experimental::filesystem::path build_tool_path;

      visual_studio()
      {
      }

      ~visual_studio()
      {
      }

      void initialize(logging::generic_logger * pLogger);
      bool is_complete();

      void compile(const std::vector<source_file*>&	source_files,
        const std::experimental::filesystem::path& module_name) override;

      void command(const std::string& command);

    };
    using compiler = visual_studio;
  }
  extern reload::runtime::compiler compiler;
}
