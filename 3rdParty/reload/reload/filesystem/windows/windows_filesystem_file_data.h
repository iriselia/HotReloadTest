#pragma once
#include <filesystem>
#include <windows.h>
#include "generic_file_data.h"

namespace reload
{
  namespace filesystem
  {
    struct windows_file_data : public generic_file_data
    {
      bool is_dirty;
      std::size_t size;
      std::experimental::filesystem::file_time_type creation_time;

      windows_file_data(std::experimental::filesystem::path path, std::size_t size,
          std::experimental::filesystem::file_time_type creation_time, std::experimental::filesystem::file_time_type last_write_time, bool is_dirty = false)
        : generic_file_data(path, last_write_time), is_dirty(false), size(size), creation_time(creation_time)
      {
      }
    };
  }
}