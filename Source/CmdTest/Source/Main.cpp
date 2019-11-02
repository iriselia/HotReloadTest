#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#include <tchar.h>

#include <thread>

constexpr std::size_t bufsize = 4096;

HANDLE StdInHandles[2];
HANDLE StdOutHandles[2];
HANDLE StdErrHandles[2];

void CreateChildProcess(void);
void WriteToPipe(void);
void ReadFromPipe(void);
void ErrorExit(PTSTR);

int _tmain(int argc, TCHAR *argv[])
{
  SECURITY_ATTRIBUTES saAttr = {0};
  saAttr.bInheritHandle = TRUE;
  
  bool result = true;
  result &= CreatePipe(&StdInHandles[0], &StdInHandles[1], &saAttr, bufsize);
  result &= CreatePipe(&StdOutHandles[0], &StdOutHandles[1], &saAttr, bufsize);
  result &= CreatePipe(&StdErrHandles[0], &StdErrHandles[1], &saAttr, bufsize);
  //result &= SetHandleInformation(StdOutHandles[0], HANDLE_FLAG_INHERIT, 0);
  //result &= SetHandleInformation(StdInHandles[1], HANDLE_FLAG_INHERIT, 0);

  CreateChildProcess();

  std::thread([&] { ReadFromPipe(); }).detach();

  WriteToPipe();

  return 0;
}

void CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
  TCHAR* szCmdline = _tcsdup(TEXT("cmd"));
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  BOOL bSuccess = FALSE;

  // Set up members of the PROCESS_INFORMATION structure. 

  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  // Set up members of the STARTUPINFO structure. 
  // This structure specifies the STDIN and STDOUT handles for redirection.

  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.hStdInput = StdInHandles[0];
  siStartInfo.hStdOutput = StdOutHandles[1];
  siStartInfo.hStdError = StdErrHandles[1];
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  // Create the child process. 

  bSuccess = CreateProcess(NULL,
    szCmdline,     // command line 
    NULL,          // process security attributes 
    NULL,          // primary thread security attributes 
    TRUE,          // handles are inherited 
    0,             // creation flags 
    NULL,          // use parent's environment 
    NULL,          // use parent's current directory 
    &siStartInfo,  // STARTUPINFO pointer 
    &piProcInfo);  // receives PROCESS_INFORMATION 

                   // If an error occurs, exit the application. 
  if (!bSuccess)
    ErrorExit(TEXT("CreateProcess"));

  else
  {
    // Close handles to the child process and its primary thread.
    // Some applications might keep these handles to monitor the status
    // of the child process, for example. 

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);
  }
}

void WriteToPipe(void)

{
  DWORD dwRead, dwWritten;
  CHAR chBuf[bufsize];
  BOOL bSuccess = FALSE;

  HANDLE hParentStdIn = GetStdHandle(STD_INPUT_HANDLE);

  for (;;)
  {
    bSuccess = ReadFile(hParentStdIn, chBuf, bufsize, &dwRead, NULL);
    if (!bSuccess || dwRead == 0) break;

    bSuccess = WriteFile(StdInHandles[1], chBuf, dwRead, &dwWritten, NULL);
    if (!bSuccess) break;
  }

  if (!CloseHandle(StdInHandles[1]))
    ErrorExit(TEXT("StdInWr CloseHandle"));
}

void ReadFromPipe(void)
{
  DWORD dwRead, dwWritten;
  CHAR chBuf[bufsize];
  BOOL bSuccess = FALSE;
  HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

  for (;;)
  {
    bSuccess = ReadFile(StdOutHandles[0], chBuf, bufsize, &dwRead, NULL);
    if (!bSuccess || dwRead == 0) break;

    bSuccess = WriteFile(hParentStdOut, chBuf,
      dwRead, &dwWritten, NULL);
    if (!bSuccess) break;
  }
}

void ErrorExit(PTSTR lpszFunction)

{
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();

  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dw,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR)&lpMsgBuf,
    0, NULL);

  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
    (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
  StringCchPrintf((LPTSTR)lpDisplayBuf,
    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
    TEXT("%s failed with error %d: %s"),
    lpszFunction, dw, lpMsgBuf);
  MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
  ExitProcess(1);
}
