#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/any.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/context.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "../../common/config.h"
#include "../../common/linter.hpp"
#include "../../common/non_comparable.h"

struct clazz {
    void member(int iv) {
        value = iv;
    }

    static char func() {
        return 'c';
    }

    operator int() const {
        return value;
    }

    int value{0};
};

struct empty {
    empty() = default;

    empty(const empty &) = default;
    empty &operator=(const empty &) = default;

    virtual ~empty() {
        ++destructor_counter;
    }

    static void destroy(empty &) {
        ++destroy_counter;
    }

    inline static int destroy_counter = 0;    // NOLINT
    inline static int destructor_counter = 0; // NOLINT
};

struct fat: empty {
    fat()
        : value{.0, .0, .0, .0} {}

    fat(double v1, double v2, double v3, double v4)
        : value{v1, v2, v3, v4} {}

    ~fat() override = default;

    fat(const fat &) = default;
    fat &operator=(const fat &) = default;

    bool operator==(const fat &other) const {
        return (value == other.value);
    }

    std::array<double, 4u> value{};
};

enum class enum_class : std::uint8_t {
    foo = 0u,
    bar = 1u
};

struct unmanageable {
    unmanageable()
        : value{std::make_unique<int>(3)} {}

    ~unmanageable() = default;

    unmanageable(const unmanageable &) = delete;
    unmanageable(unmanageable &&) = delete;

    unmanageable &operator=(const unmanageable &) = delete;
    unmanageable &operator=(unmanageable &&) = delete;

    std::unique_ptr<int> value;
};

struct MetaAny: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta_factory<empty>{}
            .type("empty"_hs)
            .dtor<empty::destroy>();

        entt::meta_factory<fat>{}
            .type("fat"_hs)
            .base<empty>()
            .dtor<fat::destroy>();

        entt::meta_factory<clazz>{}
            .type("clazz"_hs)
            .data<&clazz::value>("value"_hs)
            .func<&clazz::member>("member"_hs)
            .func<clazz::func>("func"_hs)
            .conv<int>();

        empty::destroy_counter = 0;
        empty::destructor_counter = 0;
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

using MetaAnyDeathTest = MetaAny;

TEST_F(MetaAny, Empty) {
    entt::meta_any any{};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.base().data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{});
    ASSERT_NE(entt::meta_any{'c'}, any);

    ASSERT_FALSE(any.as_ref());
    ASSERT_FALSE(any.as_sequence_container());
    ASSERT_FALSE(any.as_associative_container());

    ASSERT_FALSE(std::as_const(any).as_ref());
    ASSERT_FALSE(std::as_const(any).as_sequence_container());
    ASSERT_FALSE(std::as_const(any).as_associative_container());
}

TEST_F(MetaAny, Context) {
    entt::meta_any any{};
    const entt::meta_ctx ctx{};

    ASSERT_EQ(&any.context(), &entt::locator<entt::meta_ctx>::value_or());
    ASSERT_NE(&any.context(), &ctx);

    any = entt::meta_any{entt::meta_ctx_arg, ctx};

    ASSERT_NE(&any.context(), &entt::locator<entt::meta_ctx>::value_or());
    ASSERT_EQ(&any.context(), &ctx);
}

TEST_F(MetaAny, SBO) {
    entt::meta_any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::embedded);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<char>(), 'c');
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(entt::meta_any{'h'}, any);
}

TEST_F(MetaAny, NoSBO) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{instance};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::dynamic);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat>(), instance);
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(any, fat{});
}

TEST_F(MetaAny, SBOInPlaceConstruction) {
    std::unique_ptr<int> elem = std::make_unique<int>(2);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    entt::meta_any any{std::in_place, elem.release()};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::dynamic);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 2);
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{2});
    ASSERT_NE(entt::meta_any{0}, any);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.base().owner());
    ASSERT_EQ(other.base().policy(), entt::any_policy::ref);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 2);
    ASSERT_NE(other.base().data(), nullptr);
    ASSERT_EQ(other, entt::meta_any{2});
    ASSERT_NE(entt::meta_any{0}, other);
}

TEST_F(MetaAny, SBOInPlaceNullptrConstruction) {
    int *instance = nullptr;
    entt::meta_any any{std::in_place, instance};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_FALSE(any.try_cast<int>());
    ASSERT_EQ(any.base().data(), nullptr);
}

TEST_F(MetaAny, SBOInPlaceTypeConstruction) {
    entt::meta_any any{std::in_place_type<int>, 3};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<int>, 3}));
    ASSERT_EQ(any, entt::meta_any{3});
    ASSERT_NE(entt::meta_any{1}, any);
}

TEST_F(MetaAny, SBOAsRefConstruction) {
    int value = 1;
    int compare = 3;
    auto any = entt::forward_as_meta(value);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::ref);
    ASSERT_EQ(any.type(), entt::resolve<int>());

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int &>(), 1);
    ASSERT_EQ(any.cast<const int &>(), 1);
    ASSERT_EQ(any.cast<int>(), 1);
    ASSERT_EQ(any.base().data(), &value);

    ASSERT_EQ(any, entt::forward_as_meta(value));
    ASSERT_NE(any, entt::forward_as_meta(compare));

    ASSERT_NE(any, entt::meta_any{3});
    ASSERT_EQ(entt::meta_any{1}, any);

    any = entt::forward_as_meta(value);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.base().data(), &value);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 1);
    ASSERT_EQ(other.base().data(), any.base().data());
}

