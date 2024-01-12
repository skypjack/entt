#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/any.hpp>
#include <entt/core/type_info.hpp>
#include "../common/aggregate.h"
#include "../common/config.h"
#include "../common/linter.hpp"
#include "../common/non_comparable.h"
#include "../common/non_movable.h"

struct empty {
    empty() = default;

    empty(const empty &) = default;
    empty &operator=(const empty &) = delete;

    ~empty() {
        ++counter;
    }

    inline static int counter = 0; // NOLINT
};

struct fat {
    fat(double v1, double v2, double v3, double v4)
        : value{v1, v2, v3, v4} {}

    fat(const fat &) = default;
    fat &operator=(const fat &) = default;

    ~fat() {
        ++counter;
    }

    [[nodiscard]] bool operator==(const fat &other) const {
        return std::equal(std::begin(value), std::end(value), std::begin(other.value), std::end(other.value));
    }

    inline static int counter{0}; // NOLINT
    double value[4];              // NOLINT
};

struct alignas(64u) over_aligned {};

struct Any: ::testing::Test {
    void SetUp() override {
        fat::counter = 0;
        empty::counter = 0;
    }
};

using AnyDeathTest = Any;

TEST_F(Any, SBO) {
    entt::any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner()); // NOLINT
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<char>(any), 'c');
}

TEST_F(Any, NoSBO) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{instance};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner()); // NOLINT
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);
}

TEST_F(Any, Empty) {
    entt::any any{};

    ASSERT_FALSE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(any.data(), nullptr);
}

TEST_F(Any, SBOInPlaceTypeConstruction) {
    entt::any any{std::in_place_type<int>, 2};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::ref);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 2);
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, SBOAsRefConstruction) {
    int value = 2;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::ref);
    ASSERT_EQ(any.type(), entt::type_id<int>());

    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&any), &value);
    ASSERT_EQ(entt::any_cast<int>(&any), &value);
    ASSERT_EQ(entt::any_cast<const int>(&std::as_const(any)), &value);
    ASSERT_EQ(entt::any_cast<int>(&std::as_const(any)), &value);

    ASSERT_EQ(entt::any_cast<const int &>(any), 2);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_EQ(any.data(), &value);
    ASSERT_EQ(std::as_const(any).data(), &value);

    any.emplace<int &>(value);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::ref);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&any), &value);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::ref);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 2);
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, SBOAsConstRefConstruction) {
    const int value = 2;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::cref);
    ASSERT_EQ(any.type(), entt::type_id<int>());

    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&any), &value);
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&std::as_const(any)), &value);
    ASSERT_EQ(entt::any_cast<int>(&std::as_const(any)), &value);

    ASSERT_EQ(entt::any_cast<const int &>(any), 2);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(std::as_const(any).data(), &value);

    any.emplace<const int &>(value);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::cref);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<const int>(&any), &value);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::cref);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 2);
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, SBOCopyConstruction) {
    const entt::any any{2};
    entt::any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 2);
}

TEST_F(Any, SBOCopyAssignment) {
    const entt::any any{2};
    entt::any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 2);
}

TEST_F(Any, SBOMoveConstruction) {
    entt::any any{2};
    entt::any other{std::move(any)};

    test::is_initialized(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 2);
}

TEST_F(Any, SBOMoveAssignment) {
    entt::any any{2};
    entt::any other{3};

    other = std::move(any);
    test::is_initialized(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 2);
}

TEST_F(Any, SBODirectAssignment) {
    entt::any any{};
    any = 2;

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 2);
}

TEST_F(Any, SBOAssignValue) {
    entt::any any{2};
    const entt::any other{3};
    const entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
}

TEST_F(Any, SBOAsRefAssignValue) {
    int value = 2;
    entt::any any{entt::forward_as_any(value)};
    const entt::any other{3};
    const entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(Any, SBOAsConstRefAssignValue) {
    const int value = 2;
    entt::any any{entt::forward_as_any(value)};
    const entt::any other{3};
    const entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_FALSE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<int>(any), 2);
    ASSERT_EQ(value, 2);
}

