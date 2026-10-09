#include "../../common/registry_context.h"

namespace test {

template<typename Type>
registry_context_value<Type>::registry_context_value() = default;

template<typename Type>
registry_context_value<Type> *registry_context_value<Type>::insert(entt::registry &registry) {
    auto &ctx = registry.ctx();
    ctx.emplace<registry_context_value<Type> *>(this);
    ctx.emplace_as<registry_context_value<Type> *>(0u, this);
    return this;
}

template struct registry_context_value<std::uint32_t>;

registry_context_type *registry_context_insert(entt::registry &registry) {
    static registry_context_type value{};
    return value.insert(registry);
}

} // namespace test
