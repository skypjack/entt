#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/policy.hpp>
#include <entt/meta/resolve.hpp>

struct EntityCopy: testing::Test {
    enum class my_entity : entt::id_type {};
    enum class other_entity : entt::id_type {};

    template<typename Type>
    // NOLINTNEXTLINE(*-exception-escape)
    struct meta_mixin: Type {
        using allocator_type = Type::allocator_type;
        using element_type = Type::element_type;

        explicit meta_mixin(const allocator_type &allocator);
    };

    void TearDown() override {
        entt::meta_reset();
    }
};

template<typename Type>
struct entt::storage_type<Type, EntityCopy::my_entity> {
    using type = EntityCopy::meta_mixin<basic_storage<Type, EntityCopy::my_entity>>;
};

template<typename Type>
struct entt::storage_type<Type, EntityCopy::other_entity> {
    using type = EntityCopy::meta_mixin<basic_storage<Type, EntityCopy::other_entity>>;
};

template<typename Type>
EntityCopy::meta_mixin<Type>::meta_mixin(const allocator_type &allocator)
    : Type{allocator} {
    using namespace entt::literals;

    entt::meta_factory<element_type>{}
        // cross registry, same type
        .template func<entt::overload<entt::storage_for_t<element_type, my_entity> &(const entt::id_type)>(&entt::basic_registry<my_entity>::storage<element_type>), entt::as_ref_t>("storage"_hs)
        // cross registry, different types
        .template func<entt::overload<entt::storage_for_t<element_type, other_entity> &(const entt::id_type)>(&entt::basic_registry<other_entity>::storage<element_type>), entt::as_ref_t>("storage"_hs);
}

TEST_F(EntityCopy, SameRegistry) {
    using namespace entt::literals;

    entt::basic_registry<my_entity> registry{};
    auto &&custom = registry.storage<double>("custom"_hs);

    const auto src = registry.create();
    const auto dst = registry.create();

    custom.emplace(src, 1.);
    registry.emplace<int>(src, 2);
    registry.emplace<char>(src, 'c');

    ASSERT_EQ(registry.storage<my_entity>().size(), 2u);
    ASSERT_TRUE(custom.contains(src));
    ASSERT_FALSE(custom.contains(dst));
    ASSERT_TRUE((registry.all_of<int, char>(src)));
    ASSERT_FALSE((registry.any_of<int, char>(dst)));

    for(auto [id, storage]: registry.storage()) {
        // discard the custom storage because why not, this is just an example after all
        if(id != "custom"_hs && storage.contains(src)) {
            storage.push(dst, storage.value(src));
        }
    }

    ASSERT_EQ(registry.storage<my_entity>().size(), 2u);
    ASSERT_TRUE(custom.contains(src));
    ASSERT_FALSE(custom.contains(dst));
    ASSERT_TRUE((registry.all_of<int, char>(src)));
    ASSERT_TRUE((registry.all_of<int, char>(dst)));

    ASSERT_EQ(registry.get<int>(dst), 2);
    ASSERT_EQ(registry.get<char>(dst), 'c');
}

TEST_F(EntityCopy, CrossRegistry) {
    using namespace entt::literals;

    entt::basic_registry<my_entity> src{};
    entt::basic_registry<other_entity> dst{};

    const auto entity = src.create();
    const auto copy = dst.create();

    src.emplace<int>(entity, 2);
    src.emplace<char>(entity, 'c');

    ASSERT_EQ(src.storage<my_entity>().size(), 1u);
    ASSERT_EQ(dst.storage<other_entity>().size(), 1u);

    ASSERT_TRUE((src.all_of<int, char>(entity)));
    ASSERT_FALSE((dst.template all_of<int, char>(copy)));

    for(auto [id, storage]: src.storage()) {
        if(storage.contains(entity)) {
            auto *other = dst.storage(id);

            if(other == nullptr) {
                using namespace entt::literals;
                entt::resolve(storage.info()).invoke("storage"_hs, {}, entt::forward_as_meta(dst), id);
                other = dst.storage(id);
            }

            other->push(copy, storage.value(entity));
        }
    }

    ASSERT_EQ(src.storage<my_entity>().size(), 1u);
    ASSERT_EQ(dst.storage<other_entity>().size(), 1u);

    ASSERT_TRUE((src.all_of<int, char>(entity)));
    ASSERT_TRUE((dst.template all_of<int, char>(copy)));
    ASSERT_EQ(dst.template get<int>(copy), 2);
    ASSERT_EQ(dst.template get<char>(copy), 'c');
}