TEST_F(Any, SBOTransferValue) {
    entt::any any{2};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_TRUE(any.assign(3));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
}

TEST_F(Any, SBOTransferConstValue) {
    const int value = 3;
    entt::any any{2};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_TRUE(any.assign(entt::forward_as_any(value)));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
}

TEST_F(Any, SBOAsRefTransferValue) {
    int value = 2;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_TRUE(any.assign(3));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(Any, SBOAsConstRefTransferValue) {
    const int value = 2;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 2);

    ASSERT_FALSE(any.assign(3));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<int>(any), 2);
    ASSERT_EQ(value, 2);
}

TEST_F(Any, NoSBOInPlaceTypeConstruction) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{std::in_place_type<fat>, instance};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::ref);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, NoSBOAsRefConstruction) {
    fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::ref);
    ASSERT_EQ(any.policy(), entt::any_policy::ref);
    ASSERT_EQ(any.type(), entt::type_id<fat>());

    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const fat>(&any), &instance);
    ASSERT_EQ(entt::any_cast<fat>(&any), &instance);
    ASSERT_EQ(entt::any_cast<const fat>(&std::as_const(any)), &instance);
    ASSERT_EQ(entt::any_cast<fat>(&std::as_const(any)), &instance);

    ASSERT_EQ(entt::any_cast<const fat &>(any), instance);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);

    ASSERT_EQ(any.data(), &instance);
    ASSERT_EQ(std::as_const(any).data(), &instance);

    any.emplace<fat &>(instance);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::ref);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&any), &instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::ref);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, NoSBOAsConstRefConstruction) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::cref);
    ASSERT_EQ(any.type(), entt::type_id<fat>());

    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const fat>(&any), &instance);
    ASSERT_EQ(entt::any_cast<fat>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const fat>(&std::as_const(any)), &instance);
    ASSERT_EQ(entt::any_cast<fat>(&std::as_const(any)), &instance);

    ASSERT_EQ(entt::any_cast<const fat &>(any), instance);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);

    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(std::as_const(any).data(), &instance);

    any.emplace<const fat &>(instance);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::cref);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<const fat>(&any), &instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::cref);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, NoSBOCopyConstruction) {
    const fat instance{.1, .2, .3, .4};
    const entt::any any{instance};
    entt::any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBOCopyAssignment) {
    const fat instance{.1, .2, .3, .4};
    const entt::any any{instance};
    entt::any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBOMoveConstruction) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{std::move(any)};

    test::is_initialized(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBOMoveAssignment) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{3};

    other = std::move(any);
    test::is_initialized(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBODirectAssignment) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{};
    any = instance;

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);
}

TEST_F(Any, NoSBOAssignValue) {
    entt::any any{fat{.1, .2, .3, .4}};
    const entt::any other{fat{.0, .1, .2, .3}};
    const entt::any invalid{'c'};

    const void *addr = std::as_const(any).data();

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, std::as_const(any).data());
}

TEST_F(Any, NoSBOAsRefAssignValue) {
    fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};
    const entt::any other{fat{.0, .1, .2, .3}};
    const entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(instance, (fat{.0, .1, .2, .3}));
}

TEST_F(Any, NoSBOAsConstRefAssignValue) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};
    const entt::any other{fat{.0, .1, .2, .3}};
    const entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));

    ASSERT_FALSE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(instance, (fat{.1, .2, .3, .4}));
}

TEST_F(Any, NoSBOTransferValue) {
    entt::any any{fat{.1, .2, .3, .4}};

    const void *addr = std::as_const(any).data();

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(fat{.0, .1, .2, .3}));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, std::as_const(any).data());
}

TEST_F(Any, NoSBOTransferConstValue) {
    const fat instance{.0, .1, .2, .3};
    entt::any any{fat{.1, .2, .3, .4}};

    const void *addr = std::as_const(any).data();

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(entt::forward_as_any(instance)));
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, std::as_const(any).data());
}

