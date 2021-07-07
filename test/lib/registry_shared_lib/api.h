#pragma once

/*
 * public type to be exported and
 * visible to the users of shared registry lib
 */
struct test_increment {
  int i;
};

#include <entt/entt.hpp>

#if !_MSC_VER // MSVC does not allow extern with dllexport classes
extern
#endif
template struct ENTT_API entt::type_seq<test_increment>;