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
        void(entt::basic_registry<Entity> &, const Entity, const void *),
        const void *(const Entity) const,
        const Entity *() const,
        const void *() const,
        std::size_t() const,
        void(entt::basic_registry<Entity> &, const Entity *, const void *, const std::size_t)
    >
> {
    using entity_type = Entity;
    using size_type = std::size_t;

    template<typename Base>
    struct type: entt::Storage<Entity>::template type<Base> {
        static constexpr auto base = std::tuple_size_v<typename entt::poly_vtable<entt::Storage<Entity>>::type>;

        void emplace(entt::basic_registry<entity_type> &owner, const entity_type entity, const void *instance) {
            entt::poly_call<base + 0>(*this, owner, entity, instance);
        }

        const void * get(const entity_type entity) const {
            return entt::poly_call<base + 1>(*this, entity);
        }

        const entity_type * data() const {
            return entt::poly_call<base + 2>(*this);
        }

        const void * raw() const {
            return entt::poly_call<base + 3>(*this);
        }

        size_type size() const {
            return entt::poly_call<base + 4>(*this);
        }

        void insert(entt::basic_registry<Entity> &owner, const Entity *entity, const void *instance, const std::size_t length) {
            entt::poly_call<base + 5>(*this, owner, entity, instance, length);
        }
    };

    template<typename Type>
    struct members {
        static void emplace(Type &self, entt::basic_registry<entity_type> &owner, const entity_type entity, const void *instance) {
            self.emplace(owner, entity, *static_cast<const typename Type::value_type *>(instance));
        }

        static const typename Type::value_type * get(const Type &self, const entity_type entity) {
            return &self.get(entity);
        }

        static void insert(Type &self, entt::basic_registry<entity_type> &owner, const entity_type *entity, const void *instance, const size_type length) {
            const auto *value = static_cast<const typename Type::value_type *>(instance);
            self.insert(owner, entity, entity + length, value, value + length);
        }
    };

    template<typename Type>
    using impl = entt::value_list_cat_t<
        typename entt::Storage<Entity>::template impl<Type>,
        entt::value_list<
            &members<Type>::emplace,
            &members<Type>::get,
            &Type::data,
            entt::overload<const typename Type::value_type *() const ENTT_NOEXCEPT>(&Type::raw),
            &Type::size,
            &members<Type>::insert
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

    ASSERT_TRUE((registry.has<int, char>(entity)));
    ASSERT_FALSE((registry.any<int, char>(other)));

    registry.visit(entity, [&](const auto info) {
        auto storage = registry.storage(info);
        storage->emplace(registry, other, storage->get(entity));
    });

    ASSERT_TRUE((registry.has<int, char>(entity)));
    ASSERT_TRUE((registry.has<int, char>(other)));

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

    other.prepare<int>();
    other.prepare<char>();

    ASSERT_EQ(registry.size(), 10u);
    ASSERT_EQ(other.size(), 0u);

    other.assign(registry.data(), registry.data() + registry.size(), registry.destroyed());

    registry.visit([&](const auto info) {
        auto storage = registry.storage(info);
        other.storage(info)->insert(other, storage->data(), storage->raw(), storage->size());
    });

    ASSERT_EQ(registry.size(), other.size());
}

TEST(PolyStorage, Constness) {
    entt::registry registry;
    const auto &cregistry = registry;

    entt::entity entity[1];
    entity[0] = registry.create();
    registry.emplace<int>(entity[0], 42);

    auto cstorage = cregistry.storage(entt::type_id<int>());

    ASSERT_DEATH(cstorage->remove(registry, std::begin(entity), std::end(entity)), ".*");
    ASSERT_TRUE(registry.has<int>(entity[0]));

    auto storage = registry.storage(entt::type_id<int>());
    storage->remove(registry, std::begin(entity), std::end(entity));

    ASSERT_FALSE(registry.has<int>(entity[0]));
}