TEST_F(MetaAny, SBOAsConstRefConstruction) {
    const int value = 1;
    int compare = 3;
    auto any = entt::forward_as_meta(value);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::cref);
    ASSERT_EQ(any.type(), entt::resolve<int>());

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<const int &>(), 1);
    ASSERT_EQ(any.cast<int>(), 1);
    ASSERT_EQ(any.base().data(), &value);

    ASSERT_EQ(any, entt::forward_as_meta(value));
    ASSERT_NE(any, entt::forward_as_meta(compare));

    ASSERT_NE(any, entt::meta_any{3});
    ASSERT_EQ(entt::meta_any{1}, any);

    any = entt::forward_as_meta(std::as_const(value));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.base().data(), &value);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 1);
    ASSERT_EQ(other.base().data(), any.base().data());
}

ENTT_DEBUG_TEST_F(MetaAnyDeathTest, SBOAsConstRefConstruction) {
    const int value = 1;
    auto any = entt::forward_as_meta(value);

    ASSERT_TRUE(any);
    ASSERT_DEATH([[maybe_unused]] auto &elem = any.cast<int &>(), "");
}

TEST_F(MetaAny, SBOCopyConstruction) {
    const entt::meta_any any{3};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 3);
    ASSERT_EQ(other, entt::meta_any{3});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(MetaAny, SBOCopyAssignment) {
    const entt::meta_any any{3};
    entt::meta_any other{1};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 3);
    ASSERT_EQ(other, entt::meta_any{3});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(MetaAny, SBOSelfCopyAssignment) {
    entt::meta_any any{3};

    // avoid warnings due to self-assignment
    any = *&any;

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_EQ(any, entt::meta_any{3});
    ASSERT_NE(any, entt::meta_any{0});
}

TEST_F(MetaAny, SBOMoveConstruction) {
    entt::meta_any any{3};
    entt::meta_any other{std::move(any)};

    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 3);
    ASSERT_EQ(other, entt::meta_any{3});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(MetaAny, SBOMoveAssignment) {
    entt::meta_any any{3};
    entt::meta_any other{1};

    other = std::move(any);
    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<int>(), 3);
    ASSERT_EQ(other, entt::meta_any{3});
    ASSERT_NE(other, entt::meta_any{0});
}

TEST_F(MetaAnyDeathTest, SBOSelfMoveAssignment) {
    entt::meta_any any{3};

    // avoid warnings due to self-assignment
    any = std::move(*&any);

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(any.base().data(), nullptr);
}

TEST_F(MetaAny, SBODirectAssignment) {
    entt::meta_any any{};
    any = 3;

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_EQ(any, entt::meta_any{3});
    ASSERT_NE(entt::meta_any{0}, any);
}

TEST_F(MetaAny, SBOAssignValue) {
    entt::meta_any any{3};
    const entt::meta_any other{1};
    const entt::meta_any invalid{empty{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(any.cast<int>(), 1);
}

TEST_F(MetaAny, SBOConvertAssignValue) {
    entt::meta_any any{3};
    const entt::meta_any other{1.5};
    const entt::meta_any invalid{empty{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(any.cast<int>(), 1);
}

TEST_F(MetaAny, SBOAsRefAssignValue) {
    int value = 3;
    entt::meta_any any{entt::forward_as_meta(value)};
    const entt::meta_any other{1};
    const entt::meta_any invalid{empty{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(any.cast<int>(), 1);
    ASSERT_EQ(value, 1);
}

TEST_F(MetaAny, SBOAsConstRefAssignValue) {
    const int value = 3;
    entt::meta_any any{entt::forward_as_meta(value)};
    const entt::meta_any other{1};
    const entt::meta_any invalid{empty{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_FALSE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(MetaAny, SBOTransferValue) {
    entt::meta_any any{3};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.assign(1));
    ASSERT_FALSE(any.assign(empty{}));
    ASSERT_EQ(any.cast<int>(), 1);
}

TEST_F(MetaAny, SBOTransferConstValue) {
    const int value = 1;
    entt::meta_any any{3};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.assign(entt::forward_as_meta(value)));
    ASSERT_EQ(any.cast<int>(), 1);
}

TEST_F(MetaAny, SBOConvertTransferValue) {
    entt::meta_any any{3};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.assign(1.5));
    ASSERT_FALSE(any.assign(empty{}));
    ASSERT_EQ(any.cast<int>(), 1);
}

TEST_F(MetaAny, SBOAsRefTransferValue) {
    int value = 3;
    entt::meta_any any{entt::forward_as_meta(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.assign(1));
    ASSERT_FALSE(any.assign(empty{}));
    ASSERT_EQ(any.cast<int>(), 1);
    ASSERT_EQ(value, 1);
}

TEST_F(MetaAny, SBOAsConstRefTransferValue) {
    const int value = 3;
    entt::meta_any any{entt::forward_as_meta(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_FALSE(any.assign(1));
    ASSERT_FALSE(any.assign(empty{}));
    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(MetaAny, NoSBOInPlaceConstruction) {
    const fat instance{.1, .2, .3, .4};
    std::unique_ptr<fat> elem = std::make_unique<fat>(instance);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
    entt::meta_any any{std::in_place, elem.release()};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::dynamic);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat>(), instance);
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(any, fat{});

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.base().owner());
    ASSERT_EQ(other.base().policy(), entt::any_policy::ref);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat>(), instance);
    ASSERT_NE(other.base().data(), nullptr);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(any, fat{});
}

TEST_F(MetaAny, NoSBOInPlaceNullptrConstruction) {
    fat *instance = nullptr;
    entt::meta_any any{std::in_place, instance};

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_FALSE(any.try_cast<fat>());
    ASSERT_EQ(any.base().data(), nullptr);
}

TEST_F(MetaAny, NoSBOInPlaceTypeConstruction) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{std::in_place_type<fat>, instance};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat>(), instance);
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<fat>, instance}));
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(entt::meta_any{fat{}}, any);
}

TEST_F(MetaAny, NoSBOAsRefConstruction) {
    fat instance{.1, .2, .3, .4};
    auto any = entt::forward_as_meta(instance);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::ref);
    ASSERT_EQ(any.type(), entt::resolve<fat>());

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat &>(), instance);
    ASSERT_EQ(any.cast<const fat &>(), instance);
    ASSERT_EQ(any.cast<fat>(), instance);
    ASSERT_EQ(any.base().data(), &instance);

    ASSERT_EQ(any, entt::forward_as_meta(instance));

    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(entt::meta_any{fat{}}, any);

    any = entt::forward_as_meta(instance);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<fat>());
    ASSERT_EQ(any.base().data(), &instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::resolve<fat>());
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_EQ(other.base().data(), any.base().data());
}