TEST_F(Any, NoSBOAsRefTransferValue) {
    fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};

    const void *addr = std::as_const(any).data();

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));

    ASSERT_TRUE(any.assign(fat{.0, .1, .2, .3}));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.0, .1, .2, .3}));
    ASSERT_EQ(instance, (fat{.0, .1, .2, .3}));
    ASSERT_EQ(addr, std::as_const(any).data());
}

TEST_F(Any, NoSBOAsConstRefTransferValue) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};

    const void *addr = std::as_const(any).data();

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));

    ASSERT_FALSE(any.assign(fat{.0, .1, .2, .3}));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<const fat &>(any), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(instance, (fat{.1, .2, .3, .4}));
    ASSERT_EQ(addr, std::as_const(any).data());
}

TEST_F(Any, VoidInPlaceTypeConstruction) {
    entt::any any{std::in_place_type<void>};

    ASSERT_FALSE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
}

TEST_F(Any, VoidCopyConstruction) {
    entt::any any{std::in_place_type<void>};
    entt::any other{any};

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, VoidCopyAssignment) {
    entt::any any{std::in_place_type<void>};
    entt::any other{2};

    other = any;

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, VoidMoveConstruction) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::move(any)};

    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, VoidMoveAssignment) {
    entt::any any{std::in_place_type<void>};
    entt::any other{2};

    other = std::move(any);
    test::is_initialized(any);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, SBOMoveValidButUnspecifiedState) {
    entt::any any{2};
    entt::any other{std::move(any)};
    const entt::any valid = std::move(other);

    test::is_initialized(any);
    test::is_initialized(other);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(valid);
}

TEST_F(Any, NoSBOMoveValidButUnspecifiedState) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{std::move(any)};
    const entt::any valid = std::move(other);

    test::is_initialized(any);
    test::is_initialized(other);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(valid);
}

TEST_F(Any, VoidMoveValidButUnspecifiedState) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::move(any)};
    const entt::any valid = std::move(other);

    test::is_initialized(any);
    test::is_initialized(other);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_FALSE(valid);
}

TEST_F(Any, SBODestruction) {
    {
        entt::any any{std::in_place_type<empty>};
        any.emplace<empty>();
        any = empty{};
        entt::any other{std::move(any)};
        any = std::move(other);
    }

    ASSERT_EQ(empty::counter, 6);
}

TEST_F(Any, NoSBODestruction) {
    {
        entt::any any{std::in_place_type<fat>, 1., 2., 3., 4.};
        any.emplace<fat>(1., 2., 3., 4.);
        any = fat{1., 2., 3., 4.};
        entt::any other{std::move(any)};
        any = std::move(other);
    }

    ASSERT_EQ(fat::counter, 4);
}

TEST_F(Any, VoidDestruction) {
    // just let asan tell us if everything is ok here
    [[maybe_unused]] const entt::any any{std::in_place_type<void>};
}

TEST_F(Any, Emplace) {
    entt::any any{};
    any.emplace<int>(2);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 2);
}

TEST_F(Any, EmplaceVoid) {
    entt::any any{};
    any.emplace<void>();

    ASSERT_FALSE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<void>());
}

TEST_F(Any, Reset) {
    entt::any any{2};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<int>());

    any.reset();

    ASSERT_FALSE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<void>());

    int value = 2;
    any.emplace<int &>(value);

    ASSERT_TRUE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::ref);
    ASSERT_EQ(any.type(), entt::type_id<int>());

    any.reset();

    ASSERT_FALSE(any);
    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), entt::type_id<void>());
}

TEST_F(Any, SBOSwap) {
    entt::any lhs{'c'};
    entt::any rhs{2};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::owner);

    ASSERT_EQ(lhs.type(), entt::type_id<int>());
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<int>(lhs), 2);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');
}

TEST_F(Any, NoSBOSwap) {
    entt::any lhs{fat{.1, .2, .3, .4}};
    entt::any rhs{fat{.4, .3, .2, .1}};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::owner);

    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{.4, .3, .2, .1}));
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{.1, .2, .3, .4}));
}

