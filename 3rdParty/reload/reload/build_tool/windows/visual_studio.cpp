
#include <windows.h>
#include <tchar.h>

#include <thread>

#include "generic_compiler.h"
#include "assert.h"
#include "stdio_logger.h"

#include "visual_studio.h"
#include "solution.h"

namespace reload
{
  namespace runtime
  {
    bool visual_studio::is_complete()
    {
      if (is_compilation_complete & !is_fast_compile_mode)
      {
        //clean_up_process_and_pipes();
      }
      return is_compilation_complete;
    }

    void visual_studio::initialize(logging::generic_logger* logger)
    {
      this->logger = logger;
      windows_commandline.initialize();

      switch (_MSC_VER)
      {
        case 1700:
        {
          name = TEXT("Visual Studio 2012");
          ide_version = TEXT("11.0");
          compiler_version = 1700;
          build_tool_path = TEXT("VC");
          break;
        }
        case 1800:
        {
          name = TEXT("Visual Studio 2013");
          ide_version = TEXT("12.0");
          compiler_version = 1800;
          build_tool_path = TEXT("VC");
          break;
        }
        case 1900:
        {
          name = TEXT("Visual Studio 2015");
          ide_version = TEXT("14.0");
          compiler_version = 1900;
          build_tool_path = TEXT("VC");
          break;
        }
        case 1910:
        {
          name = TEXT("Visual Studio 2017");
          ide_version = TEXT("15.0");
          compiler_version = 1910;
          build_tool_path = TEXT("VC\\Auxiliary\\Build\\");
          break;
        }
        default:
        {
          throw "Unsupported Visual Studio Version.";
          break;
        }
      }

      {
        //e.g.: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\<version>\Setup\VS\<edition>
        auto visual_studio_regkey_name = TEXT("SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7");
        TCHAR visual_studio_path[MAX_PATH];
        DWORD visual_studio_path_size = MAX_PATH;

        const TCHAR* key_name;
        HKEY key_handle;

        LONG retKeyVal = RegOpenKeyEx(
          HKEY_LOCAL_MACHINE,
          visual_studio_regkey_name,
          0,
          KEY_READ | KEY_WOW64_32KEY,
          &key_handle);

        LONG retVal = RegQueryValueEx(
          key_handle,
          ide_version.c_str(),
          NULL,
          NULL,
          (LPBYTE)visual_studio_path,
          &visual_studio_path_size
        );
        if (ERROR_SUCCESS == retVal)
        {
          root_dir = visual_studio_path;
          build_tool_path = root_dir / build_tool_path;
        }

        RegCloseKey(key_handle);
      }

#ifdef _WIN64
      std::string cmdSetParams = "@PROMPT $ \n\"" + build_tool_path.string() + "Vcvarsall.bat\" x86_amd64\n";
#else
      std::string cmdSetParams = "@PROMPT $ \n\"" + build_tool_path.string() + "Vcvarsall.bat\" x86\n";
#endif
      windows_commandline.write(cmdSetParams);

    }

