#include "../../common/registry_context.h"

namespace test {

template<typename Type>
registry_context_value<Type>::registry_context_value() = default;

template<typename Type>
void registry_context_value<Type>::touch() {}

template<typename Type>
registry_context_value<Type> *registry_context_value<Type>::insert(entt::registry &registry) {
    registry.ctx().emplace<registry_context_value<Type> *>(this);
    return this;
}

template struct registry_context_value<std::uint32_t>;

registry_context_type *registry_context_insert(entt::registry &registry) {
    static registry_context_type value{};
    return value.insert(registry);
}

} // namespace test
