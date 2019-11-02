#include <thread>
#include <regex>
#include <tchar.h>

#include "windows_commandline.h"

namespace reload
{
  namespace runtime
  {
    constexpr std::size_t bufsize = 4096;

    windows_commandline::windows_commandline()
    {
    }

    void windows_commandline::initialize()
    {
      SECURITY_ATTRIBUTES saAttr = { 0 };
      saAttr.bInheritHandle = TRUE;

      bool result = true;
      result &= CreatePipe(&StdInHandles[0], &StdInHandles[1], &saAttr, bufsize);
      result &= CreatePipe(&StdOutHandles[0], &StdOutHandles[1], &saAttr, bufsize);
      result &= CreatePipe(&StdErrHandles[0], &StdErrHandles[1], &saAttr, bufsize);
      //result &= SetHandleInformation(StdOutHandles[0], HANDLE_FLAG_INHERIT, 0);
      // Write handle must be un-inheritable for parent process to correctly exit
      result &= SetHandleInformation(StdInHandles[1], HANDLE_FLAG_INHERIT, 0);

      STARTUPINFO startup_info = { 0 };
      startup_info.dwFlags |= STARTF_USESTDHANDLES;
      startup_info.dwFlags |= CREATE_NO_WINDOW;
      startup_info.hStdInput = StdInHandles[0];
      startup_info.hStdOutput = StdOutHandles[1];
      startup_info.hStdError = StdErrHandles[1];

      reload::tstring cmd(TEXT("cmd"));
      command_line = _tcsdup(cmd.c_str());

      result &= CreateProcess(0, command_line, 0, 0, 1, 0, 0, 0, &startup_info, &child_process_info);

      if (!result)
      {
        throw "windows_commandline::initialize()";
      }

      //SetConsoleOutputCP(65001);
      //std::string cmd = "chcp 65001";
      //write(cmd);


      should_read = true;
      is_idle = true;
      read_thread = std::thread([&] { read(); });
      read_thread.detach();
    }

    windows_commandline::~windows_commandline()
    {
      should_read = false;
      bool result = true;

      TerminateProcess(child_process_info.hProcess, 0);
      TerminateThread(child_process_info.hThread, 0);

      CloseHandle(child_process_info.hProcess);
      CloseHandle(child_process_info.hThread);

      read_thread.join();

      result &= CloseHandle(StdOutHandles[0]);
      result &= CloseHandle(StdOutHandles[1]);
      result &= CloseHandle(StdErrHandles[0]);
      result &= CloseHandle(StdErrHandles[1]);
      result &= CloseHandle(StdInHandles[0]);
      result &= CloseHandle(StdInHandles[1]);

      memset(&child_process_info, 0, sizeof(child_process_info));
    }

    void windows_commandline::write(const std::string& input)
    {
      std::lock_guard<std::mutex> lock(command_mutex);
      commands.push(input);

      return;

      if (!is_idle)
      {
        return;
      }

      DWORD num_bytes_written;
      is_idle = false;
      std::string cmd = input + "\n";
      static std::string echo_completion_token = std::string("echo ") + completion_token + "\n";
      WriteFile(StdInHandles[1], cmd.c_str(), cmd.size(), &num_bytes_written, NULL);
      WriteFile(StdInHandles[1], echo_completion_token.c_str(), (DWORD)echo_completion_token.size(), &num_bytes_written, NULL);

    }

    // utf-16 just doesn't work with cmd
    /*
    void windows_commandline::write(reload::tstring& input)
    {
      DWORD num_bytes_written;
      static const BYTE g_pnByteOrderMark[] = { 0xFF, 0xFE }; // UTF-16, Little Endian
      WriteFile(StdInHandles[1], g_pnByteOrderMark, sizeof g_pnByteOrderMark, &num_bytes_written, NULL);
      WriteFile(StdInHandles[1], (input + TEXT("\n")).c_str(), ((DWORD)input.size() + 1) * sizeof(input[0]), &num_bytes_written, NULL);
    }
    */

    void windows_commandline::read()
    {
      DWORD dwRead, dwWritten;
      std::string buffer(bufsize, 0);
      BOOL bSuccess = FALSE;
      HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

      while (should_read)
      {
        if (is_idle)
        {
          std::lock_guard<std::mutex> lock(command_mutex);
          if (commands.size())
          {
            std::string& input = commands.front();

            DWORD num_bytes_written;
            std::string cmd = input + "\n";
            static std::string echo_completion_token = std::string("echo ") + completion_token + "\n";
            WriteFile(StdInHandles[1], cmd.c_str(), cmd.size(), &num_bytes_written, NULL);
            WriteFile(StdInHandles[1], echo_completion_token.c_str(), (DWORD)echo_completion_token.size(), &num_bytes_written, NULL);

            commands.pop();
            is_idle = false;
          }
          else
          {
          }
        }
        else
        {
          // blocks until something turns up
          buffer.resize(bufsize);
          bSuccess = ReadFile(StdOutHandles[0], &buffer[0], bufsize - 1, &dwRead, NULL);
          if (!bSuccess || dwRead == 0)
          {
            throw("[RuntimeCompiler] Redirect of compile output failed on read\n");
          }

          // print to parent process std out
          //*
          buffer.resize(bufsize);
          bSuccess = WriteFile(hParentStdOut, &buffer[0], dwRead, &dwWritten, NULL);
          if (!bSuccess)
          {
            throw("[RuntimeCompiler] Redirect of compile output failed on write\n");
          }
          //*/

          buffer.resize(dwRead);
          std::match_results<std::string::const_iterator> string_match;
          
          //static std::regex regex_completion_token = std::regex("_RELOAD_COMPLETION_TOKEN_$");
          static std::regex regex_completion_token = std::regex("[\\n]" + completion_token + "|^" + completion_token);
          while (std::regex_search(buffer, string_match, regex_completion_token))
          {
            buffer = string_match.suffix();
            is_idle = true;
            //break;
          }
        }
      }
    }
  }
}
