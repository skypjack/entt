#include <cstddef>
#include <gtest/gtest.h>
#include <entt/core/utility.hpp>
#include <entt/entity/poly_storage.hpp>
#include <entt/entity/registry.hpp>

template<typename... Type>
entt::type_list<Type...> as_type_list(const entt::type_list<Type...> &);

template<typename Entity>
struct PolyStorage: entt::type_list_cat_t<
    decltype(as_type_list(std::declval<entt::Storage<Entity>>())),
    entt::type_list<
        void(const Entity *, const Entity *, void *),
        void(entt::basic_registry<Entity> &, const Entity, const void *),
        const void *(const Entity) const,
        void(entt::basic_registry<Entity> &) const
    >
> {
    using entity_type = Entity;
    using size_type = std::size_t;

    template<typename Base>
    struct type: entt::Storage<Entity>::template type<Base> {
        static constexpr auto base = decltype(as_type_list(std::declval<entt::Storage<Entity>>()))::size;

        void remove(entt::basic_registry<Entity> &owner, const entity_type *first, const entity_type *last) {
            entt::poly_call<base + 0>(*this, first, last, &owner);
        }

        void emplace(entt::basic_registry<Entity> &owner, const entity_type entity, const void *instance) {
            entt::poly_call<base + 1>(*this, owner, entity, instance);
        }

        const void * get(const entity_type entity) const {
            return entt::poly_call<base + 2>(*this, entity);
        }

        void copy_to(entt::basic_registry<Entity> &other) const {
            entt::poly_call<base + 3>(*this, other);
        }
    };

    template<typename Type>
    struct members {
        static void emplace(Type &self, entt::basic_registry<Entity> &owner, const entity_type entity, const void *instance) {
            self.emplace(owner, entity, *static_cast<const typename Type::value_type *>(instance));
        }

        static const typename Type::value_type * get(const Type &self, const entity_type entity) {
            return &self.get(entity);
        }

        static void copy_to(const Type &self, entt::basic_registry<entity_type> &other) {
            other.template insert<typename Type::value_type>(self.data(), self.data() + self.size(), self.raw(), self.raw() + self.size());
        }
    };

    template<typename Type>
    using impl = entt::value_list_cat_t<
        typename entt::Storage<Entity>::template impl<Type>,
        entt::value_list<
            &Type::template remove<const entity_type *>,
            &members<Type>::emplace,
            &members<Type>::get,
            &members<Type>::copy_to
        >
    >;
};

template<typename Entity>
struct entt::poly_storage_traits<Entity> {
    using storage_type = entt::poly<PolyStorage<Entity>>;
};

TEST(PolyStorage, CopyEntity) {
    entt::registry registry;
    const auto entity = registry.create();
    const auto other = registry.create();

    registry.emplace<int>(entity, 42);
    registry.emplace<char>(entity, 'c');

    ASSERT_TRUE((registry.all_of<int, char>(entity)));
    ASSERT_FALSE((registry.any_of<int, char>(other)));

    registry.visit(entity, [&](const auto info) {
        auto &&storage = registry.storage(info);
        storage->emplace(registry, other, storage->get(entity));
    });

    ASSERT_TRUE((registry.all_of<int, char>(entity)));
    ASSERT_TRUE((registry.all_of<int, char>(other)));

    ASSERT_EQ(registry.get<int>(entity), registry.get<int>(other));
    ASSERT_EQ(registry.get<char>(entity), registry.get<char>(other));
}

TEST(PolyStorage, CopyRegistry) {
    entt::registry registry;
    entt::registry other;
    entt::entity entities[10u];

    registry.create(std::begin(entities), std::end(entities));
    registry.insert<int>(std::begin(entities), std::end(entities), 42);
    registry.insert<char>(std::begin(entities), std::end(entities), 'c');

    ASSERT_EQ(registry.size(), 10u);
    ASSERT_EQ(other.size(), 0u);

    other.assign(registry.data(), registry.data() + registry.size(), registry.destroyed());
    registry.visit([&](const auto info) { std::as_const(registry).storage(info)->copy_to(other); });

    ASSERT_EQ(registry.size(), other.size());
    ASSERT_EQ((registry.view<int, char>().size_hint()), (other.view<int, char>().size_hint()));
    ASSERT_NE((other.view<int, char>().size_hint()), 0u);

    for(const auto entity: registry.view<int, char>()) {
        ASSERT_EQ((registry.get<int, char>(entity)), (other.get<int, char>(entity)));
    }
}

TEST(PolyStorage, Constness) {
    entt::registry registry;
    const auto &cregistry = registry;

    entt::entity entity[1];
    entity[0] = registry.create();
    registry.emplace<int>(entity[0], 42);

    // cannot invoke remove on a const storage, let's copy the returned value
    auto cstorage = cregistry.storage(entt::type_id<int>());

    ASSERT_DEATH(cstorage->remove(registry, std::begin(entity), std::end(entity)), "");
    ASSERT_TRUE(registry.all_of<int>(entity[0]));

    auto &&storage = registry.storage(entt::type_id<int>());
    storage->remove(registry, std::begin(entity), std::end(entity));

    ASSERT_FALSE(registry.all_of<int>(entity[0]));
}
