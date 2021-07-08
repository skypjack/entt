#ifndef ENTT_API_IMPORT
#error "ENTT_API_IMPORT must be defined when using entt as a shared library"
#endif

#include "api.h"

#include <iterator>

extern "C" {
int get_id();
void increment(entt::registry &);
}

struct dummy_0 {};
struct dummy_1 {};

/**
 * If functions are compiled inlined, there might be 2 situations:
 * - symbols were not exported by the linker from shared registry library
 * - symbols were not imported to the plugin due to linker optimization
 *
 * @param registry
 * @return
 */
int get_id() {
  [[maybe_unused]] auto dummyId0 =
      entt::type_seq<dummy_0>();             // expected id == 1
  auto dummyId1 = entt::type_seq<dummy_1>(); // expected id == 2

  return static_cast<int>(dummyId1);
}

/**
 * Increment will fail if linker has optimized out imported symbols.
 * @param registry
 */
void increment(entt::registry &registry) {

  // if entt::internal::type_seq has been imported, ids will be 2 and 3
  // respectively.
  [[maybe_unused]] auto dummyId0 = entt::type_seq<dummy_0>();
  [[maybe_unused]] auto dummyId1 = entt::type_seq<dummy_1>();

  auto view = registry.view<test_increment>();
  if (std::distance(view.begin(), view.end()))
    ++(registry.get<test_increment>(view[0]).i);
}
