#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_info.hpp>
#include <entt/resource/cache.hpp>

struct resource {
    virtual ~resource() = default;

    virtual const entt::type_info &type() const ENTT_NOEXCEPT {
        return entt::type_id<resource>();
    }

    int value;
};

struct derived_resource: resource {
    const entt::type_info &type() const ENTT_NOEXCEPT override {
        return entt::type_id<derived_resource>();
    }
};

template<typename Resource>
struct loader: entt::resource_loader<loader<Resource>, Resource> {
    std::shared_ptr<Resource> load(int value) const {
        auto res = std::shared_ptr<Resource>(new Resource);
        res->value = value;
        return res;
    }
};

template<typename Resource>
struct broken_loader: entt::resource_loader<broken_loader<Resource>, Resource> {
    std::shared_ptr<Resource> load(int) const {
        return nullptr;
    }
};

template<typename Type, typename Other>
entt::resource_handle<Type> dynamic_resource_handle_cast(const entt::resource_handle<Other> &other) {
    if(other->type() == entt::type_id<Type>()) {
        return entt::resource_handle<Type>{other, static_cast<Type &>(other.get())};
    }

    return {};
}

TEST(Resource, Functionalities) {
    entt::resource_cache<resource> cache;

    constexpr auto hs1 = entt::hashed_string{"res1"};
    constexpr auto hs2 = entt::hashed_string{"res2"};

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_FALSE(cache.load<broken_loader<resource>>(hs1, 42));
    ASSERT_FALSE(cache.reload<broken_loader<resource>>(hs1, 42));

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_TRUE(cache.load<loader<resource>>(hs1, 42));
    ASSERT_TRUE(cache.reload<loader<resource>>(hs1, 42));

    ASSERT_EQ(cache.handle(hs1).use_count(), 2);

    auto tmp = cache.handle(hs1);

    ASSERT_EQ(std::as_const(cache).handle(hs1).use_count(), 3);
    ASSERT_TRUE(static_cast<bool>(tmp));

    tmp = {};

    ASSERT_EQ(cache.handle(hs1).use_count(), 2);
    ASSERT_FALSE(static_cast<bool>(tmp));

    ASSERT_NE(cache.size(), 0u);
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));
    ASSERT_EQ((*std::as_const(cache).handle(hs1)).value, 42);

    ASSERT_TRUE(cache.load<loader<resource>>(hs1, 42));
    ASSERT_TRUE(cache.load<loader<resource>>(hs2, 42));

    ASSERT_NE(cache.size(), 0u);
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.contains(hs1));
    ASSERT_TRUE(cache.contains(hs2));
    ASSERT_EQ((*cache.handle(hs1)).value, 42);
    ASSERT_EQ(std::as_const(cache).handle(hs2)->value, 42);

    ASSERT_NO_FATAL_FAILURE(cache.discard(hs1));

    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_TRUE(cache.contains(hs2));
    ASSERT_EQ(cache.handle(hs2)->value, 42);

    ASSERT_TRUE(cache.load<loader<resource>>(hs1, 42));
    ASSERT_NO_FATAL_FAILURE(cache.clear());

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());
    ASSERT_FALSE(cache.contains(hs1));
    ASSERT_FALSE(cache.contains(hs2));

    ASSERT_TRUE(cache.load<loader<resource>>(hs1, 42));

    ASSERT_NE(cache.size(), 0u);
    ASSERT_FALSE(cache.empty());
    ASSERT_TRUE(cache.handle(hs1));
    ASSERT_FALSE(cache.handle(hs2));
    ASSERT_TRUE(std::as_const(cache).handle(hs1));
    ASSERT_FALSE(std::as_const(cache).handle(hs2));

    ASSERT_TRUE(std::as_const(cache).handle(hs1));
    ASSERT_EQ(&cache.handle(hs1).get(), &static_cast<const resource &>(cache.handle(hs1)));
    ASSERT_NO_FATAL_FAILURE(cache.clear());

    ASSERT_EQ(cache.size(), 0u);
    ASSERT_TRUE(cache.empty());

    ASSERT_TRUE(cache.temp<loader<resource>>(42));
    ASSERT_TRUE(cache.empty());

    ASSERT_FALSE(entt::resource_handle<resource>{});
    ASSERT_TRUE(std::is_copy_constructible_v<entt::resource_handle<resource>>);
    ASSERT_TRUE(std::is_move_constructible_v<entt::resource_handle<resource>>);
    ASSERT_TRUE(std::is_copy_assignable_v<entt::resource_handle<resource>>);
    ASSERT_TRUE(std::is_move_assignable_v<entt::resource_handle<resource>>);
}

