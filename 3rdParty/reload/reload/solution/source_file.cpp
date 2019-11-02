#include <fstream>

#include "rapidjson/prettywriter.h"
#include "source_file.h"
#include "solution.h"

namespace reload
{
    bool path_ends_with(const std::experimental::filesystem::path::string_type & lhs, const std::experimental::filesystem::path::string_type & rhs)
    {
        for (std::size_t i = 0, s1_max_index = lhs.size() - 1, s2_max_index = rhs.size() - 1; i < s2_max_index + 1; i++)
        {
            auto& s1_next = lhs[s1_max_index - i];
            auto& s2_next = rhs[s2_max_index - i];

            if (s1_next != s2_next)
            {
                return false;
            }
        }

        return true;
    }

    bool is_implementation_file(const std::experimental::filesystem::path & path)
    {
        static const std::experimental::filesystem::path::string_type cplusplus = TEXT(".c++");
        static const std::experimental::filesystem::path::string_type cpp = TEXT(".cpp");
        static const std::experimental::filesystem::path::string_type cxx = TEXT(".cxx");
        static const std::experimental::filesystem::path::string_type cc = TEXT(".cc");
        static const std::experimental::filesystem::path::string_type c = TEXT(".c");

        static const std::experimental::filesystem::path::string_type pchcpp = TEXT(".pch.cpp");
        static const std::experimental::filesystem::path::string_type pchc = TEXT(".pch.c");

        auto& entry_string = path.native();
        if (path_ends_with(entry_string, cplusplus)
            || path_ends_with(entry_string, cpp)
            || path_ends_with(entry_string, cxx)
            || path_ends_with(entry_string, cc)
            || path_ends_with(entry_string, c))
        {
            if (!path_ends_with(entry_string, pchcpp)
                && !path_ends_with(entry_string, pchc)
                // To utf 8 if on windows
                && !std::regex_match(path.string(), std::regex(".+(?:CMake).+Modules.+", std::regex::optimize))
                && !std::regex_match(path.string(), std::regex(".+(?:CMakeFiles).+", std::regex::optimize))
                && !std::regex_match(path.string(), std::regex(".+(?:enc_temp_folder).+", std::regex::optimize)))
            {
                return true;
            }
        }

        return false;
    }

    bool is_declaration_file(const std::experimental::filesystem::path & path)
    {
        static const std::experimental::filesystem::path::string_type hpp = TEXT(".hpp");
        static const std::experimental::filesystem::path::string_type inl = TEXT(".inl");
        static const std::experimental::filesystem::path::string_type h = TEXT(".h");

        static const std::experimental::filesystem::path::string_type generatedpchh = TEXT(".generated.pch.h");
        static const std::experimental::filesystem::path::string_type generatedh = TEXT(".generated.h");

        auto& entry_string = path.native();
        if (path_ends_with(entry_string, hpp)
            || path_ends_with(entry_string, inl)
            || path_ends_with(entry_string, h))
        {
            if (!path_ends_with(entry_string, generatedpchh)
                && !path_ends_with(entry_string, generatedh)
                && !std::regex_match(path.string(), std::regex(".+(?:CMake).+Modules.+", std::regex::optimize))
                && !std::regex_match(path.string(), std::regex(".+(?:enc_temp_folder).+", std::regex::optimize)))
            {
                return true;
            }
        }

        return false;
    }

    bool is_source_file(const std::experimental::filesystem::path & path)
    {
        return reload::is_implementation_file(path) || reload::is_declaration_file(path);
    }

