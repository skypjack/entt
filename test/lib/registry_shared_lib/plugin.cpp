
#include "api.h"

#include <iterator>

extern "C" {
int get_id(entt::registry &);
}

/**
 * If functions are compiled inlined, there might be 2 situations:
 * - symbols were not exported by the linker from shared registry library
 * - symbols were not imported to the plugin due to linker optimization
 *
 * The context of the test implies that another type has been registered prior
 * to an exported, hence if an imported symbol is used, the id will be equal to
 * 1. Otherwise it will be equal to 0, and there will be two identical
 * identifiers stored in duplicated symbols.
 *
 * In case of linker optimization on importing, the entity will not be found
 * within the registry.
 *
 * @param registry
 * @return
 */
int get_id(entt::registry &registry) {

  const auto id = entt::type_seq<test_increment>();
  auto view = registry.view<test_increment>();
  if (std::distance(view.begin(), view.end()))
    ++(registry.get<test_increment>(view[0]).i);
  return static_cast<int>(id);
}