    void visual_studio::compile(const std::vector<source_file*>&	source_files,
      const std::experimental::filesystem::path& module_name)
    {
      std::cerr << "Recompiling: " << std::endl;
      
      for (auto& i : source_files)
      {
        std::cerr << i->path << std::endl;
      }

      reload::solution.build_target.build_dir;

      is_compilation_complete = false;
      //optimization and c runtime

#ifdef _DEBUG
      reload::tstring compile_flags = TEXT("/nologo /Zi /FC /MDd /LDd ");
#else
      reload::tstring compile_flags = TEXT("/nologo /Zi /FC /MD /LD ");	//also need debug information in release
#endif
      std::string compiler_flags;
      for (auto& i : reload::solution.build_target.compiler_flags)
      {
        compiler_flags += i;
        compiler_flags += " ";
      }

      std::string linker_flags;
      for (auto& i : reload::solution.build_target.linker_flags)
      {
        linker_flags += i;
        linker_flags += " ";
      }

      int a = 0;
      a++;

      if (reload::solution.build_target.link_libraries.size())
      {
        linker_flags += "/link ";
        for (auto& i : reload::solution.build_target.link_libraries)
        {
          linker_flags += " /LIBPATH:\"";
          linker_flags += i.string().c_str();
          linker_flags += "\"";
        }

      }

      /*

      compile_flags += generic_compiler_options.compile_options;
      compile_flags += TEXT(" ");

      reload::tstring link_flags;
      bool have_link_options = (0 != generic_compiler_options.link_options.length());
      if (generic_compiler_options.library_dir_list.size() || have_link_options)
      {
        link_flags = TEXT(" /link ");
        for (size_t i = 0; i < generic_compiler_options.library_dir_list.size(); ++i)
        {
          link_flags += TEXT(" /LIBPATH:\"") + generic_compiler_options.library_dir_list[i].native() + TEXT("\"");
        }

        if (have_link_options)
        {
          link_flags += generic_compiler_options.link_options;
          link_flags += TEXT(" ");
        }
      }

      // Check for intermediate directory, create it if required
      // There are a lot more checks and robustness that could be added here
      if (!std::experimental::filesystem::exists(generic_compiler_options.intermediate_path))
      {
        bool success = std::experimental::filesystem::create_directory(generic_compiler_options.intermediate_path);
        if (success && logger)
        {
          logger->LogInfo("Created intermediate folder \"%s\"\n", generic_compiler_options.intermediate_path.c_str());
        }
        else if (logger)
        {
          logger->LogError("Error creating intermediate folder \"%s\"\n", generic_compiler_options.intermediate_path.c_str());
        }
      }

      //create include path search string
      reload::tstring include_file_strings;
      for (size_t i = 0; i < generic_compiler_options.include_dir_list.size(); ++i)
      {
        include_file_strings += TEXT(" /I \"") + generic_compiler_options.include_dir_list[i].native() + TEXT("\"");
      }

      // When using multithreaded compilation, listing a file for compilation twice can cause errors, hence
      // we do a final filtering of input here.
      // See http://msdn.microsoft.com/en-us/library/bb385193.aspx - "Source Files and Build Order"

      // Create compile path search string
      reload::tstring source_file_strings;
      std::set<reload::tstring> filtered_paths;
      for (size_t i = 0; i < files_to_compile.size(); ++i)
      {
        reload::tstring strPath = files_to_compile[i].native();
        std::transform(strPath.begin(), strPath.end(), strPath.begin(), ::tolower);

        std::set<reload::tstring>::const_iterator it = filtered_paths.find(strPath);
        if (it == filtered_paths.end())
        {
          source_file_strings += TEXT(" \"") + strPath + TEXT("\"");
          filtered_paths.insert(strPath);
        }
      }

      reload::tstring link_library_strings;
      for (size_t i = 0; i < link_library_list.size(); ++i)
      {
        link_library_strings += TEXT(" \"") + link_library_list[i].native() + TEXT("\" ");
      }

      std::experimental::filesystem::path pdbName = module_name;
      pdbName.replace_extension(".pdb");



      */
      // /MP - use multiple processes to compile if possible. Only speeds up compile for multiple files and not link
      std::string cmd;
      cmd += "cl ";
      cmd += compiler_flags;
#ifdef _UNICODE
      cmd += "/D UNICODE /D _UNICODE ";
#endif
      cmd += " /MP /Fo\"";
      /*
      cmd += generic_compiler_options.intermediate_path.native();
      cmd += TEXT("\\\\\" ");
      cmd += TEXT("/D WIN32 /EHa /Fe");
      cmd += module_name.native();
      cmd += TEXT(" /Fd");
      cmd += pdbName.native();
      cmd += TEXT(" ");
      cmd += include_file_strings;
      cmd += TEXT(" ");
      cmd += source_file_strings;
      cmd += link_library_strings;
      cmd += link_flags;
      cmd += TEXT("\necho ");

      if (logger)
      {
        logger->LogInfo("%s", cmd.c_str()); // use %s to prevent any tokens in compile string being interpreted as formating
      }

      cmd += windows_commandline.completion_token;
      cmd += TEXT("\n");
      */
      windows_commandline.write(cmd);

      //windows_commandline.write(cmd);

    }

    void visual_studio::command(const std::string& command)
    {
      windows_commandline.write(command);
    }
  }
  reload::runtime::compiler compiler;
}
