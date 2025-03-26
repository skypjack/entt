#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/resource/resource.hpp>
#include "../../common/linter.hpp"

struct base {
    virtual ~base() = default;

    [[nodiscard]] virtual const entt::type_info &info() const noexcept {
        return entt::type_id<base>();
    }
};

struct derived: base {
    [[nodiscard]] const entt::type_info &info() const noexcept override {
        return entt::type_id<derived>();
    }
};

template<typename Type, typename Other>
entt::resource<Type> dynamic_resource_cast(const entt::resource<Other> &other) {
    if(other->info() == entt::type_id<Type>()) {
        return entt::resource<Type>{other, static_cast<Type &>(*other)};
    }

    return {};
}

TEST(Resource, Functionalities) {
    const entt::resource<derived> resource{};

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

    copy.reset(std::make_shared<derived>());

    ASSERT_TRUE(copy);
    ASSERT_TRUE(move);
    ASSERT_NE(copy, move);

    move.reset();

    ASSERT_TRUE(copy);
    ASSERT_FALSE(move);
    ASSERT_NE(copy, move);
}

TEST(Resource, Swap) {
    entt::resource<int> resource{};
    entt::resource<int> other{};

    ASSERT_FALSE(resource);
    ASSERT_FALSE(other);

    resource.swap(other);

    ASSERT_FALSE(resource);
    ASSERT_FALSE(other);

    resource.reset(std::make_shared<int>(1));

    ASSERT_TRUE(resource);
    ASSERT_EQ(*resource, 1);
    ASSERT_FALSE(other);

    resource.swap(other);

    ASSERT_FALSE(resource);
    ASSERT_TRUE(other);
    ASSERT_EQ(*other, 1);
}

TEST(Resource, DerivedToBase) {
    const entt::resource<derived> resource{std::make_shared<derived>()};
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

    testing::StaticAssertTypeEq<decltype(*resource), derived &>();
    testing::StaticAssertTypeEq<decltype(*entt::resource<const derived>{other}), const derived &>();
    testing::StaticAssertTypeEq<decltype(*std::as_const(resource)), derived &>();

    entt::resource<const derived> copy{resource};
    entt::resource<const derived> move{std::move(other)};

    test::is_initialized(other);

    ASSERT_TRUE(resource);
    ASSERT_FALSE(other);

    ASSERT_TRUE(copy);
    ASSERT_EQ(copy, resource);
    ASSERT_NE(copy, entt::resource<derived>{});
    ASSERT_EQ(copy.handle().use_count(), 3);

    ASSERT_TRUE(move);
    ASSERT_EQ(move, resource);
    ASSERT_NE(move, entt::resource<derived>{});
    ASSERT_EQ(move.handle().use_count(), 3);

    copy = resource;
    move = std::move(resource);
    test::is_initialized(resource);

    ASSERT_FALSE(resource);
    ASSERT_FALSE(other);

    ASSERT_TRUE(copy);
    ASSERT_TRUE(move);
    ASSERT_EQ(copy.handle().use_count(), 2);
}

TEST(Resource, DynamicResourceHandleCast) {
    const entt::resource<derived> resource{std::make_shared<derived>()};
    entt::resource<const base> other = resource;

    ASSERT_TRUE(other);
    ASSERT_EQ(resource.handle().use_count(), 2);
    ASSERT_EQ(resource, other);

    entt::resource<const derived> cast = dynamic_resource_cast<const derived>(other);

    ASSERT_TRUE(cast);
    ASSERT_EQ(resource.handle().use_count(), 3);
    ASSERT_EQ(resource, cast);

    other = entt::resource<base>{std::make_shared<base>()};
    cast = dynamic_resource_cast<const derived>(other);

    ASSERT_FALSE(cast);
    ASSERT_EQ(resource.handle().use_count(), 1);
}

TEST(Resource, Comparison) {
    const entt::resource<derived> resource{std::make_shared<derived>()};
    const entt::resource<const base> other = resource;

    ASSERT_TRUE(resource == other);
    ASSERT_FALSE(resource != other);

    ASSERT_FALSE(resource < other);
    ASSERT_FALSE(resource > other);

    ASSERT_TRUE(resource <= other);
    ASSERT_TRUE(resource >= other);
}
