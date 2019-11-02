#pragma once
#include "reload.h"

namespace reload
{
  class fake_object
  {
  public:
    virtual ~fake_object()
    {
    }
  };
  class cool_object : public fake_object
  {
  public:
    const char* alias = "cool_object";
    ~cool_object()
    {
    }
    size_t type_id;
    size_t type_id2;
  };
}

int constexpr length(const char* str)
{
  return *str ? 1 + length(str + 1) : 0;
}

int constexpr count(const char* str, const char ch, int length)
{
  if (length)
  {
    if (*str == ch)
    {
      return 1 + count(str + 1, ch, length - 1);
    }
    else
    {
      return 0 + count(str + 1, ch, length - 1);
    }
  }
  else
  {
    return 0;
  }
}

enable_hot_reload(reload::cool_object, "cool_object");
enable_hot_reload(reload::cool_object, "cool_object");

void test3()
{
  //std::cerr << "number of include dirs is: " << count(_INCLUDE_DIRS, '\n', length(_INCLUDE_DIRS)) << std::endl;
  //std::cerr << _INCLUDE_DIRS << std::endl;

  //std::cerr << "number of link libs is: " << count(_LINK_LIBS, '\n', length(_LINK_LIBS)) << std::endl;
  //std::cerr << _LINK_LIBS << std::endl;

  auto factory = reload::runtime::generic_factory();
  auto list = factory.constructor_list;
  auto index = list.find(list.end, [&](reload::runtime::generic_constructor other)
  {
    return other.name == "cool_object";
  });
  if (index == list.end)
  {
    throw std::exception("constructor not found");
  }
  reload::runtime::generic_constructor* ptr0 = (reload::runtime::generic_constructor*) list[index].construct();

  auto p0 = dynamic_cast<reload::cool_object*>(ptr0);

  auto size = list.size();
}