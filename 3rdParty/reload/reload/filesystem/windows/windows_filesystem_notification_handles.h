#pragma once
#include <windows.h>

namespace reload
{
  namespace filesystem
  {
    constexpr static int filesystem_notification_types[] = { FILE_NOTIFY_CHANGE_LAST_WRITE, FILE_NOTIFY_CHANGE_CREATION, FILE_NOTIFY_CHANGE_SIZE, FILE_NOTIFY_CHANGE_FILE_NAME };
    constexpr static size_t filesystem_notification_type_count = sizeof(filesystem_notification_types) / sizeof(int);

    struct windows_filesystem_notification_handles
    {
      HANDLE handles[filesystem_notification_type_count] = { 0 };

      HANDLE& operator[](size_t index)
      {
        return handles[index];
      }

      HANDLE* operator&()
      {
        return handles;
      }
      windows_filesystem_notification_handles()
      {
      }

      windows_filesystem_notification_handles(windows_filesystem_notification_handles&& other)
      {
        for (int i = 0; i < filesystem_notification_type_count; i++)
        {
          std::swap(handles[i], other.handles[i]);
        }
      }

      windows_filesystem_notification_handles& operator=(windows_filesystem_notification_handles&& other)
      {
        for (int i = 0; i < filesystem_notification_type_count; i++)
        {
          std::swap(handles[i], other.handles[i]);
        }

        return *this;
      }

      ~windows_filesystem_notification_handles()
      {
        for (auto handle : handles)
        {
          if (handle)
          {
            FindCloseChangeNotification(handle);
          }
        }
      }
    };
  }
}