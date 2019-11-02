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

#include "ConsoleGame.h"


#include "../AUArray.h"
#include "../BuildTool.h"
#include "../ICompilerLogger.h"
#include "../FileChangeNotifier.h"
#include "IObjectFactorySystem.h"
#include "ObjectFactorySystem.h"
#include "RuntimeObjectSystem.h"

#include "StdioLogSystem.h"

#include "IObject.h"
#include "IUpdateable.h"
#include "InterfaceIds.h"

#include <iostream>
#ifdef WIN32
#include <conio.h>
#include <tchar.h>
#else
#include <unistd.h>
int _getche()
{
  int ret = getchar();
  return ret;
}
int _kbhit()
{
  std::cout << "This port needs a fix, CTRL-C to quit\n";
  return 0;
}

int Sleep(int msecs)
{
  return usleep(msecs * 1000);
}
#endif
#include <sstream>
#include <vector>
#include <algorithm>
#include <string>
#include <stdarg.h>

// Remove windows.h define of GetObject which conflicts with EntitySystem GetObject
#if defined _WINDOWS_ && defined GetObject
#undef GetObject
#endif

ConsoleGame::ConsoleGame()
  : CompilerLogger(std::make_shared<StdioLogSystem>())
  , _RuntimeObjectSystem(std::make_shared<RuntimeObjectSystem>())
  , Updateable()
{
}

ConsoleGame::~ConsoleGame()
{
  if (_RuntimeObjectSystem)
  {
    // clean temp object files
    _RuntimeObjectSystem->CleanObjectFiles();
  }

  if (_RuntimeObjectSystem && _RuntimeObjectSystem->GetObjectFactorySystem())
  {
    _RuntimeObjectSystem->GetObjectFactorySystem()->RemoveListener(this);

    // delete object via correct interface
    IObject* pObj = _RuntimeObjectSystem->GetObjectFactorySystem()->GetObject(ObjectId);
    delete pObj;
  }
}


bool ConsoleGame::Init()
{
  //Initialise the RuntimeObjectSystem
  if (!_RuntimeObjectSystem->Initialise(CompilerLogger.get(), 0))
  {
    _RuntimeObjectSystem = 0;
    return false;
  }
  _RuntimeObjectSystem->GetObjectFactorySystem()->AddListener(this);


  // construct first object
  IObjectConstructor* pCtor = _RuntimeObjectSystem->GetObjectFactorySystem()->GetConstructor("RuntimeObject01");
  if (pCtor)
  {
    IObject* pObj = pCtor->Construct();
    pObj->GetInterface(&Updateable);
    if (0 == Updateable)
    {
      delete pObj;
      CompilerLogger->LogError("Error - no updateable interface found\n");
      return false;
    }
    ObjectId = pObj->GetObjectId();

  }

  return true;
}

void ConsoleGame::OnConstructorsAdded()
{
  // This could have resulted in a change of object pointer, so release old and get new one.
  if (Updateable)
  {
    IObject* pObj = _RuntimeObjectSystem->GetObjectFactorySystem()->GetObject(ObjectId);
    pObj->GetInterface(&Updateable);
    if (0 == Updateable)
    {
      delete pObj;
      CompilerLogger->LogError("Error - no updateable interface found\n");
    }
  }
}



bool ConsoleGame::MainLoop()
{
  //check status of any compile
  if (_RuntimeObjectSystem->GetIsCompiledComplete())
  {
    // load module when compile complete
    _RuntimeObjectSystem->LoadCompiledModule();
  }

  if (!_RuntimeObjectSystem->GetIsCompiling())
  {
    static int numUpdates = 0;
    std::cout << "\nMain Loop - press q to quit. Updates every second. Update: " << numUpdates++ << "\n";
    if (_kbhit())
    {
      int ret = _getche();
      if ('q' == ret)
      {
        return false;
      }
    }
    const float deltaTime = 1.0f;
    _RuntimeObjectSystem->GetFileChangeNotifier()->Update(deltaTime);
    Updateable->Update(deltaTime);
    Sleep(1000);
  }

  return true;
}
