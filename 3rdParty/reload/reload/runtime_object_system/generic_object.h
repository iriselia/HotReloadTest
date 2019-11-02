#pragma once

#include "const_counter.h"
#include "generic_factory.h"

#include <string>

namespace reload
{
  struct generic_object
  {
  };

  namespace runtime
  {
    struct generic_constructor
    {
      std::string name;
      std::size_t id;

      generic_constructor()
      {
      }

      generic_constructor(const char* name, std::size_t id)
        : name(name),
        id(id)
      {
      }

      virtual ~generic_constructor()
      {
      }

      virtual void* construct() const
      {
        return nullptr;
      }
    };

    template<typename T>
    struct constructor : public generic_constructor
    {
      constructor()
        : generic_constructor(typeid(T).name(), COUNTER_READ(generic_factory::constructor_index))
      {
      }

      constructor(const char* name)
        : generic_constructor(name, COUNTER_READ(generic_factory::constructor_index))
      {
      }


      virtual void* construct() const
      {
        return new T();
      }
    };
  }
}