TEST_F(MetaAny, NoSBOAsConstRefConstruction) {
    const fat instance{.1, .2, .3, .4};
    auto any = entt::forward_as_meta(instance);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::cref);
    ASSERT_EQ(any.type(), entt::resolve<fat>());

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<const fat &>(), instance);
    ASSERT_EQ(any.cast<fat>(), instance);
    ASSERT_EQ(any.base().data(), &instance);

    ASSERT_EQ(any, entt::forward_as_meta(instance));

    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(entt::meta_any{fat{}}, any);

    any = entt::forward_as_meta(std::as_const(instance));

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<fat>());
    ASSERT_EQ(any.base().data(), &instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::resolve<fat>());
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_EQ(other.base().data(), any.base().data());
}

ENTT_DEBUG_TEST_F(MetaAnyDeathTest, NoSBOAsConstRefConstruction) {
    const fat instance{.1, .2, .3, .4};
    auto any = entt::forward_as_meta(instance);

    ASSERT_TRUE(any);
    ASSERT_DEATH([[maybe_unused]] auto &elem = any.cast<fat &>(), "");
}

TEST_F(MetaAny, NoSBOCopyConstruction) {
    const fat instance{.1, .2, .3, .4};
    const entt::meta_any any{instance};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat{});
}

TEST_F(MetaAny, NoSBOCopyAssignment) {
    const fat instance{.1, .2, .3, .4};
    const entt::meta_any any{instance};
    entt::meta_any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat{});
}

TEST_F(MetaAny, NoSBOSelfCopyAssignment) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{instance};

    // avoid warnings due to self-assignment
    any = *&any;

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat>(), instance);
    ASSERT_EQ(any, entt::meta_any{instance});
    ASSERT_NE(any, fat{});
}

TEST_F(MetaAny, NoSBOMoveConstruction) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{instance};
    entt::meta_any other{std::move(any)};

    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat{});
}

TEST_F(MetaAny, NoSBOMoveAssignment) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{instance};
    entt::meta_any other{3};

    other = std::move(any);
    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(other.cast<fat>(), instance);
    ASSERT_EQ(other, entt::meta_any{instance});
    ASSERT_NE(other, fat{});
}

TEST_F(MetaAnyDeathTest, NoSBOSelfMoveAssignment) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{instance};

    // avoid warnings due to self-assignment
    any = std::move(*&any);

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(any.base().data(), nullptr);
}

TEST_F(MetaAny, NoSBODirectAssignment) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{};
    any = instance;

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<fat>(), instance);
    ASSERT_EQ(any, (entt::meta_any{fat{.1, .2, .3, .4}}));
    ASSERT_NE(any, fat{});
}

TEST_F(MetaAny, NoSBOAssignValue) {
    entt::meta_any any{fat{.1, .2, .3, .4}};
    const entt::meta_any other{fat{.0, .1, .2, .3}};
    const entt::meta_any invalid{'c'};

    const void *addr = any.base().data();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(any.cast<const fat &>(), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, any.base().data());
}

TEST_F(MetaAny, NoSBOConvertAssignValue) {
    entt::meta_any any{empty{}};
    const entt::meta_any other{fat{.0, .1, .2, .3}};
    const entt::meta_any invalid{'c'};

    const void *addr = any.base().data();

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(addr, any.base().data());
}

TEST_F(MetaAny, NoSBOAsRefAssignValue) {
    fat instance{.1, .2, .3, .4};
    entt::meta_any any{entt::forward_as_meta(instance)};
    const entt::meta_any other{fat{.0, .1, .2, .3}};
    const entt::meta_any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(any.cast<const fat &>(), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(instance, (fat{.0, .1, .2, .3}));
}

TEST_F(MetaAny, NoSBOAsConstRefAssignValue) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{entt::forward_as_meta(instance)};
    const entt::meta_any other{fat{.0, .1, .2, .3}};
    const entt::meta_any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));

    ASSERT_FALSE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(instance, (fat{.1, .2, .3, .4}));
}

TEST_F(MetaAny, NoSBOTransferValue) {
    entt::meta_any any{fat{.1, .2, .3, .4}};

    const void *addr = any.base().data();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(fat{.0, .1, .2, .3}));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(any.cast<const fat &>(), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, any.base().data());
}