TEST_F(Any, VoidSwap) {
    entt::any lhs{std::in_place_type<void>};
    entt::any rhs{std::in_place_type<void>};
    const auto *pre = lhs.data();

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::owner);

    ASSERT_EQ(pre, lhs.data());
}

TEST_F(Any, SBOWithNoSBOSwap) {
    entt::any lhs{fat{.1, .2, .3, .4}};
    entt::any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::owner);

    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{.1, .2, .3, .4}));
}

TEST_F(Any, SBOWithRefSwap) {
    int value = 3;
    entt::any lhs{entt::forward_as_any(value)};
    entt::any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::ref);

    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(rhs.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
    ASSERT_EQ(entt::any_cast<int>(rhs), 3);
    ASSERT_EQ(rhs.data(), &value);
}

TEST_F(Any, SBOWithConstRefSwap) {
    const int value = 3;
    entt::any lhs{entt::forward_as_any(value)};
    entt::any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::cref);

    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(rhs.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
    ASSERT_EQ(entt::any_cast<int>(rhs), 3);
    ASSERT_EQ(rhs.data(), nullptr);
    ASSERT_EQ(std::as_const(rhs).data(), &value);
}

TEST_F(Any, SBOWithEmptySwap) {
    entt::any lhs{'c'};
    entt::any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
}

TEST_F(Any, SBOWithVoidSwap) {
    entt::any lhs{'c'};
    entt::any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(lhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(lhs), 'c');
}

TEST_F(Any, NoSBOWithRefSwap) {
    int value = 3;
    entt::any lhs{entt::forward_as_any(value)};
    entt::any rhs{fat{.1, .2, .3, .4}};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::ref);

    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(rhs.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(entt::any_cast<int>(rhs), 3);
    ASSERT_EQ(rhs.data(), &value);
}

TEST_F(Any, NoSBOWithConstRefSwap) {
    const int value = 3;
    entt::any lhs{entt::forward_as_any(value)};
    entt::any rhs{fat{.1, .2, .3, .4}};

    std::swap(lhs, rhs);

    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.policy(), entt::any_policy::cref);

    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(rhs.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(entt::any_cast<int>(rhs), 3);
    ASSERT_EQ(rhs.data(), nullptr);
    ASSERT_EQ(std::as_const(rhs).data(), &value);
}

TEST_F(Any, NoSBOWithEmptySwap) {
    entt::any lhs{fat{.1, .2, .3, .4}};
    entt::any rhs{};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{.1, .2, .3, .4}));

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{.1, .2, .3, .4}));
}

TEST_F(Any, NoSBOWithVoidSwap) {
    entt::any lhs{fat{.1, .2, .3, .4}};
    entt::any rhs{std::in_place_type<void>};

    std::swap(lhs, rhs);

    ASSERT_FALSE(lhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{.1, .2, .3, .4}));

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_EQ(lhs.policy(), entt::any_policy::owner);
    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{.1, .2, .3, .4}));
}

