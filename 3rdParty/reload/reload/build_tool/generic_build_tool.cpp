#include "generic_build_tool.h"

#include "generic_logger.h"
/*
namespace reload
{
    namespace runtime
    {
        void build_tool::clean(const std::experimental::filesystem::path& temporaryPath_) const
        {
            /*
            // Remove any existing intermediate directory
            std::experimental::filesystem::pathIterator pathIter(temporaryPath_);
            std::string obj_extension = compiler.GetObjectFileExtension();
            while (++pathIter)
            {
              if (pathIter.GetPath().Extension() == obj_extension)
              {
                if (logger)
                {
                  logger->LogInfo("Deleting temp RCC++ obj file: %s\n", pathIter.GetPath().c_str());
                }
                pathIter.GetPath().Remove();
              }
            }
            
        }


        void build_tool::initialize(logging::generic_logger * logger)
        {
            this->logger = logger;
            compiler.initialize(logger);
        }

        void build_tool::build_module(const std::vector<file_to_build>&		buildFileList_,
            const generic_compiler_options&					compiler_options_,
            std::vector<std::experimental::filesystem::path>		linkLibraryList_,
            const std::experimental::filesystem::path&			moduleName_)
        {
            // Initial version is very basic, simply compiles them.
            std::experimental::filesystem::path objectFileExtension = compiler.get_object_file_extension();
            std::vector<std::experimental::filesystem::path> compileFileList;			// List of files we pass to the compiler
            compileFileList.reserve(buildFileList_.size());
            std::vector<std::experimental::filesystem::path> forcedCompileFileList;		// List of files which must be compiled even if object file exists
            std::vector<std::experimental::filesystem::path> nonForcedCompileFileList;	// List of files which can be linked if already compiled

            // Seperate into seperate file lists of force and non-forced,
            // so we can ensure we don't have the same file in both
            for (size_t i = 0; i < buildFileList_.size(); ++i)
            {
                std::experimental::filesystem::path buildFile = buildFileList_[i].filePath;
                if (buildFileList_[i].forceCompile)
                {
                    if (find(forcedCompileFileList.begin(), forcedCompileFileList.end(), buildFile) == forcedCompileFileList.end())
                    {
                        forcedCompileFileList.push_back(buildFile);
                    }
                }
                else
                {
                    if (find(nonForcedCompileFileList.begin(), nonForcedCompileFileList.end(), buildFile) == nonForcedCompileFileList.end())
                    {
                        nonForcedCompileFileList.push_back(buildFile);
                    }
                }
            }

            // Add all forced compile files to build list
            for (size_t i = 0; i < forcedCompileFileList.size(); ++i)
            {
                compileFileList.push_back(forcedCompileFileList[i]);
            }

            // runtime folder needs to be aware of compilation level and debug/

            // Add non forced files, but only if they don't exist in forced compile list
            for (size_t i = 0; i < nonForcedCompileFileList.size(); ++i)
            {
                std::experimental::filesystem::path buildFile = nonForcedCompileFileList[i];
                if (find(forcedCompileFileList.begin(), forcedCompileFileList.end(), buildFile) == forcedCompileFileList.end())
                {
                    // Check if we have a pre-compiled object version of this file, and if so use that.
                    std::experimental::filesystem::path objectFileName = compiler_options_.intermediate_path / buildFile.filename();
                    objectFileName.replace_extension(objectFileExtension.c_str());

                    if (std::experimental::filesystem::exists(objectFileName) && std::experimental::filesystem::exists(buildFile))
                    {
                        /*
                        FileSystemUtils::filetime_t objTime = objectFileName.GetLastWriteTime();
                        if (objTime > buildFile.GetLastWriteTime())
                        {
                          // we only want to use the object file if it's newer than the source file
                          buildFile = objectFileName;
                        }
                        
                    }
                    compileFileList.push_back(buildFile);
                }
            }

            compiler.compile(compileFileList, compiler_options_, linkLibraryList_, moduleName_);
        }
    }
    reload::runtime::build_tool build_tool;

}
*/