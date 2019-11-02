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

// ObjectInterfaceOerDllSource.cpp : Defines the entry point for the DLL application.
#include "object_interface_per_module.h"

namespace reload
{
  namespace runtime
  {
    /*
    per_module_interface* per_module_interface::ms_pObjectManager = NULL;

    SystemTable* per_module_interface::g_pSystemTable = 0;

    extern "C"
#ifdef _WIN32
      __declspec(dllexport)	//should create file with export import macros etc.
#else
      __attribute__((visibility("default")))
#endif
      generic_per_module_interface* GetPerModuleInterface()
    {
      return per_module_interface::GetInstance();
    }

    per_module_interface* per_module_interface::GetInstance()
    {
      if (!ms_pObjectManager)
      {
        ms_pObjectManager = new per_module_interface;
      }
      return ms_pObjectManager;
    }

    void per_module_interface::AddConstructor(generic_object_constructor* pConstructor)
    {
      m_ObjectConstructors.push_back(pConstructor);
    }

    std::vector<generic_object_constructor*>& per_module_interface::GetConstructors()
    {
      return m_ObjectConstructors;
    }

    void per_module_interface::SetProjectIdForAllConstructors(unsigned short projectId_)
    {
      for (size_t i = 0; i < m_ObjectConstructors.size(); ++i)
      {
        m_ObjectConstructors[i]->SetProjectId(projectId_);
      }
    }


    void per_module_interface::SetSystemTable(SystemTable* pSystemTable)
    {
      g_pSystemTable = pSystemTable;
    }

    per_module_interface::per_module_interface()
    {
      //ensure this file gets compiled
      AddRequiredSourceFiles(__FILE__);
    }

    const std::vector<const char*>& per_module_interface::GetRequiredSourceFiles() const
    {
      return m_RequiredSourceFiles;
    }

    void per_module_interface::AddRequiredSourceFiles(const char* file_)
    {
      m_RequiredSourceFiles.push_back(file_);
    }
    */
  }
}