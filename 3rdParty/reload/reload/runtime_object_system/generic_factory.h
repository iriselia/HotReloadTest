#pragma once

#include "static_list.h"

namespace reload
{
  namespace runtime
  {
    struct generic_constructor;

    class generic_factory
    {
    public:
      template<typename T>
      T* construct(char* name)
      {
        return new T();
      }

      template<typename T>
      std::shared_ptr<T> construct_shared(char* name)
      {
        return std::make_shared<T>();
      }

      template<typename T>
      std::unique_ptr<T> construct_unique(char* name)
      {
        return std::make_unique<T>();
      }

      struct constructor_index
      {
      };
      reload::util::static_list<generic_constructor, generic_factory::constructor_index> constructor_list;

    private:
    };
  }
}