#pragma once
#include "windows.h"
#include "reload/common/generic_platform.h"
#include <thread>
#include <queue>
#include <mutex>

namespace reload
{
  namespace runtime
  {
    class windows_commandline
    {
    private:

      PROCESS_INFORMATION child_process_info;

      TCHAR* command_line;
      
      HANDLE StdInHandles[2];
      HANDLE StdOutHandles[2];
      HANDLE StdErrHandles[2];

      std::mutex command_mutex;
      std::queue<std::string> commands;

      std::thread read_thread;
      volatile bool should_read;
      void read();

    public:
      volatile bool is_idle;

      const std::string completion_token = "_RELOAD_COMPLETION_TOKEN_";

      windows_commandline();

      ~windows_commandline();

      void initialize();

      void write(const std::string& input);
      //void write(reload::tstring& input);

    };
  }
}
