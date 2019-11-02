#pragma once
#include <utility>

namespace reload
{
  namespace runtime
  {
    template<std::size_t value>
    struct constant : std::integral_constant<std::size_t, value>
    {
    };

    // ADL fallback
    template<typename counter_type, std::size_t rank, std::size_t accumulation>
    constexpr auto counter_func(counter_type, std::integral_constant<std::size_t, rank>,std::integral_constant<std::size_t, accumulation>)
      -> std::integral_constant<std::size_t, accumulation>
    {
      return {};
    }
  }
}


#define COUNTER_READ(counter_type) \
std::size_t( \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 0>(), \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 1>(), \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 2>(), \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 3>(), \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 4>(), \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 5>(), \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 6>(), \
::reload::runtime::counter_func(counter_type(), std::integral_constant<std::size_t, 1 << 7>(), std::integral_constant<std::size_t, 0>())))))))))

#define COUNTER_INC(counter_type) \
namespace reload \
{ \
  namespace runtime \
  { \
    constexpr auto counter_func( \
      counter_type, \
      std::integral_constant<std::size_t, (COUNTER_READ(counter_type) + 1) & ~COUNTER_READ(counter_type)>, \
      std::integral_constant<std::size_t, (COUNTER_READ(counter_type) + 1) &  COUNTER_READ(counter_type)>) \
      -> std::integral_constant<std::size_t, COUNTER_READ(counter_type) + 1> { return {}; } \
  } \
}

#define static_list_new(counter_type) COUNTER_INC(counter_type)


#define COUNTER_LINK_NAMESPACE( NS ) using NS::counter_func;

// File scope counter
/*
template<unsigned int NUM> struct Counter { enum { value = Counter<NUM-1>::value }; };
template<> struct Counter<0> { enum { value = 0 }; };

#define counter_read Counter<__LINE__>::value
#define counter_inc template<> struct Counter<__LINE__> { enum { value = Counter<__LINE__-1>::value + 1}; }
*/