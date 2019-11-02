#pragma once

#if PLATFORM_WINDOWS
#include "windows_filesystem_watcher.h"
#elif PLATFORM_MACOS
#include "macos_filesystem_watcher.h"
#elif PLATFORM_LINUX
#include "linux_filesystem_watcher.h"
#endif