TEST_F(MetaAny, NoSBOTransferConstValue) {
    const fat instance{.0, .1, .2, .3};
    entt::meta_any any{fat{.1, .2, .3, .4}};

    const void *addr = any.base().data();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(entt::forward_as_meta(instance)));
    ASSERT_EQ(any.cast<const fat &>(), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, any.base().data());
}

TEST_F(MetaAny, NoSBOConvertTransferValue) {
    entt::meta_any any{empty{}};

    const void *addr = any.base().data();

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.assign(fat{.0, .1, .2, .3}));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(addr, any.base().data());
}

TEST_F(MetaAny, NoSBOAsRefTransferValue) {
    fat instance{.1, .2, .3, .4};
    entt::meta_any any{entt::forward_as_meta(instance)};

    const void *addr = any.base().data();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(fat{.0, .1, .2, .3}));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(any.cast<const fat &>(), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(instance, (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, any.base().data());
}

TEST_F(MetaAny, NoSBOAsConstRefTransferValue) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{entt::forward_as_meta(instance)};

    const void *addr = any.base().data();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));

    ASSERT_FALSE(any.assign(fat{.0, .1, .2, .3}));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(any.cast<const fat &>(), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(instance, (fat{.1, .2, .3, .4}));
    ASSERT_EQ(addr, any.base().data());
}

TEST_F(MetaAny, VoidInPlaceTypeConstruction) {
    entt::meta_any any{std::in_place_type<void>};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::empty);
    ASSERT_FALSE(any.try_cast<char>());
    ASSERT_EQ(any.base().data(), nullptr);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(any, entt::meta_any{std::in_place_type<void>});
    ASSERT_NE(entt::meta_any{3}, any);
}

TEST_F(MetaAny, VoidAsRefConstruction) {
    entt::meta_any any{std::in_place_type<void>};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.base().owner());
    ASSERT_EQ(any.base().policy(), entt::any_policy::empty);
    ASSERT_EQ(any.type(), entt::resolve<void>());

    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.base().data(), nullptr);

    ASSERT_EQ(any, entt::meta_any{std::in_place_type<void>});
    ASSERT_EQ(entt::meta_any{std::in_place_type<void>}, any);
    ASSERT_NE(entt::meta_any{3}, any);

    any = entt::meta_any{std::in_place_type<void>};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(any.base().data(), nullptr);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(any.base().data(), nullptr);
    ASSERT_EQ(other.base().data(), any.base().data());
}

TEST_F(MetaAny, VoidCopyConstruction) {
    const entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaAny, VoidCopyAssignment) {
    const entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{std::in_place_type<void>};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_FALSE(other.try_cast<std::size_t>());
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaAny, VoidSelfCopyAssignment) {
    entt::meta_any any{std::in_place_type<void>};

    // avoid warnings due to self-assignment
    any = *&any;

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(any, entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaAny, VoidMoveConstruction) {
    entt::meta_any any{std::in_place_type<void>};
    const entt::meta_any other{std::move(any)};

    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaAny, VoidMoveAssignment) {
    entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{std::in_place_type<void>};

    other = std::move(any);
    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.type(), entt::resolve<void>());
    ASSERT_EQ(other, entt::meta_any{std::in_place_type<void>});
}

TEST_F(MetaAnyDeathTest, VoidSelfMoveAssignment) {
    entt::meta_any any{std::in_place_type<void>};

    // avoid warnings due to self-assignment
    any = std::move(*&any);

    ASSERT_FALSE(any);
    ASSERT_FALSE(any.type());
    ASSERT_EQ(any.base().data(), nullptr);
}

TEST_F(MetaAny, SBOMoveInvalidate) {
    entt::meta_any any{3};
    entt::meta_any other{std::move(any)};
    const entt::meta_any valid = std::move(other);

    test::is_initialized(any);
    test::is_initialized(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
}

TEST_F(MetaAny, NoSBOMoveInvalidate) {
    const fat instance{.1, .2, .3, .4};
    entt::meta_any any{instance};
    entt::meta_any other{std::move(any)};
    const entt::meta_any valid = std::move(other);

    test::is_initialized(any);
    test::is_initialized(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
}

TEST_F(MetaAny, VoidMoveInvalidate) {
    entt::meta_any any{std::in_place_type<void>};
    entt::meta_any other{std::move(any)};
    const entt::meta_any valid = std::move(other);

    test::is_initialized(any);
    test::is_initialized(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(valid);
}

TEST_F(MetaAny, SBODestruction) {
    {
        entt::meta_any any{std::in_place_type<empty>};
        any.emplace<empty>();
        any = empty{};
        entt::meta_any other{std::move(any)};
        any = std::move(other);
    }

    ASSERT_EQ(empty::destroy_counter, 3);
    ASSERT_EQ(empty::destructor_counter, 6);
}

TEST_F(MetaAny, NoSBODestruction) {
    {
        entt::meta_any any{std::in_place_type<fat>, 1., 2., 3., 4.};
        any.emplace<fat>(1., 2., 3., 4.);
        any = fat{1., 2., 3., 4.};
        entt::meta_any other{std::move(any)};
        any = std::move(other);
    }

    ASSERT_EQ(fat::destroy_counter, 3);
    ASSERT_EQ(empty::destructor_counter, 4);
}

TEST_F(MetaAny, VoidDestruction) {
    // just let asan tell us if everything is ok here
    [[maybe_unused]] const entt::meta_any any{std::in_place_type<void>};
}

TEST_F(MetaAny, Emplace) {
    entt::meta_any any{};
    any.emplace<int>(3);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.try_cast<std::size_t>());
    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<int>, 3}));
    ASSERT_EQ(any, entt::meta_any{3});
    ASSERT_NE(entt::meta_any{1}, any);
}

