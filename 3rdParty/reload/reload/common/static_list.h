#pragma once

#include<memory>
#include "const_counter.h"

namespace reload
{
  namespace util
  {
    template<typename T, typename Counter, std::size_t Index = COUNTER_READ(Counter) - 1>
    struct static_list : public static_list<T, Counter, Index - 1>
    {
      static const std::size_t end = std::numeric_limits<std::size_t>().max();
      /*
      static_list(std::size_t size = Index)
      : static_list<T, Counter, Index - 1>()
      {
      }
      */

      template<typename Predicate>
      const std::size_t find(std::size_t end, Predicate predicate)
      {
        if (!!predicate(data))
        {
          return Index;
        }

        auto size = std::min(COUNTER_READ(Counter), end);
        auto index = Index;
        if (Index && Index < size)
        {
          return static_list<T, Counter, Index - 1>().find(end, predicate);
        }

        return end;
      }
      
      std::size_t size()
      {
        return COUNTER_READ(Counter);
      }

      virtual const T& operator[](std::size_t index) const
      {
        if (index > COUNTER_READ(Counter))
        {
          throw("static_list: operator[] index out of bound.");
        }
        else if (index == Index)
        {
          return data;
        }
        else if (index < Index)
        {
          return static_list<T, Counter, Index - 1>()[index];
        }
        else if (index > Index)
        {
          throw("static_list: unexpected access order.");
        }

        throw("static_list: operator[] index out of bound.");
      }

      static T data;
    };

    template<typename T, typename Counter>
    struct static_list<T, Counter, std::numeric_limits<std::size_t>().max()>
    {
      static const std::size_t end = std::numeric_limits<std::size_t>().max();
      using root_node = static_list<T, Counter, 0>;

      constexpr auto next()
      {
        return static_list<T, Counter, 0>();
      }

      std::size_t size()
      {
        return COUNTER_READ(Counter);
      }

      template<typename Predicate>
      const std::size_t find(std::size_t end, Predicate predicate)
      {
        return static_list<T, Counter, COUNTER_READ(Counter) - 1>().find(end, predicate);

      }

      virtual const T& operator[](std::size_t index) const
      {
        return static_list<T, Counter, COUNTER_READ(Counter) - 1>()[index];
      }
    };
  }
}