    void source_file::extract_dependencies()
    {
        dependencies.clear();
        //std::cerr << "Updating dependencies for: " << path << std::endl;

        using tchar = char; //std::experimental::filesystem::path::value_type;
        using tstring = std::string; //std::experimental::filesystem::path::string_type;

        std::basic_ifstream<tchar, std::char_traits<tchar> > file(path);
        std::basic_regex<tchar> include_file_name_filter((R"(^#[\s\\]*include[\s\\]*[\"<]([^\">#]*)[\">])"), std::regex::optimize);
        std::match_results<tstring::const_iterator> string_match;
        if (file.is_open())
        {
            file.seekg (0, file.end);
            int length = file.tellg();
            file.seekg (0, file.beg);

            if (length == 0)
            {
                return;
            }

            auto content = tstring();
            content.resize(length);
            file.read(&content.front(), length);

            while (std::regex_search(content, string_match, include_file_name_filter))
            {
                tstring file_name = std::regex_replace(string_match.format(("$1")), std::basic_regex<tchar>((R"(\\|[\s]|\n)")), (""));
                if (reload::is_source_file(file_name))
                {
                    for (auto& i : reload::solution.build_target.include_directories)
                    {
                        auto file_path = std::experimental::filesystem::path(i) / file_name;
                        if (std::experimental::filesystem::exists(file_path))
                        {
                            auto file_itr = reload::solution.find_source_file(file_path);
                            if (file_itr != reload::solution.source_files.end())
                            {
                                dependencies.push_back(&(*file_itr));
                            }
                            break;
                        }
                    }
                }
                content = string_match.suffix();
            }
            file.close();
        }
    }

    void source_file::update_dependencies_mmap()
    {
        dependencies.clear();
        //std::cerr << "Updating dependencies for: " << path << std::endl;

        using tchar = char;// std::experimental::filesystem::path::value_type;
        using tstring = std::string; //std::experimental::filesystem::path::string_type;

        std::basic_ifstream<tchar, std::char_traits<tchar> > file(path);
        std::basic_regex<tchar> include_file_name_filter((R"(^#[\s\\]*include[\s\\]*[\"<]([^\">#]*)[\">])"), std::regex::optimize);
        std::match_results<tstring::const_iterator> string_match;
        if (file.is_open())
        {
            #define BUF_SIZE1 4096*1024 // 4 mb buffer
            LARGE_INTEGER size;
            size.QuadPart = 4096;
            DWORD dwflags;
            unsigned long lowDWordVal = 0;
            unsigned long highDWordVal = 0;
            unsigned long dwordSize = 4294967294;
            dwflags = GENERIC_READ;// | GENERIC_WRITE;
            auto h_file = CreateFile(path.native().c_str(), dwflags, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
            auto h_map_file = CreateFileMapping(h_file,    // use paging file
                NULL,                    // default security 
                PAGE_READONLY,          // read/write access
                0, //size.HighPart,                       // maximum object size (high-order DWORD) 
                0, //size.LowPart,                // maximum object size (low-order DWORD)  
                L"Outputmmap");                 // name of mapping object

            auto error = GetLastError();

            if (h_map_file == NULL)
            {
                std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
                std::cin.get();
            }

            auto pBuf = (char*)MapViewOfFile(h_map_file,   // handle to map object
                FILE_MAP_READ, // read/write permission
                highDWordVal,
                lowDWordVal,
                0);	//BUF_SIZE);
            /*
            const int file_size=file.seekg();
            const int page_size=0x1000;
            int off=0;
            void *data;

            int fd = open("filename.bin", O_RDONLY);

            while (off < file_size)
            {
                data = mmap(NULL, page_size, PROT_READ, 0, fd, off);
                // do stuff with data
                munmap(data, page_size);
                off += page_size;
            }
            */
            auto content = tstring(pBuf);
            CloseHandle(h_map_file);
            CloseHandle(h_file);
            //auto content = tstring((std::istreambuf_iterator<tchar>(file)), std::istreambuf_iterator<tchar>());

            /*
            while (std::regex_search(content, string_match, include_file_name_filter))
            {
                tstring file_name = std::regex_replace(string_match.format(("$1")), std::basic_regex<tchar>((R"(\\|[\s]|\n)")), (""));
                if (reload::is_source_file(file_name))
                {
                    //std::cerr << file_name << std::endl;
                    //dependencies.push_back(source_file(file_name));
                }
                content = string_match.suffix();
            }
            file.close();
            */
        }

    }

}