TEST_F(MetaAny, EmplaceVoid) {
    entt::meta_any any{};
    any.emplace<void>();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.base().data(), nullptr);
    ASSERT_EQ(any.type(), entt::resolve<void>());
    ASSERT_EQ(any, (entt::meta_any{std::in_place_type<void>}));
}

TEST_F(MetaAny, Reset) {
    entt::meta_any any{3};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<int>());

    any.reset();

    ASSERT_FALSE(any);
    ASSERT_EQ(any.type(), entt::meta_type{});
}

TEST_F(MetaAny, SBOSwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{3};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs.try_cast<char>());
    ASSERT_EQ(lhs.cast<int>(), 3);
    ASSERT_FALSE(rhs.try_cast<int>());
    ASSERT_EQ(rhs.cast<char>(), 'c');
}

TEST_F(MetaAny, NoSBOSwap) {
    entt::meta_any lhs{fat{.1, .2, .3, .4}};
    entt::meta_any rhs{fat{.4, .3, .2, .1}};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.cast<fat>(), (fat{.4, .3, .2, .1}));
    ASSERT_EQ(rhs.cast<fat>(), (fat{.1, .2, .3, .4}));
}

TEST_F(MetaAny, VoidSwap) {
    entt::meta_any lhs{std::in_place_type<void>};
    entt::meta_any rhs{std::in_place_type<void>};
    const auto *pre = lhs.base().data();

    std::swap(lhs, rhs);

    ASSERT_EQ(pre, lhs.base().data());
}

TEST_F(MetaAny, SBOWithNoSBOSwap) {
    entt::meta_any lhs{fat{.1, .2, .3, .4}};
    entt::meta_any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs.try_cast<fat>());
    ASSERT_EQ(lhs.cast<char>(), 'c');
    ASSERT_FALSE(rhs.try_cast<char>());
    ASSERT_EQ(rhs.cast<fat>(), (fat{.1, .2, .3, .4}));
}

TEST_F(MetaAny, SBOWithEmptySwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(rhs.cast<char>(), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.cast<char>(), 'c');
}

TEST_F(MetaAny, SBOWithVoidSwap) {
    entt::meta_any lhs{'c'};
    entt::meta_any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.type(), entt::resolve<void>());
    ASSERT_EQ(rhs.cast<char>(), 'c');
}

TEST_F(MetaAny, NoSBOWithEmptySwap) {
    entt::meta_any lhs{fat{.1, .2, .3, .4}};
    entt::meta_any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(rhs.cast<fat>(), (fat{.1, .2, .3, .4}));

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.cast<fat>(), (fat{.1, .2, .3, .4}));
}

TEST_F(MetaAny, NoSBOWithVoidSwap) {
    entt::meta_any lhs{fat{.1, .2, .3, .4}};
    entt::meta_any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.type(), entt::resolve<void>());
    ASSERT_EQ(rhs.cast<fat>(), (fat{.1, .2, .3, .4}));

    std::swap(lhs, rhs);

    ASSERT_EQ(rhs.type(), entt::resolve<void>());
    ASSERT_EQ(lhs.cast<fat>(), (fat{.1, .2, .3, .4}));
}

TEST_F(MetaAny, AsRef) {
    entt::meta_any any{3};
    auto ref = any.as_ref();
    auto cref = std::as_const(any).as_ref();

    ASSERT_EQ(any.try_cast<int>(), any.base().data());
    ASSERT_EQ(ref.try_cast<int>(), any.base().data());
    ASSERT_EQ(cref.try_cast<int>(), nullptr);

    ASSERT_EQ(any.try_cast<const int>(), any.base().data());
    ASSERT_EQ(ref.try_cast<const int>(), any.base().data());
    ASSERT_EQ(cref.try_cast<const int>(), any.base().data());

    ASSERT_EQ(any.cast<int>(), 3);
    ASSERT_EQ(ref.cast<int>(), 3);
    ASSERT_EQ(cref.cast<int>(), 3);

    ASSERT_EQ(any.cast<const int>(), 3);
    ASSERT_EQ(ref.cast<const int>(), 3);
    ASSERT_EQ(cref.cast<const int>(), 3);

    ASSERT_EQ(any.cast<int &>(), 3);
    ASSERT_EQ(any.cast<const int &>(), 3);
    ASSERT_EQ(ref.cast<int &>(), 3);
    ASSERT_EQ(ref.cast<const int &>(), 3);
    ASSERT_EQ(cref.cast<const int &>(), 3);

    any.cast<int &>() = 1;

    ASSERT_EQ(any.cast<int>(), 1);
    ASSERT_EQ(ref.cast<int>(), 1);
    ASSERT_EQ(cref.cast<int>(), 1);

    std::swap(ref, cref);

    ASSERT_EQ(ref.try_cast<int>(), nullptr);
    ASSERT_EQ(cref.try_cast<int>(), any.base().data());

    ref = ref.as_ref();
    cref = std::as_const(cref).as_ref();

    ASSERT_EQ(ref.try_cast<int>(), nullptr);
    ASSERT_EQ(cref.try_cast<int>(), nullptr);
    ASSERT_EQ(ref.try_cast<const int>(), any.base().data());
    ASSERT_EQ(cref.try_cast<const int>(), any.base().data());

    ASSERT_EQ(ref.cast<const int &>(), 1);
    ASSERT_EQ(cref.cast<const int &>(), 1);

    ref = 3;
    cref = 3;

    ASSERT_NE(ref.try_cast<int>(), nullptr);
    ASSERT_NE(cref.try_cast<int>(), nullptr);
    ASSERT_EQ(ref.cast<int &>(), 3);
    ASSERT_EQ(cref.cast<int &>(), 3);
    ASSERT_EQ(ref.cast<const int &>(), 3);
    ASSERT_EQ(cref.cast<const int &>(), 3);
    ASSERT_NE(ref.try_cast<int>(), any.base().data());
    ASSERT_NE(cref.try_cast<int>(), any.base().data());

    any.emplace<void>();
    ref = any.as_ref();
    cref = std::as_const(any).as_ref();

    ASSERT_TRUE(any);
    ASSERT_TRUE(ref);
    ASSERT_TRUE(cref);
}

