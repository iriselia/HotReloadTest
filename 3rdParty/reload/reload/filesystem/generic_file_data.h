#pragma once
#include <filesystem>

namespace reload
{
    namespace filesystem
    {
        struct generic_file_data
        {
            std::experimental::filesystem::path path;
            std::experimental::filesystem::file_time_type last_write_time;

            generic_file_data(const std::experimental::filesystem::path& path, std::experimental::filesystem::file_time_type last_write_time)
                : path(path), last_write_time(last_write_time)
            {
            }

            generic_file_data(const std::experimental::filesystem::path&& path, std::experimental::filesystem::file_time_type last_write_time)
                : path(path), last_write_time(last_write_time)
            {
            }
        };
    }
}