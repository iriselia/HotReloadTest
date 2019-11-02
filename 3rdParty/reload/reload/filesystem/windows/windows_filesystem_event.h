#pragma once
#include <windows.h>

namespace reload
{
  namespace filesystem
  {
    enum windows_filesystem_event_types
    {
      CHANGE_LAST_WRITE = 0,
      CHANGE_CREATION,
      CHANGE_SIZE,
      CHANGE_FILE_NAME,
      CHANGE_NONE
    };

    struct windows_filesystem_event
    {
      std::uint32_t id;
      windows_filesystem_event_types event_type;
      const reload::filesystem::generic_file_data* file_data;
      DWORD file_action_type;

      windows_filesystem_event()
        : id(-1), event_type(CHANGE_NONE)
      {
      }
    };
  }
}