ENTT_DEBUG_TEST_F(MetaAnyDeathTest, AsRef) {
    entt::meta_any any{3};
    auto cref = std::as_const(any).as_ref();

    ASSERT_TRUE(any);
    ASSERT_DEATH([[maybe_unused]] auto &elem = cref.cast<int &>(), "");
}

TEST_F(MetaAny, Comparable) {
    const entt::meta_any any{'c'};

    ASSERT_EQ(any, any);
    ASSERT_EQ(any, entt::meta_any{'c'});
    ASSERT_NE(entt::meta_any{'a'}, any);
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_TRUE(any == entt::meta_any{'c'});
    ASSERT_FALSE(entt::meta_any{'a'} == any);
    ASSERT_TRUE(any != entt::meta_any{'a'});
    ASSERT_TRUE(entt::meta_any{} != any);
}

TEST_F(MetaAny, NonComparable) {
    const entt::meta_any any{test::non_comparable{}};

    ASSERT_EQ(any, any);
    ASSERT_NE(any, entt::meta_any{test::non_comparable{}});
    ASSERT_NE(entt::meta_any{}, any);

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(any == entt::meta_any{test::non_comparable{}});
    ASSERT_TRUE(entt::meta_any{} != any);
}

TEST_F(MetaAny, CompareVoid) {
    const entt::meta_any any{std::in_place_type<void>};

    ASSERT_EQ(any, any);
    ASSERT_EQ(any, entt::meta_any{std::in_place_type<void>});
    ASSERT_NE(entt::meta_any{'a'}, any);
    ASSERT_NE(any, entt::meta_any{});

    ASSERT_TRUE(any == any);
    ASSERT_TRUE(any == entt::meta_any{std::in_place_type<void>});
    ASSERT_FALSE(entt::meta_any{'a'} == any);
    ASSERT_TRUE(any != entt::meta_any{'a'});
    ASSERT_TRUE(entt::meta_any{} != any);
}

TEST_F(MetaAny, TryCast) {
    entt::meta_any any{fat{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<fat>());
    ASSERT_EQ(any.try_cast<void>(), nullptr);
    ASSERT_NE(any.try_cast<empty>(), nullptr);
    ASSERT_EQ(any.try_cast<fat>(), any.base().data());
    ASSERT_EQ(std::as_const(any).try_cast<empty>(), any.try_cast<empty>());
    ASSERT_EQ(std::as_const(any).try_cast<fat>(), any.base().data());
    ASSERT_EQ(std::as_const(any).try_cast<int>(), nullptr);
}

TEST_F(MetaAny, Cast) {
    entt::meta_any any{fat{}};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<fat>());
    ASSERT_EQ(std::as_const(any).cast<const fat &>(), fat{});
    ASSERT_EQ(any.cast<const fat>(), fat{});
    ASSERT_EQ(any.cast<fat &>(), fat{});
    ASSERT_EQ(any.cast<fat>(), fat{});

    ASSERT_EQ(any.cast<fat>().value[0u], 0.);

    any.cast<fat &>().value[0u] = 3.;

    ASSERT_EQ(any.cast<fat>().value[0u], 3.);
}

