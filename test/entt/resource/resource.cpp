#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/resource/resource.hpp>

struct base {
    virtual ~base() = default;

    virtual const entt::type_info &type() const noexcept {
        return entt::type_id<base>();
    }
};

struct derived: base {
    const entt::type_info &type() const noexcept override {
        return entt::type_id<derived>();
    }
};

template<typename Type, typename Other>
entt::resource<Type> dynamic_resource_cast(const entt::resource<Other> &other) {
    if(other->type() == entt::type_id<Type>()) {
        return entt::resource<Type>{other, static_cast<Type &>(*other)};
    }

    return {};
}

TEST(Resource, Functionalities) {
    entt::resource<derived> resource{};

    ASSERT_FALSE(resource);
    ASSERT_EQ(resource.operator->(), nullptr);
    ASSERT_EQ(resource.handle().use_count(), 0l);

    const auto value = std::make_shared<derived>();
    entt::resource<derived> other{value};

    ASSERT_TRUE(other);
    ASSERT_EQ(other.operator->(), value.get());
    ASSERT_EQ(&static_cast<derived &>(other), value.get());
    ASSERT_EQ(&*other, value.get());
    ASSERT_EQ(other.handle().use_count(), 2l);

    entt::resource<derived> copy{resource};
    entt::resource<derived> move{std::move(other)};

    ASSERT_FALSE(copy);
    ASSERT_TRUE(move);

    copy = std::move(move);
    move = copy;

    ASSERT_TRUE(copy);
    ASSERT_TRUE(move);
    ASSERT_EQ(copy, move);
}

TEST(Resource, DerivedToBase) {
    entt::resource<derived> resource{std::make_shared<derived>()};
    entt::resource<base> other{resource};
    entt::resource<const base> cother{resource};

    ASSERT_TRUE(resource);
    ASSERT_TRUE(other);
    ASSERT_TRUE(cother);
    ASSERT_EQ(resource, other);
    ASSERT_EQ(other, cother);

    other = resource;
    cother = resource;

    ASSERT_EQ(resource, other);
    ASSERT_EQ(other, cother);
}

TEST(Resource, ConstNonConstAndAllInBetween) {
    entt::resource<derived> resource{std::make_shared<derived>()};
    entt::resource<derived> other{resource};

    static_assert(std::is_same_v<decltype(*resource), derived &>);
    static_assert(std::is_same_v<decltype(*entt::resource<const derived>{other}), const derived &>);
    static_assert(std::is_same_v<decltype(*std::as_const(resource)), derived &>);

    entt::resource<const derived> copy{resource};
    entt::resource<const derived> move{std::move(other)};

    ASSERT_TRUE(resource);
    ASSERT_FALSE(other);

    ASSERT_TRUE(copy);
    ASSERT_EQ(copy, resource);
    ASSERT_NE(copy, entt::resource<derived>{});
    ASSERT_EQ(copy.handle().use_count(), 3u);

    ASSERT_TRUE(move);
    ASSERT_EQ(move, resource);
    ASSERT_NE(move, entt::resource<derived>{});
    ASSERT_EQ(move.handle().use_count(), 3u);

    copy = resource;
    move = std::move(resource);

    ASSERT_FALSE(resource);
    ASSERT_FALSE(other);

    ASSERT_TRUE(copy);
    ASSERT_TRUE(move);
    ASSERT_EQ(copy.handle().use_count(), 2u);
}

TEST(Resource, DynamicResourceHandleCast) {
    entt::resource<derived> resource{std::make_shared<derived>()};
    entt::resource<const base> other = resource;

    ASSERT_TRUE(other);
    ASSERT_EQ(resource.handle().use_count(), 2u);
    ASSERT_EQ(resource, other);

    entt::resource<const derived> cast = dynamic_resource_cast<const derived>(other);

    ASSERT_TRUE(cast);
    ASSERT_EQ(resource.handle().use_count(), 3u);
    ASSERT_EQ(resource, cast);

    other = entt::resource<base>{std::make_shared<base>()};
    cast = dynamic_resource_cast<const derived>(other);

    ASSERT_FALSE(cast);
    ASSERT_EQ(resource.handle().use_count(), 1u);
}

TEST(Resource, Comparison) {
    entt::resource<derived> resource{std::make_shared<derived>()};
    entt::resource<const base> other = resource;

    ASSERT_TRUE(resource == other);
    ASSERT_FALSE(resource != other);

    ASSERT_FALSE(resource < other);
    ASSERT_FALSE(resource > other);

    ASSERT_TRUE(resource <= other);
    ASSERT_TRUE(resource >= other);
}
