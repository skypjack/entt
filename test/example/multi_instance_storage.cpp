#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/entity/pool.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/sparse_set.hpp>
#include <entt/entity/storage.hpp>

/**
 * Yeah, I could have used a single memory chunk and a free list and associated
 * entities with a pointer to their first element, but this is just an example
 * of how to create a custom storage class.
 * You can have fun regarding the actual implementation.
 */
template<typename Entity, typename Type>
struct multi_instance_storage: entt::basic_storage<Entity, std::vector<Type>> {
    using underlying_storage = entt::basic_storage<Entity, std::vector<Type>>;

    using value_type = typename underlying_storage::value_type;
    using entity_type = typename underlying_storage::entity_type;
    using size_type = typename underlying_storage::size_type;
    using iterator = typename underlying_storage::iterator;
    using const_iterator = typename underlying_storage::const_iterator;

    template<typename... Args>
    void insert(Args &&...) = delete;

    using underlying_storage::erase;

    template<typename... Args>
    Type & emplace(const entity_type entt, Args &&... args) {
        std::vector<Type> *vec = underlying_storage::try_get(entt);

        if(!vec) {
            vec = &underlying_storage::emplace(entt);
        }

        return vec->emplace_back(Type{std::forward<Args>(args)...});
    }

    void erase(const entity_type entt, const size_type index) {
        auto &vec = underlying_storage::get(entt);
        vec.erase(vec.begin() + index);

        if(vec.empty()) {
            underlying_storage::erase(entt);
        }
    }
};

struct single_instance_type { int value; };
struct multi_instance_type { int value; };

template<typename Entity>
struct entt::pool<Entity, multi_instance_type> {
    using type = storage_adapter_mixin<multi_instance_storage<Entity, multi_instance_type>>;
};

TEST(Example, MultiInstanceStorage) {
    entt::registry registry;
    const auto entity = registry.create();

    ASSERT_FALSE(registry.has<multi_instance_type>(entity));

    registry.emplace<multi_instance_type>(entity, 0);

    ASSERT_TRUE(registry.has<multi_instance_type>(entity));
    ASSERT_EQ(registry.get<multi_instance_type>(entity).size(), 1u);
    
    registry.remove<multi_instance_type>(entity, 0u);
    
    ASSERT_FALSE(registry.has<multi_instance_type>(entity));
    
    registry.emplace<multi_instance_type>(entity, 42);
    registry.emplace<multi_instance_type>(entity, 3);
    registry.emplace<multi_instance_type>(entity, 0);
    
    ASSERT_EQ(registry.get<multi_instance_type>(entity).size(), 3u);
    ASSERT_EQ(registry.get<multi_instance_type>(entity)[0].value, 42);
    ASSERT_EQ(registry.get<multi_instance_type>(entity)[1].value, 3);
    ASSERT_EQ(registry.get<multi_instance_type>(entity)[2].value, 0);
    
    registry.remove<multi_instance_type>(entity, 1u);

    ASSERT_TRUE(registry.has<multi_instance_type>(entity));
    ASSERT_EQ(registry.get<multi_instance_type>(entity).size(), 2u);
    ASSERT_EQ(registry.get<multi_instance_type>(entity)[0].value, 42);
    ASSERT_EQ(registry.get<multi_instance_type>(entity)[1].value, 0);
    
    registry.remove<multi_instance_type>(entity);
    
    ASSERT_FALSE(registry.has<multi_instance_type>(entity));
}