TEST_F(MetaAny, AllowCast) {
    entt::meta_any instance{clazz{}};
    entt::meta_any other{fat{}};
    entt::meta_any arithmetic{3};
    auto as_cref = entt::forward_as_meta(arithmetic.cast<const int &>());

    ASSERT_TRUE(instance);
    ASSERT_TRUE(other);
    ASSERT_TRUE(arithmetic);
    ASSERT_TRUE(as_cref);

    ASSERT_TRUE(instance.allow_cast<clazz>());
    ASSERT_TRUE(instance.allow_cast<clazz &>());
    ASSERT_TRUE(instance.allow_cast<const clazz &>());
    ASSERT_EQ(instance.type(), entt::resolve<clazz>());

    ASSERT_TRUE(instance.allow_cast<const int &>());
    ASSERT_EQ(instance.type(), entt::resolve<int>());
    ASSERT_TRUE(instance.allow_cast<int>());
    ASSERT_TRUE(instance.allow_cast<int &>());
    ASSERT_TRUE(instance.allow_cast<const int &>());

    ASSERT_TRUE(other.allow_cast<fat>());
    ASSERT_TRUE(other.allow_cast<fat &>());
    ASSERT_TRUE(other.allow_cast<const empty &>());
    ASSERT_EQ(other.type(), entt::resolve<fat>());
    ASSERT_FALSE(other.allow_cast<int>());

    ASSERT_TRUE(std::as_const(other).allow_cast<fat>());
    ASSERT_FALSE(std::as_const(other).allow_cast<fat &>());
    ASSERT_TRUE(std::as_const(other).allow_cast<const empty &>());
    ASSERT_EQ(other.type(), entt::resolve<fat>());
    ASSERT_FALSE(other.allow_cast<int>());

    ASSERT_TRUE(arithmetic.allow_cast<int>());
    ASSERT_TRUE(arithmetic.allow_cast<int &>());
    ASSERT_TRUE(arithmetic.allow_cast<const int &>());
    ASSERT_EQ(arithmetic.type(), entt::resolve<int>());
    ASSERT_FALSE(arithmetic.allow_cast<fat>());

    ASSERT_TRUE(arithmetic.allow_cast<double &>());
    ASSERT_EQ(arithmetic.type(), entt::resolve<double>());
    ASSERT_EQ(arithmetic.cast<double &>(), 3.);

    ASSERT_TRUE(arithmetic.allow_cast<const float &>());
    ASSERT_EQ(arithmetic.type(), entt::resolve<float>());
    ASSERT_EQ(arithmetic.cast<float &>(), 3.f);

    ASSERT_TRUE(as_cref.allow_cast<int>());
    ASSERT_FALSE(as_cref.allow_cast<int &>());
    ASSERT_TRUE(as_cref.allow_cast<const int &>());
    ASSERT_EQ(as_cref.type(), entt::resolve<int>());
    ASSERT_FALSE(as_cref.allow_cast<fat>());

    ASSERT_TRUE(as_cref.allow_cast<double &>());
    ASSERT_EQ(as_cref.type(), entt::resolve<double>());
}

TEST_F(MetaAny, OpaqueAllowCast) {
    entt::meta_any instance{clazz{}};
    entt::meta_any other{fat{}};
    entt::meta_any arithmetic{3};
    auto as_cref = entt::forward_as_meta(arithmetic.cast<const int &>());

    ASSERT_TRUE(instance);
    ASSERT_TRUE(other);
    ASSERT_TRUE(arithmetic);
    ASSERT_TRUE(as_cref);

    ASSERT_TRUE(instance.allow_cast(entt::resolve<clazz>()));
    ASSERT_EQ(instance.type(), entt::resolve<clazz>());

    ASSERT_TRUE(instance.allow_cast(entt::resolve<int>()));
    ASSERT_EQ(instance.type(), entt::resolve<int>());
    ASSERT_TRUE(instance.allow_cast(entt::resolve<int>()));

    ASSERT_TRUE(other.allow_cast(entt::resolve<fat>()));
    ASSERT_TRUE(other.allow_cast(entt::resolve<empty>()));
    ASSERT_EQ(other.type(), entt::resolve<fat>());
    ASSERT_FALSE(other.allow_cast(entt::resolve<int>()));

    ASSERT_TRUE(std::as_const(other).allow_cast(entt::resolve<fat>()));
    ASSERT_TRUE(std::as_const(other).allow_cast(entt::resolve<empty>()));
    ASSERT_EQ(other.type(), entt::resolve<fat>());
    ASSERT_FALSE(other.allow_cast(entt::resolve<int>()));

    ASSERT_TRUE(arithmetic.allow_cast(entt::resolve<int>()));
    ASSERT_EQ(arithmetic.type(), entt::resolve<int>());
    ASSERT_FALSE(arithmetic.allow_cast(entt::resolve<fat>()));

    ASSERT_TRUE(arithmetic.allow_cast(entt::resolve<double>()));
    ASSERT_EQ(arithmetic.type(), entt::resolve<double>());
    ASSERT_EQ(arithmetic.cast<double &>(), 3.);

    ASSERT_TRUE(arithmetic.allow_cast(entt::resolve<float>()));
    ASSERT_EQ(arithmetic.type(), entt::resolve<float>());
    ASSERT_EQ(arithmetic.cast<float &>(), 3.f);

    ASSERT_TRUE(as_cref.allow_cast(entt::resolve<int>()));
    ASSERT_EQ(as_cref.type(), entt::resolve<int>());
    ASSERT_FALSE(as_cref.allow_cast(entt::resolve<fat>()));

    ASSERT_TRUE(as_cref.allow_cast(entt::resolve<double>()));
    ASSERT_EQ(as_cref.type(), entt::resolve<double>());

    ASSERT_TRUE(as_cref.allow_cast(entt::resolve<float>()));
    ASSERT_EQ(as_cref.type(), entt::resolve<float>());
}

TEST_F(MetaAny, Convert) {
    entt::meta_any any{clazz{}};
    any.cast<clazz &>().value = 3;
    auto as_int = std::as_const(any).allow_cast<int>();

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::resolve<clazz>());
    ASSERT_TRUE(any.allow_cast<clazz>());
    ASSERT_EQ(any.type(), entt::resolve<clazz>());
    ASSERT_TRUE(any.allow_cast<int>());
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(as_int);
    ASSERT_EQ(as_int.type(), entt::resolve<int>());
    ASSERT_EQ(as_int.cast<int>(), 3);

    ASSERT_TRUE(as_int.allow_cast<char>());
    ASSERT_EQ(as_int.type(), entt::resolve<char>());
    ASSERT_EQ(as_int.cast<char>(), char{3});
}