TEST_F(Any, AsRef) {
    entt::any any{2};
    auto ref = any.as_ref();
    auto cref = std::as_const(any).as_ref();

    ASSERT_EQ(ref.policy(), entt::any_policy::ref);
    ASSERT_EQ(cref.policy(), entt::any_policy::cref);

    ASSERT_EQ(entt::any_cast<int>(&any), any.data());
    ASSERT_EQ(entt::any_cast<int>(&ref), any.data());
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);

    ASSERT_EQ(entt::any_cast<const int>(&any), any.data());
    ASSERT_EQ(entt::any_cast<const int>(&ref), any.data());
    ASSERT_EQ(entt::any_cast<const int>(&cref), any.data());

    ASSERT_EQ(entt::any_cast<int>(any), 2);
    ASSERT_EQ(entt::any_cast<int>(ref), 2);
    ASSERT_EQ(entt::any_cast<int>(cref), 2);

    ASSERT_EQ(entt::any_cast<const int>(any), 2);
    ASSERT_EQ(entt::any_cast<const int>(ref), 2);
    ASSERT_EQ(entt::any_cast<const int>(cref), 2);

    ASSERT_EQ(entt::any_cast<int &>(any), 2);
    ASSERT_EQ(entt::any_cast<const int &>(any), 2);
    ASSERT_EQ(entt::any_cast<int &>(ref), 2);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 2);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 2);

    entt::any_cast<int &>(any) = 3;

    ASSERT_EQ(entt::any_cast<int>(any), 3);
    ASSERT_EQ(entt::any_cast<int>(ref), 3);
    ASSERT_EQ(entt::any_cast<int>(cref), 3);

    std::swap(ref, cref);

    ASSERT_EQ(ref.policy(), entt::any_policy::cref);
    ASSERT_EQ(cref.policy(), entt::any_policy::ref);

    ASSERT_EQ(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), any.data());

    ref = ref.as_ref();
    cref = std::as_const(cref).as_ref();

    ASSERT_EQ(ref.policy(), entt::any_policy::cref);
    ASSERT_EQ(cref.policy(), entt::any_policy::cref);

    ASSERT_EQ(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&ref), any.data());
    ASSERT_EQ(entt::any_cast<const int>(&cref), any.data());

    ASSERT_EQ(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);

    ASSERT_EQ(entt::any_cast<const int &>(ref), 3);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 3);

    ref = 2;
    cref = 2;

    ASSERT_EQ(ref.policy(), entt::any_policy::owner);
    ASSERT_EQ(cref.policy(), entt::any_policy::owner);

    ASSERT_NE(entt::any_cast<int>(&ref), nullptr);
    ASSERT_NE(entt::any_cast<int>(&cref), nullptr);
    ASSERT_EQ(entt::any_cast<int &>(ref), 2);
    ASSERT_EQ(entt::any_cast<int &>(cref), 2);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 2);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 2);
    ASSERT_NE(entt::any_cast<int>(&ref), any.data());
    ASSERT_NE(entt::any_cast<int>(&cref), any.data());
}

TEST_F(Any, Comparable) {
    const entt::any any{'c'};
    const entt::any other{'a'};

    ASSERT_EQ(any, any);
    ASSERT_NE(other, any);
    ASSERT_NE(any, entt::any{});

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(other == any);
    ASSERT_TRUE(any != other);
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, NoSBOComparable) {
    const entt::any any{fat{.1, .2, .3, .4}};
    const entt::any other{fat{.0, .1, .2, .3}};

    ASSERT_EQ(any, any);
    ASSERT_NE(other, any);
    ASSERT_NE(any, entt::any{});

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(other == any);
    ASSERT_TRUE(any != other);
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, RefComparable) {
    int value = 2;
    const entt::any any{entt::forward_as_any(value)};
    const entt::any other{3};

    ASSERT_EQ(any, any);
    ASSERT_NE(other, any);
    ASSERT_NE(any, entt::any{});

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(other == any);
    ASSERT_TRUE(any != other);
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, ConstRefComparable) {
    int value = 2;
    const entt::any any{3};
    const entt::any other{entt::make_any<const int &>(value)};

    ASSERT_EQ(any, any);
    ASSERT_NE(other, any);
    ASSERT_NE(any, entt::any{});

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(other == any);
    ASSERT_TRUE(any != other);
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, UnrelatedComparable) {
    const entt::any any{'c'};
    const entt::any other{2};

    ASSERT_EQ(any, any);
    ASSERT_NE(other, any);
    ASSERT_NE(any, entt::any{});

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(other == any);
    ASSERT_TRUE(any != other);
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, NonComparable) {
    const test::non_comparable instance{};
    auto any = entt::forward_as_any(instance);

    ASSERT_EQ(any, any);
    ASSERT_NE(any, entt::any{instance});
    ASSERT_NE(entt::any{}, any);

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(any == entt::any{instance});
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, AssociativeContainerOfNonComparable) {
    const std::unordered_map<int, test::non_comparable> instance{};
    auto any = entt::forward_as_any(instance);

    ASSERT_EQ(any, any);
    ASSERT_NE(any, entt::any{instance});
    ASSERT_NE(entt::any{}, any);

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(any == entt::any{instance});
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, SequenceContainerOfNonComparable) {
    const std::vector<test::non_comparable> instance{};
    auto any = entt::forward_as_any(instance);

    ASSERT_EQ(any, any);
    ASSERT_NE(any, entt::any{instance});
    ASSERT_NE(entt::any{}, any);

    ASSERT_TRUE(any == any);
    ASSERT_FALSE(any == entt::any{instance});
    ASSERT_TRUE(entt::any{} != any);
}

