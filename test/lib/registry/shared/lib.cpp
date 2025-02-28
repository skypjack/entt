#include <entt/core/attribute.h>
#include <entt/entity/mixin.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/view.hpp>
#include "../../../common/boxed_type.h"
#include "../../../common/empty.h"

template class entt::basic_registry<entt::entity>;

ENTT_API void update(entt::registry &registry, int value) {
    registry.view<test::boxed_int, test::empty>().each([value](auto &elem) {
        elem.value += value;
    });
}

ENTT_API void insert(entt::registry &registry) {
    // forces the creation of the pool for the empty type
    static_cast<void>(registry.storage<test::empty>());

    const auto view = registry.view<test::boxed_int>();
    registry.insert<test::empty>(view.begin(), view.end());
}
