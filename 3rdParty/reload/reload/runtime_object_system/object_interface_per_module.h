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

#ifndef OBJECTINTERFACEPERMODULE_INCLUDED
#define OBJECTINTERFACEPERMODULE_INCLUDED

#include "generic_object.h"
#include "runtime_include.h"
#include "runtime_link_library.h"
#include "runtime_source_dependency.h"
#include <string>
#include <vector>
#include <assert.h>

#ifndef RCCPPOFF
#define AU_ASSERT( statement )  do { if (!(statement)) { volatile int* p = 0; int a = *p; if(a) {} } } while(0)
#else
#define AU_ASSERT( statement ) assert( statement )
#endif //RCCPPOFF

namespace reload
{
  namespace runtime
  {
    /*
    class per_module_interface : public generic_per_module_interface
    {
    public:
      static per_module_interface*  GetInstance();
      static SystemTable*			g_pSystemTable;

      void AddConstructor(generic_object_constructor* pConstructor);

      virtual std::vector<generic_object_constructor*>& GetConstructors();
      virtual void SetProjectIdForAllConstructors(unsigned short projectId_);
      virtual void SetSystemTable(SystemTable* pSystemTable);

      SystemTable* GetSystemTable()
      {
        return g_pSystemTable;
      }

      virtual const std::vector<const char*>& GetRequiredSourceFiles() const;
      virtual void AddRequiredSourceFiles(const char* file_);
      virtual void SetModuleFileName(const char* name)
      {
        m_ModuleFilename = name;
      }
      const char* GetCompiledPath() const
      {
#ifdef COMPILE_PATH
        return COMPILE_PATH;
#else
        return "";
#endif
      }

    private:
      per_module_interface();

      ~per_module_interface()
      {
      }


      static per_module_interface*			ms_pObjectManager;
      std::vector<generic_object_constructor*>	m_ObjectConstructors;
      std::vector<const char*>			m_RequiredSourceFiles;
      std::string                         m_ModuleFilename;
    };





    template<typename T> class TObjectConstructorConcrete : public generic_object_constructor
    {
    public:
      TObjectConstructorConcrete(
#ifndef RCCPPOFF
        const char* Filename,
        generic_runtime_include_file_list*        pIncludeFileList_,
        generic_runtime_source_dependency_list*   pSourceDependencyList_,
        generic_runtime_link_library_list*        pLinkLibraryList,
#endif
        bool                            bIsSingleton,
        bool                            bIsAutoConstructSingleton)
        : m_bIsSingleton(bIsSingleton)
        , m_bIsAutoConstructSingleton(bIsAutoConstructSingleton)
        , m_pModuleInterface(0)
        , m_Project(0)
#ifndef RCCPPOFF
        , m_FileName(Filename)
        , m_pIncludeFileList(pIncludeFileList_)
        , m_pSourceDependencyList(pSourceDependencyList_)
        , m_pLinkLibraryList(pLinkLibraryList)
#endif
      {
#ifndef RCCPPOFF
        // add path to filename
#ifdef COMPILE_PATH
        m_FileName = COMPILE_PATH + m_FileName;
#endif
#endif
        per_module_interface::GetInstance()->AddConstructor(this);
        m_pModuleInterface = per_module_interface::GetInstance();
        m_Id = object_id::invalid_id;
      }

      virtual generic_object* Construct()
      {
        T* pT = 0;
        if (m_bIsSingleton && m_ConstructedObjects.size() && m_ConstructedObjects[0])
        {
          return m_ConstructedObjects[0];
        }

        if (m_FreeIds.empty())
        {
          object_id::per_type_object_id id = m_ConstructedObjects.size();

          pT = new T();
          pT->SetPerTypeId(id);
          m_ConstructedObjects.push_back(pT);
        }
        else
        {
          object_id::per_type_object_id id = m_FreeIds.back();
          m_FreeIds.pop_back();
          pT = new T();
          pT->SetPerTypeId(id);
          AU_ASSERT(0 == m_ConstructedObjects[id]);
          m_ConstructedObjects[id] = pT;

        }
        return pT;
      }

      virtual void ConstructNull()
      {
        // should not occur for singletons
        AU_ASSERT(!m_bIsSingleton);
        m_ConstructedObjects.push_back(NULL);
      }

      virtual const char* GetName()
      {
        return T::GetTypeNameStatic();
      }

      virtual void SetProjectId(unsigned short projectId_)
      {
        m_Project = projectId_;
      }

      virtual unsigned short GetProjectId() const
      {
        return m_Project;
      }

      virtual const char* GetFileName()
      {
#ifndef RCCPPOFF
        return m_FileName.c_str();
#else
        return 0;
#endif
      }

      virtual const char* GetCompiledPath()
      {
#ifndef RCCPPOFF
#ifdef COMPILE_PATH
        return COMPILE_PATH;
#else
        return "";
#endif
#else
        return 0;
#endif
      }

      virtual const char* GetIncludeFile(size_t Num_) const
      {
#ifndef RCCPPOFF
        if (m_pIncludeFileList)
        {
          return m_pIncludeFileList->GetIncludeFile(Num_);
        }
#endif
        return 0;
      }

      virtual size_t GetMaxNumIncludeFiles() const
      {
#ifndef RCCPPOFF
        if (m_pIncludeFileList)
        {
          return m_pIncludeFileList->MaxNum;
        }
#endif
        return 0;
      }

      virtual const char* GetLinkLibrary(size_t Num_) const
      {
#ifndef RCCPPOFF
        if (m_pLinkLibraryList)
        {
          return m_pLinkLibraryList->GetLinkLibrary(Num_);
        }
#endif
        return 0;
      }

      virtual size_t GetMaxNumLinkLibraries() const
      {
#ifndef RCCPPOFF
        if (m_pLinkLibraryList)
        {
          return m_pLinkLibraryList->MaxNum;
        }
#endif
        return 0;
      }

      virtual source_dependency_info GetSourceDependency(size_t Num_) const
      {
#ifndef RCCPPOFF
        if (m_pSourceDependencyList)
        {
          return m_pSourceDependencyList->GetSourceDependency(Num_);
        }
#endif
        return source_dependency_info::GetNULL();
      }

      virtual size_t GetMaxNumSourceDependencies() const
      {
#ifndef RCCPPOFF
        if (m_pSourceDependencyList)
        {
          return m_pSourceDependencyList->MaxNum;
        }
#endif
        return 0;
      }

      virtual bool GetIsSingleton() const
      {
        return m_bIsSingleton;
      }
      virtual bool        GetIsAutoConstructSingleton() const
      {
        return m_bIsSingleton && m_bIsAutoConstructSingleton;
      }


      virtual generic_object* GetConstructedObject(object_id::per_type_object_id id) const
      {
        if (m_ConstructedObjects.size() > id)
        {
          return m_ConstructedObjects[id];
        }
        return 0;
      }
      virtual size_t	 GetNumberConstructedObjects() const
      {
        return m_ConstructedObjects.size();
      }
      virtual object_id::constructor_object_id GetConstructorId() const
      {
        return m_Id;
      }
      virtual void SetConstructorId(object_id::constructor_object_id id)
      {
        if (object_id::invalid_id == m_Id)
        {
          m_Id = id;
        }
      }

      void DeRegister(object_id::per_type_object_id id)
      {
        //remove from constructed objects.
        //use swap with last one
        if (m_ConstructedObjects.size() - 1 == id)
        {
          //it's the last one, just remove it.
          m_ConstructedObjects.pop_back();
        }
        else
        {
          m_FreeIds.push_back(id);
          m_ConstructedObjects[id] = 0;
        }
      }
      virtual void ClearIfAllDeleted()
      {
        m_FreeIds.clear();
        m_ConstructedObjects.clear();
      }

    private:
      bool                            m_bIsSingleton;
      bool                            m_bIsAutoConstructSingleton;
      std::vector<T*>                 m_ConstructedObjects;
      std::vector<object_id::per_type_object_id>	m_FreeIds;
      object_id::constructor_object_id                   m_Id;
      per_module_interface*             m_pModuleInterface;
      unsigned short                  m_Project;
#ifndef RCCPPOFF
      std::string                     m_FileName;
      generic_runtime_include_file_list*        m_pIncludeFileList;
      generic_runtime_source_dependency_list*   m_pSourceDependencyList;
      generic_runtime_link_library_list*        m_pLinkLibraryList;
#endif
    };


    template<typename T> class TActual : public T
    {
    public:
      // overload new/delete to get alignment correct
#ifdef _WIN32
      void* operator new(size_t size)
      {
        size_t align = __alignof(TActual<T>);
        return _aligned_malloc(size, align);
      }
      void operator delete(void* p)
      {
        _aligned_free(p);
      }
#else
      void* operator new(size_t size)
      {
        size_t align = __alignof__(TActual<T>);
        void* pRet;
        int retval = posix_memalign(&pRet, align, size);
        (void)retval;	//unused
        return pRet;
      }
      void operator delete(void* p)
      {
        free(p);
      }
#endif //_WIN32
      friend class TObjectConstructorConcrete<TActual>;
      virtual ~TActual()
      {
        constructor.DeRegister(m_Id);
      }
      virtual object_id::per_type_object_id GetPerTypeId() const
      {
        return m_Id;
      }
      virtual generic_object_constructor* GetConstructor() const
      {
        return &constructor;
      }
      static const char* GetTypeNameStatic();
      virtual const char* GetTypeName() const
      {
        return GetTypeNameStatic();
      }

      static TObjectConstructorConcrete<TActual> constructor;

    private:
      void SetPerTypeId(object_id::per_type_object_id id)
      {
        m_Id = id;
      }
      object_id::per_type_object_id m_Id;
    };
#ifndef RCCPPOFF
#define REGISTERBASE( T, bIsSingleton, bIsAutoConstructSingleton )	\
    static RuntimeIncludeFiles< __COUNTER__ >       g_includeFileList_##T; \
    static runtime_source_dependency< __COUNTER__ >   g_sourceDependencyList_##T; \
    static runtime_link_library< __COUNTER__ >        g_linkLibraryList_##T; \
  template<> TObjectConstructorConcrete< TActual< T > > TActual< T >::constructor( __FILE__, &g_includeFileList_##T, &g_sourceDependencyList_##T, &g_linkLibraryList_##T, bIsSingleton, bIsAutoConstructSingleton );\
  template<> const char* TActual< T >::GetTypeNameStatic() { return #T; } \
  template class TActual< T >;
#else
#define REGISTERBASE( T, bIsSingleton, bIsAutoConstructSingleton )	\
  template<> TObjectConstructorConcrete< TActual< T > > TActual< T >::constructor( bIsSingleton, bIsAutoConstructSingleton); \
  template<> const char* TActual< T >::GetTypeNameStatic() { return #T; } \
  template class TActual< T >;
#endif

    //NOTE: the file macro will only emit the full path if /FC option is used in visual studio or /ZI (Which forces /FC)
#define REGISTERCLASS( T )	REGISTERBASE( T, false, false )

#define REGISTERSINGLETON( T, bIsAutoConstructSingleton )	REGISTERBASE( T, true, bIsAutoConstructSingleton )
*/
  }
}

#endif // OBJECTINTERFACEPERMODULE_INCLUDED