TEST_F(Any, CompareVoid) {
    const entt::any any{std::in_place_type<void>};

    ASSERT_EQ(any, any);
    ASSERT_EQ(any, entt::any{std::in_place_type<void>});
    ASSERT_NE(entt::any{'a'}, any);
    ASSERT_EQ(any, entt::any{});

    ASSERT_TRUE(any == any);
    ASSERT_TRUE(any == entt::any{std::in_place_type<void>});
    ASSERT_FALSE(entt::any{'a'} == any);
    ASSERT_TRUE(any != entt::any{'a'});
    ASSERT_FALSE(entt::any{} != any);
}

TEST_F(Any, AnyCast) {
    entt::any any{2};
    const auto &cany = any;

    ASSERT_EQ(entt::any_cast<char>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&cany), nullptr);
    ASSERT_EQ(*entt::any_cast<int>(&any), 2);
    ASSERT_EQ(*entt::any_cast<int>(&cany), 2);
    ASSERT_EQ(entt::any_cast<int &>(any), 2);
    ASSERT_EQ(entt::any_cast<const int &>(cany), 2);

    auto instance = std::make_unique<double>(2.);
    entt::any ref{entt::forward_as_any(instance)};
    entt::any cref{entt::forward_as_any(std::as_const(*instance))};

    ASSERT_EQ(entt::any_cast<double>(std::move(cref)), 2.);
    ASSERT_EQ(*entt::any_cast<std::unique_ptr<double>>(std::move(ref)), 2.);
    ASSERT_EQ(entt::any_cast<int>(entt::any{2}), 2);
}

ENTT_DEBUG_TEST_F(AnyDeathTest, AnyCast) {
    entt::any any{2};
    const auto &cany = any;

    ASSERT_DEATH([[maybe_unused]] auto &elem = entt::any_cast<double &>(any), "");
    ASSERT_DEATH([[maybe_unused]] const auto &elem = entt::any_cast<const double &>(cany), "");

    auto instance = std::make_unique<double>(2.);
    entt::any ref{entt::forward_as_any(instance)};
    const entt::any cref{entt::forward_as_any(std::as_const(*instance))};

    ASSERT_DEATH([[maybe_unused]] auto elem = entt::any_cast<std::unique_ptr<double>>(std::as_const(ref).as_ref()), "");
    ASSERT_DEATH([[maybe_unused]] auto elem = entt::any_cast<double>(entt::any{2}), "");
}

TEST_F(Any, MakeAny) {
    int value = 2;
    auto any = entt::make_any<int>(value);
    auto ext = entt::make_any<int, sizeof(int), alignof(int)>(value);
    auto ref = entt::make_any<int &>(value);

    ASSERT_TRUE(any);
    ASSERT_TRUE(ext);
    ASSERT_TRUE(ref);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(ext.policy(), entt::any_policy::owner);
    ASSERT_EQ(ref.policy(), entt::any_policy::ref);

    ASSERT_EQ(entt::any_cast<const int &>(any), 2);
    ASSERT_EQ(entt::any_cast<const int &>(ext), 2);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 2);

    ASSERT_EQ(decltype(any)::length, entt::any::length);
    ASSERT_NE(decltype(ext)::length, entt::any::length);
    ASSERT_EQ(decltype(ref)::length, entt::any::length);

    ASSERT_NE(any.data(), &value);
    ASSERT_NE(ext.data(), &value);
    ASSERT_EQ(ref.data(), &value);
}