TEST_F(MetaAny, ArithmeticConversion) {
    entt::meta_any any{'c'};

    ASSERT_EQ(any.type(), entt::resolve<char>());
    ASSERT_EQ(any.cast<char>(), 'c');

    ASSERT_TRUE(any.allow_cast<double>());
    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), static_cast<double>('c'));

    any = 3.1;

    ASSERT_TRUE(any.allow_cast(entt::resolve<int>()));
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 3);

    ASSERT_TRUE(any.allow_cast<float>());
    ASSERT_EQ(any.type(), entt::resolve<float>());
    ASSERT_EQ(any.cast<float>(), 3.f);

    any = static_cast<float>('c');

    ASSERT_TRUE(any.allow_cast<char>());
    ASSERT_EQ(any.type(), entt::resolve<char>());
    ASSERT_EQ(any.cast<char>(), 'c');
}

TEST_F(MetaAny, EnumConversion) {
    entt::meta_any any{enum_class::foo};

    ASSERT_EQ(any.type(), entt::resolve<enum_class>());
    ASSERT_EQ(any.cast<enum_class>(), enum_class::foo);

    ASSERT_TRUE(any.allow_cast<double>());
    ASSERT_EQ(any.type(), entt::resolve<double>());
    ASSERT_EQ(any.cast<double>(), 0.);

    any = enum_class::bar;

    ASSERT_TRUE(any.allow_cast(entt::resolve<int>()));
    ASSERT_EQ(any.type(), entt::resolve<int>());
    ASSERT_EQ(any.cast<int>(), 1);

    ASSERT_TRUE(any.allow_cast<enum_class>());
    ASSERT_EQ(any.type(), entt::resolve<enum_class>());
    ASSERT_EQ(any.cast<enum_class>(), enum_class::bar);

    any = 0;

    ASSERT_TRUE(any.allow_cast(entt::resolve<enum_class>()));
    ASSERT_EQ(any.type(), entt::resolve<enum_class>());
    ASSERT_EQ(any.cast<enum_class>(), enum_class::foo);
}

TEST_F(MetaAny, UnmanageableType) {
    unmanageable instance{};
    auto any = entt::forward_as_meta(instance);
    entt::meta_any other = any.as_ref();

    std::swap(any, other);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);

    ASSERT_EQ(any.type(), entt::resolve<unmanageable>());
    ASSERT_NE(any.base().data(), nullptr);
    ASSERT_EQ(any.try_cast<int>(), nullptr);
    ASSERT_NE(any.try_cast<unmanageable>(), nullptr);

    ASSERT_TRUE(any.allow_cast<unmanageable>());
    ASSERT_FALSE(any.allow_cast<int>());

    ASSERT_TRUE(std::as_const(any).allow_cast<unmanageable>());
    ASSERT_FALSE(std::as_const(any).allow_cast<int>());
}

TEST_F(MetaAny, Invoke) {
    using namespace entt::literals;

    clazz instance;
    auto any = entt::forward_as_meta(instance);
    const auto result = any.invoke("func"_hs);

    ASSERT_TRUE(any.invoke("member"_hs, 3));
    ASSERT_FALSE(std::as_const(any).invoke("member"_hs, 3));
    ASSERT_FALSE(std::as_const(any).as_ref().invoke("member"_hs, 3));
    ASSERT_FALSE(any.invoke("non_existent"_hs, 3));

    ASSERT_TRUE(result);
    ASSERT_NE(result.try_cast<char>(), nullptr);
    ASSERT_EQ(result.cast<char>(), 'c');
    ASSERT_EQ(instance.value, 3);
}

TEST_F(MetaAny, SetGet) {
    using namespace entt::literals;

    clazz instance;
    auto any = entt::forward_as_meta(instance);

    ASSERT_TRUE(any.set("value"_hs, 3));

    const auto value = std::as_const(any).get("value"_hs);

    ASSERT_TRUE(value);
    ASSERT_EQ(value, any.get("value"_hs));
    ASSERT_EQ(value, std::as_const(any).as_ref().get("value"_hs));
    ASSERT_NE(value.try_cast<int>(), nullptr);
    ASSERT_EQ(value.cast<int>(), 3);
    ASSERT_EQ(instance.value, 3);

    ASSERT_FALSE(any.set("non_existent"_hs, 3));
    ASSERT_FALSE(any.get("non_existent"_hs));
}

TEST_F(MetaAny, ForwardAsMeta) {
    int value = 3;
    auto ref = entt::forward_as_meta(value);
    auto cref = entt::forward_as_meta(std::as_const(value));
    auto any = entt::forward_as_meta(static_cast<int &&>(value));

    ASSERT_TRUE(any);
    ASSERT_TRUE(ref);
    ASSERT_TRUE(cref);

    ASSERT_NE(any.try_cast<int>(), nullptr);
    ASSERT_NE(ref.try_cast<int>(), nullptr);
    ASSERT_EQ(cref.try_cast<int>(), nullptr);

    ASSERT_EQ(any.cast<const int &>(), 3);
    ASSERT_EQ(ref.cast<const int &>(), 3);
    ASSERT_EQ(cref.cast<const int &>(), 3);

    ASSERT_NE(any.base().data(), &value);
    ASSERT_EQ(ref.base().data(), &value);
}
