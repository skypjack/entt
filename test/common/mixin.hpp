#ifndef ENTT_COMMON_MIXIN_HPP
#define ENTT_COMMON_MIXIN_HPP

#include <entt/core/any.hpp>
#include <entt/entity/fwd.hpp>

namespace test {

struct assure_loop {};

template<typename Type>
class assure_loop_mixin: public Type {
    using underlying_type = Type;
    using registry_type = entt::basic_registry<typename underlying_type::entity_type, typename underlying_type::base_type::allocator_type>;

    void bind_any(entt::any value) noexcept override {
        if(auto *owner = entt::any_cast<registry_type>(&value); owner) {
            owner->template storage<int>();
        }
    }

public:
    using allocator_type = typename underlying_type::allocator_type;
    using entity_type = typename underlying_type::entity_type;

    using Type::Type;
};

} // namespace test

template<typename Entity>
struct entt::storage_type<test::assure_loop, Entity> {
    using type = test::assure_loop_mixin<entt::basic_storage<test::assure_loop, Entity>>;
};

#endif