TEST_F(Any, ForwardAsAny) {
    int value = 2;
    auto ref = entt::forward_as_any(value);
    auto cref = entt::forward_as_any(std::as_const(value));
    auto any = entt::forward_as_any(static_cast<int &&>(value));

    ASSERT_TRUE(any);
    ASSERT_TRUE(ref);
    ASSERT_TRUE(cref);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(ref.policy(), entt::any_policy::ref);
    ASSERT_EQ(cref.policy(), entt::any_policy::cref);

    ASSERT_NE(entt::any_cast<int>(&any), nullptr);
    ASSERT_NE(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);

    ASSERT_EQ(entt::any_cast<const int &>(any), 2);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 2);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 2);

    ASSERT_NE(any.data(), &value);
    ASSERT_EQ(ref.data(), &value);
}

TEST_F(Any, NonCopyableType) {
    const std::unique_ptr<int> value{};
    entt::any any{std::in_place_type<std::unique_ptr<int>>};
    entt::any other = entt::forward_as_any(value);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.policy(), entt::any_policy::cref);
    ASSERT_EQ(any.type(), other.type());

    ASSERT_FALSE(any.assign(other));
    ASSERT_FALSE(any.assign(std::move(other)));

    entt::any copy{any};

    ASSERT_TRUE(any);
    ASSERT_FALSE(copy);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(copy.policy(), entt::any_policy::owner);

    copy = any;

    ASSERT_TRUE(any);
    ASSERT_FALSE(copy);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(copy.policy(), entt::any_policy::owner);
}

TEST_F(Any, NonCopyableValueType) {
    std::vector<entt::any> vec{};
    vec.emplace_back(std::in_place_type<std::unique_ptr<int>>);
    vec.shrink_to_fit();

    ASSERT_EQ(vec.size(), 1u);
    ASSERT_EQ(vec.capacity(), 1u);
    ASSERT_TRUE(vec[0u]);

    // strong exception guarantee due to noexcept move ctor
    vec.emplace_back(std::in_place_type<std::unique_ptr<int>>);

    ASSERT_EQ(vec.size(), 2u);
    ASSERT_TRUE(vec[0u]);
    ASSERT_TRUE(vec[1u]);
}