TEST(Resource, ConstNonConstHandle) {
    entt::resource_cache<resource> cache;

    entt::resource_handle<resource> handle = cache.temp<loader<resource>>(42);
    entt::resource_handle<const resource> chandle = handle;

    static_assert(std::is_same_v<decltype(handle.get()), resource &>);
    static_assert(std::is_same_v<decltype(chandle.get()), const resource &>);
    static_assert(std::is_same_v<decltype(std::as_const(handle).get()), resource &>);

    ASSERT_TRUE(chandle);
    ASSERT_EQ(handle.use_count(), 2u);
    ASSERT_EQ(chandle->value, 42);

    chandle = {};

    ASSERT_FALSE(chandle);
    ASSERT_EQ(handle.use_count(), 1u);
}

TEST(Resource, MutableHandle) {
    entt::resource_cache<resource> cache;

    constexpr auto hs = entt::hashed_string{"res"};
    auto handle = cache.load<loader<resource>>(hs, 0);

    ASSERT_TRUE(handle);

    ++handle.get().value;
    ++static_cast<resource &>(handle).value;
    ++(*handle).value;
    ++handle->value;

    ASSERT_EQ(cache.handle(hs)->value, 4);
}

TEST(Resource, HandleImplicitCast) {
    entt::resource_cache<resource> cache;
    auto handle = cache.temp<loader<derived_resource>>(0);

    auto resource = std::make_shared<derived_resource>();
    entt::resource_handle<derived_resource> other{resource};

    ASSERT_TRUE(handle);
    ASSERT_TRUE(other);
    ASSERT_NE(&*handle, &*other);
    ASSERT_EQ(resource.use_count(), 2u);

    auto temp = std::move(handle);
    handle = other;

    ASSERT_TRUE(handle);
    ASSERT_TRUE(other);
    ASSERT_TRUE(temp);
    ASSERT_EQ(&*handle, &*other);
    ASSERT_EQ(resource.use_count(), 3u);

    temp = std::move(other);

    ASSERT_TRUE(handle);
    ASSERT_FALSE(other);
    ASSERT_TRUE(temp);
    ASSERT_EQ(&*handle, &*temp);
    ASSERT_EQ(resource.use_count(), 3u);

    temp = handle = {};

    ASSERT_FALSE(handle);
    ASSERT_FALSE(other);
    ASSERT_FALSE(temp);
    ASSERT_EQ(resource.use_count(), 1u);
}

TEST(Resource, DynamicResourceHandleCast) {
    entt::resource_handle<derived_resource> handle = entt::resource_cache<derived_resource>{}.temp<loader<derived_resource>>(42);
    entt::resource_handle<const resource> base = handle;

    ASSERT_TRUE(base);
    ASSERT_EQ(handle.use_count(), 2u);
    ASSERT_EQ(base->value, 42);

    entt::resource_handle<const derived_resource> chandle = dynamic_resource_handle_cast<const derived_resource>(base);

    ASSERT_TRUE(chandle);
    ASSERT_EQ(handle.use_count(), 3u);
    ASSERT_EQ(chandle->value, 42);

    base = entt::resource_cache<resource>{}.temp<loader<resource>>(42);
    chandle = dynamic_resource_handle_cast<const derived_resource>(base);

    ASSERT_FALSE(chandle);
    ASSERT_EQ(handle.use_count(), 1u);
}

TEST(Resource, Each) {
    using namespace entt::literals;

    entt::resource_cache<resource> cache;
    cache.load<loader<resource>>("resource"_hs, 0);

    cache.each([](entt::resource_handle<resource> res) {
        ++res->value;
    });

    ASSERT_FALSE(cache.empty());
    ASSERT_EQ(cache.handle("resource"_hs)->value, 1);

    cache.each([](entt::id_type id, entt::resource_handle<resource> res) {
        ASSERT_EQ(id, "resource"_hs);
        ++res->value;
    });

    ASSERT_FALSE(cache.empty());

    std::as_const(cache).each([](entt::id_type id, entt::resource_handle<const resource> res) {
        ASSERT_EQ(res->value, 2);
    });

    cache.each([&cache](entt::id_type id) {
        cache.discard(id);
    });

    ASSERT_TRUE(cache.empty());
}
