#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/storage.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/policy.hpp>
#include <entt/meta/resolve.hpp>

enum class my_entity : entt::id_type {};

template<typename Type>
// NOLINTNEXTLINE(*-exception-escape)
struct meta_mixin: Type {
    using allocator_type = typename Type::allocator_type;
    using element_type = typename Type::element_type;

    explicit meta_mixin(const allocator_type &allocator);
};

template<typename Type, typename Entity>
struct entt::storage_type<Type, Entity> {
    using type = meta_mixin<basic_storage<Type, Entity>>;
};

template<typename Type>
meta_mixin<Type>::meta_mixin(const allocator_type &allocator)
    : Type{allocator} {
    using namespace entt::literals;

    entt::meta_factory<element_type>{}
        // cross registry, same type
        .template func<entt::overload<entt::storage_for_t<element_type, entt::entity> &(const entt::id_type)>(&entt::basic_registry<entt::entity>::storage<element_type>), entt::as_ref_t>("storage"_hs)
        // cross registry, different types
        .template func<entt::overload<entt::storage_for_t<element_type, my_entity> &(const entt::id_type)>(&entt::basic_registry<my_entity>::storage<element_type>), entt::as_ref_t>("storage"_hs);
}

template<typename Type>
struct EntityCopy: testing::Test {
    using type = Type;
};

using EntityCopyTypes = ::testing::Types<entt::basic_registry<entt::entity>, entt::basic_registry<my_entity>>;

TYPED_TEST_SUITE(EntityCopy, EntityCopyTypes, );

TEST(EntityCopy, SameRegistry) {
    using namespace entt::literals;

    entt::registry registry{};
    auto &&custom = registry.storage<double>("custom"_hs);

    const auto src = registry.create();
    const auto dst = registry.create();

    custom.emplace(src, 1.);
    registry.emplace<int>(src, 2);
    registry.emplace<char>(src, 'c');

    ASSERT_EQ(registry.storage<entt::entity>().size(), 2u);
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

    ASSERT_EQ(registry.storage<entt::entity>().size(), 2u);
    ASSERT_TRUE(custom.contains(src));
    ASSERT_FALSE(custom.contains(dst));
    ASSERT_TRUE((registry.all_of<int, char>(src)));
    ASSERT_TRUE((registry.all_of<int, char>(dst)));

    ASSERT_EQ(registry.get<int>(dst), 2);
    ASSERT_EQ(registry.get<char>(dst), 'c');
}

TYPED_TEST(EntityCopy, CrossRegistry) {
    using namespace entt::literals;

    entt::basic_registry<entt::entity> src{};
    // other registry type, see typed test suite
    typename TestFixture::type dst{};

    const auto entity = src.create();
    const auto copy = dst.create();

    src.emplace<int>(entity, 2);
    src.emplace<char>(entity, 'c');

    ASSERT_EQ(src.storage<entt::entity>().size(), 1u);
    ASSERT_EQ(dst.template storage<typename TestFixture::type::entity_type>().size(), 1u);

    ASSERT_TRUE((src.all_of<int, char>(entity)));
    ASSERT_FALSE((dst.template all_of<int, char>(copy)));

    for(auto [id, storage]: src.storage()) {
        if(storage.contains(entity)) {
            auto *other = dst.storage(id);

            if(!other) {
                using namespace entt::literals;
                entt::resolve(storage.type()).invoke("storage"_hs, {}, entt::forward_as_meta(dst), id);
                other = dst.storage(id);
            }

            other->push(copy, storage.value(entity));
        }
    }

    ASSERT_EQ(src.storage<entt::entity>().size(), 1u);
    ASSERT_EQ(dst.template storage<typename TestFixture::type::entity_type>().size(), 1u);

    ASSERT_TRUE((src.all_of<int, char>(entity)));
    ASSERT_TRUE((dst.template all_of<int, char>(copy)));
    ASSERT_EQ(dst.template get<int>(copy), 2);
    ASSERT_EQ(dst.template get<char>(copy), 'c');
}