TEST_F(Any, NonMovableType) {
    entt::any any{std::in_place_type<test::non_movable>};
    entt::any other{std::in_place_type<test::non_movable>};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(other.policy(), entt::any_policy::owner);
    ASSERT_EQ(any.type(), other.type());

    ASSERT_TRUE(any.assign(other));
    ASSERT_TRUE(any.assign(std::move(other)));

    entt::any copy{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(copy);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(copy.policy(), entt::any_policy::owner);

    copy = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(copy);

    ASSERT_EQ(any.policy(), entt::any_policy::owner);
    ASSERT_EQ(copy.policy(), entt::any_policy::owner);
}

TEST_F(Any, Array) {
    entt::any any{std::in_place_type<int[1]>}; // NOLINT
    const entt::any copy{any};

    ASSERT_TRUE(any);
    ASSERT_FALSE(copy);

    ASSERT_EQ(any.type(), entt::type_id<int[1]>());   // NOLINT
    ASSERT_NE(entt::any_cast<int[1]>(&any), nullptr); // NOLINT
    ASSERT_EQ(entt::any_cast<int[2]>(&any), nullptr); // NOLINT
    ASSERT_EQ(entt::any_cast<int *>(&any), nullptr);

    entt::any_cast<int(&)[1]>(any)[0] = 2; // NOLINT

    ASSERT_EQ(entt::any_cast<const int(&)[1]>(std::as_const(any))[0], 2); // NOLINT
}

TEST_F(Any, CopyMoveReference) {
    int value = 3;
    auto any = entt::forward_as_any(value);
    entt::any move = std::move(any);
    entt::any copy = move;

    test::is_initialized(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(move);
    ASSERT_TRUE(copy);

    ASSERT_EQ(move.policy(), entt::any_policy::ref);
    ASSERT_EQ(copy.policy(), entt::any_policy::owner);

    ASSERT_EQ(move.type(), entt::type_id<int>());
    ASSERT_EQ(copy.type(), entt::type_id<int>());

    ASSERT_EQ(std::as_const(move).data(), &value);
    ASSERT_NE(std::as_const(copy).data(), &value);

    ASSERT_EQ(entt::any_cast<int>(move), 3);
    ASSERT_EQ(entt::any_cast<int>(copy), 3);

    value = 2;

    ASSERT_EQ(entt::any_cast<int &>(move), 2);
    ASSERT_EQ(entt::any_cast<int &>(copy), 3);
}

TEST_F(Any, CopyMoveConstReference) {
    int value = 3;
    auto any = entt::forward_as_any(std::as_const(value));
    entt::any move = std::move(any);
    entt::any copy = move;

    test::is_initialized(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(move);
    ASSERT_TRUE(copy);

    ASSERT_EQ(move.policy(), entt::any_policy::cref);
    ASSERT_EQ(copy.policy(), entt::any_policy::owner);

    ASSERT_EQ(move.type(), entt::type_id<int>());
    ASSERT_EQ(copy.type(), entt::type_id<int>());

    ASSERT_EQ(std::as_const(move).data(), &value);
    ASSERT_NE(std::as_const(copy).data(), &value);

    ASSERT_EQ(entt::any_cast<int>(move), 3);
    ASSERT_EQ(entt::any_cast<int>(copy), 3);

    value = 2;

    ASSERT_EQ(entt::any_cast<const int &>(move), 2);
    ASSERT_EQ(entt::any_cast<const int &>(copy), 3);
}

TEST_F(Any, SBOVsZeroedSBOSize) {
    entt::any sbo{2};
    const auto *broken = sbo.data();
    entt::any other = std::move(sbo);

    ASSERT_NE(broken, other.data());

    entt::basic_any<0u> dyn{2};
    const auto *valid = dyn.data();
    entt::basic_any<0u> same = std::move(dyn);

    ASSERT_EQ(valid, same.data());
}

TEST_F(Any, SboAlignment) {
    constexpr auto alignment = alignof(over_aligned);
    entt::basic_any<alignment, alignment> sbo[2] = {over_aligned{}, over_aligned{}}; // NOLINT
    const auto *data = sbo[0].data();

    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[0u].data()) % alignment) == 0u); // NOLINT
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[1u].data()) % alignment) == 0u); // NOLINT

    std::swap(sbo[0], sbo[1]);

    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[0u].data()) % alignment) == 0u); // NOLINT
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(sbo[1u].data()) % alignment) == 0u); // NOLINT

    ASSERT_NE(data, sbo[1].data());
}

TEST_F(Any, NoSboAlignment) {
    constexpr auto alignment = alignof(over_aligned);
    entt::basic_any<alignment> nosbo[2] = {over_aligned{}, over_aligned{}}; // NOLINT
    const auto *data = nosbo[0].data();

    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[0u].data()) % alignment) == 0u); // NOLINT
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[1u].data()) % alignment) == 0u); // NOLINT

    std::swap(nosbo[0], nosbo[1]);

    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[0u].data()) % alignment) == 0u); // NOLINT
    ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(nosbo[1u].data()) % alignment) == 0u); // NOLINT

    ASSERT_EQ(data, nosbo[1].data());
}

TEST_F(Any, AggregatesMustWork) {
    // the goal of this test is to enforce the requirements for aggregate types
    entt::any{std::in_place_type<test::aggregate>, 2}.emplace<test::aggregate>(2);
}

TEST_F(Any, DeducedArrayType) {
    entt::any any{"array of char"};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<const char *>());
    ASSERT_EQ((std::strcmp("array of char", entt::any_cast<const char *>(any))), 0);

    any = "another array of char";

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<const char *>());
    ASSERT_EQ((std::strcmp("another array of char", entt::any_cast<const char *>(any))), 0);
}
