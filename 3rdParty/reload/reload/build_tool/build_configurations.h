//
// Copyright (c) 2010-2011 Matthew Jack and Doug Binks
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#pragma once
#include "generic_platform.h"

namespace reload
{
  namespace runtime
  {
    enum class optimization_levels
    {
      optimization_level_default = 0,		// optimization_level_debug in DEBUG, optimization_level_release in release. This is the default state.
      optimization_level_debug,			// Low optimization, improve debug experiece. Default in DEBUG
      optimization_level_release,			// Optimization for performance, debug experience may suffer. Default in RELEASE
      optimization_level_unset,			// No optimization set in compile, so either underlying generic_compiler default or set through SetAdditionalCompileOptions
      optimization_level_min_size,			// Size of enum, do not use to set opt level
    };

    constexpr TCHAR* optimization_level_strings[] =
    {
      TEXT("DEFAULT"),
      TEXT("DEBUG"),
      TEXT("PERF"),
      TEXT("NOT_SET"),
    };

    // get_actual_optimization_level - translates DEFAULT into DEUG or PERF
    inline optimization_levels get_actual_optimization_level(optimization_levels optimization_level)
    {
      if (optimization_levels::optimization_level_default == optimization_level)
      {
#ifdef _DEBUG
        optimization_level = optimization_levels::optimization_level_debug;
#else
        optimization_level = optimization_levels::optimization_level_release;
#endif
      }
      return optimization_level;
    }
  }
}