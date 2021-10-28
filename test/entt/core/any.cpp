#include <algorithm>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/any.hpp>

namespace std {
template <typename T>
struct hash<std::reference_wrapper<T>> {
    size_t operator()(const std::reference_wrapper<T>& x) const {
        return std::hash<std::decay_t<T>>()(x);
    }
};
}

struct empty {
    ~empty() {
        ++counter;
    }

    inline static int counter = 0;
};

struct fat {
    fat(double v1, double v2, double v3, double v4)
        : value{v1, v2, v3, v4} {}

    ~fat() {
        ++counter;
    }

    bool operator==(const fat &other) const {
        return std::equal(std::begin(value), std::end(value), std::begin(other.value), std::end(other.value));
    }

    inline static int counter{0};
    double value[4];
};

struct not_comparable {
    bool operator==(const not_comparable &) const = delete;
};

struct not_hashable {};

struct not_copyable {
    not_copyable()
        : payload{} {}

    not_copyable(const not_copyable &) = delete;
    not_copyable(not_copyable &&) = default;

    not_copyable &operator=(const not_copyable &) = delete;
    not_copyable &operator=(not_copyable &&) = default;

    double payload;
};

struct not_movable {
    not_movable() = default;
    not_movable(const not_movable &) = default;
    not_movable(not_movable &&) = delete;

    not_movable &operator=(const not_movable &) = default;
    not_movable &operator=(not_movable &&) = delete;

    double payload;
};

struct alignas(64u) over_aligned {};

struct Any: ::testing::Test {
    void SetUp() override {
        fat::counter = 0;
        empty::counter = 0;
    }
};

TEST_F(Any, SBO) {
    entt::any any{'c'};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<char>(any), 'c');
}

TEST_F(Any, NoSBO) {
    fat instance{.1, .2, .3, .4};
    entt::any any{instance};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);
}

TEST_F(Any, Empty) {
    entt::any any{};

    ASSERT_FALSE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(any.data(), nullptr);
}

TEST_F(Any, SBOInPlaceTypeConstruction) {
    entt::any any{std::in_place_type<int>, 42};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.owner());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 42);
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, SBOAsRefConstruction) {
    int value = 42;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());

    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&any), &value);
    ASSERT_EQ(entt::any_cast<int>(&any), &value);
    ASSERT_EQ(entt::any_cast<const int>(&std::as_const(any)), &value);
    ASSERT_EQ(entt::any_cast<int>(&std::as_const(any)), &value);

    ASSERT_EQ(entt::any_cast<const int &>(any), 42);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_EQ(any.data(), &value);
    ASSERT_EQ(std::as_const(any).data(), &value);

    any.emplace<int &>(value);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(&any), &value);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.owner());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 42);
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, SBOAsConstRefConstruction) {
    const int value = 42;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());

    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&any), &value);
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&std::as_const(any)), &value);
    ASSERT_EQ(entt::any_cast<int>(&std::as_const(any)), &value);

    ASSERT_EQ(entt::any_cast<const int &>(any), 42);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(std::as_const(any).data(), &value);

    any.emplace<const int &>(value);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<const int>(&any), &value);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.owner());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<int>(other), 42);
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, SBOCopyConstruction) {
    entt::any any{42};
    entt::any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST_F(Any, SBOCopyAssignment) {
    entt::any any{42};
    entt::any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST_F(Any, SBOMoveConstruction) {
    entt::any any{42};
    entt::any other{std::move(any)};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST_F(Any, SBOMoveAssignment) {
    entt::any any{42};
    entt::any other{3};

    other = std::move(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_NE(any.data(), nullptr);
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(other.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<int>(other), 42);
}

TEST_F(Any, SBODirectAssignment) {
    entt::any any{};
    any = 42;

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 42);
}

TEST_F(Any, SBOAssignValue) {
    entt::any any{42};
    entt::any other{3};
    entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
}

TEST_F(Any, SBOAsRefAssignValue) {
    int value = 42;
    entt::any any{entt::forward_as_any(value)};
    entt::any other{3};
    entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_TRUE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(Any, SBOAsConstRefAssignValue) {
    const int value = 42;
    entt::any any{entt::forward_as_any(value)};
    entt::any other{3};
    entt::any invalid{'c'};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_FALSE(any.assign(other));
    ASSERT_FALSE(any.assign(invalid));
    ASSERT_EQ(entt::any_cast<int>(any), 42);
    ASSERT_EQ(value, 42);
}

TEST_F(Any, SBOTransferValue) {
    entt::any any{42};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_TRUE(any.assign(3));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
}

TEST_F(Any, SBOTransferConstValue) {
    const int value = 3;
    entt::any any{42};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_TRUE(any.assign(entt::forward_as_any(value)));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
}

TEST_F(Any, SBOAsRefTransferValue) {
    int value = 42;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_TRUE(any.assign(3));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<int>(any), 3);
    ASSERT_EQ(value, 3);
}

TEST_F(Any, SBOAsConstRefTransferValue) {
    const int value = 42;
    entt::any any{entt::forward_as_any(value)};

    ASSERT_TRUE(any);
    ASSERT_EQ(entt::any_cast<int>(any), 42);

    ASSERT_FALSE(any.assign(3));
    ASSERT_FALSE(any.assign('c'));
    ASSERT_EQ(entt::any_cast<int>(any), 42);
    ASSERT_EQ(value, 42);
}

TEST_F(Any, NoSBOInPlaceTypeConstruction) {
    fat instance{.1, .2, .3, .4};
    entt::any any{std::in_place_type<fat>, instance};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.owner());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, NoSBOAsRefConstruction) {
    fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.owner());
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
    ASSERT_FALSE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&any), &instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.owner());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, NoSBOAsConstRefConstruction) {
    const fat instance{.1, .2, .3, .4};
    entt::any any{entt::forward_as_any(instance)};

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.owner());
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
    ASSERT_FALSE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<const fat>(&any), &instance);

    auto other = any.as_ref();

    ASSERT_TRUE(other);
    ASSERT_FALSE(other.owner());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(other), (fat{.1, .2, .3, .4}));
    ASSERT_EQ(other.data(), any.data());
}

TEST_F(Any, NoSBOCopyConstruction) {
    fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{any};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBOCopyAssignment) {
    fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{3};

    other = any;

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBOMoveConstruction) {
    fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{std::move(any)};

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBOMoveAssignment) {
    fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{3};

    other = std::move(any);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.data(), nullptr);
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(other.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(other), instance);
}

TEST_F(Any, NoSBODirectAssignment) {
    fat instance{.1, .2, .3, .4};
    entt::any any{};
    any = instance;

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(any), instance);
}

TEST_F(Any, NoSBOAssignValue) {
    entt::any any{fat{.1, .2, .3, .4}};
    entt::any other{fat{.0, .1, .2, .3}};
    entt::any invalid{'c'};

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
    entt::any other{fat{.0, .1, .2, .3}};
    entt::any invalid{'c'};

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
    entt::any other{fat{.0, .1, .2, .3}};
    entt::any invalid{'c'};

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
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
}

TEST_F(Any, VoidCopyConstruction) {
    entt::any any{std::in_place_type<void>};
    entt::any other{any};

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, VoidCopyAssignment) {
    entt::any any{std::in_place_type<void>};
    entt::any other{42};

    other = any;

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, VoidMoveConstruction) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::move(any)};

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, VoidMoveAssignment) {
    entt::any any{std::in_place_type<void>};
    entt::any other{42};

    other = std::move(any);

    ASSERT_FALSE(any);
    ASSERT_FALSE(other);
    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(other.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
    ASSERT_EQ(other.type(), entt::type_id<void>());
    ASSERT_EQ(entt::any_cast<int>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&other), nullptr);
}

TEST_F(Any, SBOMoveValidButUnspecifiedState) {
    entt::any any{42};
    entt::any other{std::move(any)};
    entt::any valid = std::move(other);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(valid);
}

TEST_F(Any, NoSBOMoveValidButUnspecifiedState) {
    fat instance{.1, .2, .3, .4};
    entt::any any{instance};
    entt::any other{std::move(any)};
    entt::any valid = std::move(other);

    ASSERT_TRUE(any);
    ASSERT_TRUE(other);
    ASSERT_TRUE(valid);
}

TEST_F(Any, VoidMoveValidButUnspecifiedState) {
    entt::any any{std::in_place_type<void>};
    entt::any other{std::move(any)};
    entt::any valid = std::move(other);

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
    [[maybe_unused]] entt::any any{std::in_place_type<void>};
}

TEST_F(Any, Emplace) {
    entt::any any{};
    any.emplace<int>(42);

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());
    ASSERT_EQ(entt::any_cast<double>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int>(any), 42);
}

TEST_F(Any, EmplaceVoid) {
    entt::any any{};
    any.emplace<void>();

    ASSERT_FALSE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
}

TEST_F(Any, Reset) {
    entt::any any{42};

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());

    any.reset();

    ASSERT_FALSE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());

    int value = 42;
    any.emplace<int &>(value);

    ASSERT_TRUE(any);
    ASSERT_FALSE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<int>());

    any.reset();

    ASSERT_FALSE(any);
    ASSERT_TRUE(any.owner());
    ASSERT_EQ(any.type(), entt::type_id<void>());
}

TEST_F(Any, SBOSwap) {
    entt::any lhs{'c'};
    entt::any rhs{42};

    std::swap(lhs, rhs);

    ASSERT_TRUE(lhs.owner());
    ASSERT_TRUE(rhs.owner());

    ASSERT_EQ(lhs.type(), entt::type_id<int>());
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<int>(lhs), 42);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');
}

TEST_F(Any, NoSBOSwap) {
    entt::any lhs{fat{.1, .2, .3, .4}};
    entt::any rhs{fat{.4, .3, .2, .1}};

    std::swap(lhs, rhs);

    ASSERT_TRUE(lhs.owner());
    ASSERT_TRUE(rhs.owner());

    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{.4, .3, .2, .1}));
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{.1, .2, .3, .4}));
}

TEST_F(Any, VoidSwap) {
    entt::any lhs{std::in_place_type<void>};
    entt::any rhs{std::in_place_type<void>};
    const auto *pre = lhs.data();

    std::swap(lhs, rhs);

    ASSERT_TRUE(lhs.owner());
    ASSERT_TRUE(rhs.owner());

    ASSERT_EQ(pre, lhs.data());
}

TEST_F(Any, SBOWithNoSBOSwap) {
    entt::any lhs{fat{.1, .2, .3, .4}};
    entt::any rhs{'c'};

    std::swap(lhs, rhs);

    ASSERT_TRUE(lhs.owner());
    ASSERT_TRUE(rhs.owner());

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

    ASSERT_TRUE(lhs.owner());
    ASSERT_FALSE(rhs.owner());

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

    ASSERT_TRUE(lhs.owner());
    ASSERT_FALSE(rhs.owner());

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
    ASSERT_TRUE(lhs.owner());
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_TRUE(rhs.owner());
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
    ASSERT_TRUE(lhs.owner());
    ASSERT_EQ(rhs.type(), entt::type_id<char>());
    ASSERT_EQ(entt::any_cast<char>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<char>(rhs), 'c');

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_TRUE(rhs.owner());
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

    ASSERT_TRUE(lhs.owner());
    ASSERT_FALSE(rhs.owner());

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

    ASSERT_TRUE(lhs.owner());
    ASSERT_FALSE(rhs.owner());

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
    ASSERT_TRUE(lhs.owner());
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{.1, .2, .3, .4}));

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_TRUE(rhs.owner());
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
    ASSERT_TRUE(lhs.owner());
    ASSERT_EQ(rhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<fat>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<double>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(rhs), (fat{.1, .2, .3, .4}));

    std::swap(lhs, rhs);

    ASSERT_FALSE(rhs);
    ASSERT_TRUE(rhs.owner());
    ASSERT_EQ(lhs.type(), entt::type_id<fat>());
    ASSERT_EQ(entt::any_cast<double>(&lhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(&rhs), nullptr);
    ASSERT_EQ(entt::any_cast<fat>(lhs), (fat{.1, .2, .3, .4}));
}

TEST_F(Any, AsRef) {
    entt::any any{42};
    auto ref = any.as_ref();
    auto cref = std::as_const(any).as_ref();

    ASSERT_FALSE(ref.owner());
    ASSERT_FALSE(cref.owner());

    ASSERT_EQ(entt::any_cast<int>(&any), any.data());
    ASSERT_EQ(entt::any_cast<int>(&ref), any.data());
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);

    ASSERT_EQ(entt::any_cast<const int>(&any), any.data());
    ASSERT_EQ(entt::any_cast<const int>(&ref), any.data());
    ASSERT_EQ(entt::any_cast<const int>(&cref), any.data());

    ASSERT_EQ(entt::any_cast<int>(any), 42);
    ASSERT_EQ(entt::any_cast<int>(ref), 42);
    ASSERT_EQ(entt::any_cast<int>(cref), 42);

    ASSERT_EQ(entt::any_cast<const int>(any), 42);
    ASSERT_EQ(entt::any_cast<const int>(ref), 42);
    ASSERT_EQ(entt::any_cast<const int>(cref), 42);

    ASSERT_EQ(entt::any_cast<int &>(any), 42);
    ASSERT_EQ(entt::any_cast<const int &>(any), 42);
    ASSERT_EQ(entt::any_cast<int &>(ref), 42);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 42);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 42);

    entt::any_cast<int &>(any) = 3;

    ASSERT_EQ(entt::any_cast<int>(any), 3);
    ASSERT_EQ(entt::any_cast<int>(ref), 3);
    ASSERT_EQ(entt::any_cast<int>(cref), 3);

    std::swap(ref, cref);

    ASSERT_FALSE(ref.owner());
    ASSERT_FALSE(cref.owner());

    ASSERT_EQ(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), any.data());

    ref = ref.as_ref();
    cref = std::as_const(cref).as_ref();

    ASSERT_FALSE(ref.owner());
    ASSERT_FALSE(cref.owner());

    ASSERT_EQ(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);
    ASSERT_EQ(entt::any_cast<const int>(&ref), any.data());
    ASSERT_EQ(entt::any_cast<const int>(&cref), any.data());

    ASSERT_EQ(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);

    ASSERT_EQ(entt::any_cast<const int &>(ref), 3);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 3);

    ref = 42;
    cref = 42;

    ASSERT_TRUE(ref.owner());
    ASSERT_TRUE(cref.owner());

    ASSERT_NE(entt::any_cast<int>(&ref), nullptr);
    ASSERT_NE(entt::any_cast<int>(&cref), nullptr);
    ASSERT_EQ(entt::any_cast<int &>(ref), 42);
    ASSERT_EQ(entt::any_cast<int &>(cref), 42);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 42);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 42);
    ASSERT_NE(entt::any_cast<int>(&ref), any.data());
    ASSERT_NE(entt::any_cast<int>(&cref), any.data());
}

TEST_F(Any, Comparable) {
    auto test = [](entt::any any, entt::any other) {
        ASSERT_EQ(any, any);
        ASSERT_NE(other, any);
        ASSERT_NE(any, entt::any{});

        ASSERT_TRUE(any == any);
        ASSERT_FALSE(other == any);
        ASSERT_TRUE(any != other);
        ASSERT_TRUE(entt::any{} != any);
    };

    int value = 42;

    test('c', 'a');
    test(fat{.1, .2, .3, .4}, fat{.0, .1, .2, .3});
    test(entt::forward_as_any(value), 3);
    test(3, entt::make_any<const int &>(value));
    test('c', value);
}

TEST_F(Any, NotComparable) {
    auto test = [](const auto &instance) {
        auto any = entt::forward_as_any(instance);

        ASSERT_EQ(any, any);
        ASSERT_NE(any, entt::any{instance});
        ASSERT_NE(entt::any{}, any);

        ASSERT_TRUE(any == any);
        ASSERT_FALSE(any == entt::any{instance});
        ASSERT_TRUE(entt::any{} != any);
    };

    test(not_comparable{});
    test(std::unordered_map<int, not_comparable>{});
    test(std::vector<not_comparable>{});
}

TEST_F(Any, CompareVoid) {
    entt::any any{std::in_place_type<void>};

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

template <typename Type>
void test_hashable(const Type& value)
{
    const std::hash<Type> type_hasher;
    const std::hash<entt::any> any_hasher;
    const size_t hash = type_hasher(value);

    entt::any any{value};
    entt::any ref_any{std::ref(value)};
    entt::any cref_any{std::cref(value)};

    ASSERT_EQ(hash, any.hash());
    ASSERT_EQ(hash, ref_any.hash());
    ASSERT_EQ(hash, cref_any.hash());

    ASSERT_EQ(hash, any_hasher(any));
    ASSERT_EQ(hash, any_hasher(ref_any));
    ASSERT_EQ(hash, any_hasher(cref_any));
}

TEST_F(Any, Hashable) {
    test_hashable(42);
    test_hashable('a');
    test_hashable(std::string("hello world"));
}

TEST_F(Any, NotHashable) {
    ASSERT_EQ(entt::any(not_hashable{}).hash(), 0);
}

TEST_F(Any, AnyCast) {
    entt::any any{42};
    const auto &cany = any;

    ASSERT_EQ(entt::any_cast<char>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<char>(&cany), nullptr);
    ASSERT_EQ(*entt::any_cast<int>(&any), 42);
    ASSERT_EQ(*entt::any_cast<int>(&cany), 42);
    ASSERT_EQ(entt::any_cast<int &>(any), 42);
    ASSERT_DEATH(entt::any_cast<double &>(any), "");
    ASSERT_EQ(entt::any_cast<const int &>(cany), 42);
    ASSERT_DEATH(entt::any_cast<const double &>(cany), "");

    not_copyable instance{};
    instance.payload = 42.;
    entt::any ref{entt::forward_as_any(instance)};
    entt::any cref{entt::forward_as_any(std::as_const(instance).payload)};

    ASSERT_EQ(entt::any_cast<not_copyable>(std::move(ref)).payload, 42.);
    ASSERT_DEATH(entt::any_cast<not_copyable>(std::as_const(ref).as_ref()), "");
    ASSERT_EQ(entt::any_cast<double>(std::move(cref)), 42.);
    ASSERT_DEATH(entt::any_cast<double>(entt::any{42}), "");
    ASSERT_EQ(entt::any_cast<int>(entt::any{42}), 42);
}

TEST_F(Any, MakeAny) {
    int value = 42;
    auto any = entt::make_any<int>(value);
    auto ext = entt::make_any<int, sizeof(int), alignof(int)>(value);
    auto ref = entt::make_any<int &>(value);

    ASSERT_TRUE(any);
    ASSERT_TRUE(ext);
    ASSERT_TRUE(ref);

    ASSERT_TRUE(any.owner());
    ASSERT_TRUE(ext.owner());
    ASSERT_FALSE(ref.owner());

    ASSERT_EQ(entt::any_cast<const int &>(any), 42);
    ASSERT_EQ(entt::any_cast<const int &>(ext), 42);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 42);

    ASSERT_EQ(decltype(any)::length, entt::any::length);
    ASSERT_NE(decltype(ext)::length, entt::any::length);
    ASSERT_EQ(decltype(ref)::length, entt::any::length);

    ASSERT_NE(any.data(), &value);
    ASSERT_NE(ext.data(), &value);
    ASSERT_EQ(ref.data(), &value);
}

TEST_F(Any, ForwardAsAny) {
    int value = 42;
    auto any = entt::forward_as_any(std::move(value));
    auto ref = entt::forward_as_any(value);
    auto cref = entt::forward_as_any(std::as_const(value));

    ASSERT_TRUE(any);
    ASSERT_TRUE(ref);
    ASSERT_TRUE(cref);

    ASSERT_TRUE(any.owner());
    ASSERT_FALSE(ref.owner());
    ASSERT_FALSE(cref.owner());

    ASSERT_NE(entt::any_cast<int>(&any), nullptr);
    ASSERT_NE(entt::any_cast<int>(&ref), nullptr);
    ASSERT_EQ(entt::any_cast<int>(&cref), nullptr);

    ASSERT_EQ(entt::any_cast<const int &>(any), 42);
    ASSERT_EQ(entt::any_cast<const int &>(ref), 42);
    ASSERT_EQ(entt::any_cast<const int &>(cref), 42);

    ASSERT_NE(any.data(), &value);
    ASSERT_EQ(ref.data(), &value);
}

TEST_F(Any, NotCopyableType) {
    auto test = [](entt::any any, entt::any other) {
        ASSERT_TRUE(any);
        ASSERT_TRUE(other);

        ASSERT_TRUE(any.owner());
        ASSERT_FALSE(other.owner());
        ASSERT_EQ(any.type(), other.type());

        ASSERT_FALSE(any.assign(other));
        ASSERT_FALSE(any.assign(std::move(other)));

        entt::any copy{any};

        ASSERT_TRUE(any);
        ASSERT_FALSE(copy);

        ASSERT_TRUE(any.owner());
        ASSERT_TRUE(copy.owner());

        copy = any;

        ASSERT_TRUE(any);
        ASSERT_FALSE(copy);

        ASSERT_TRUE(any.owner());
        ASSERT_TRUE(copy.owner());
    };

    const not_copyable value{};
    test(entt::any{std::in_place_type<not_copyable>}, entt::forward_as_any(value));
}

TEST_F(Any, NotMovableType) {
    auto test = [](entt::any any, entt::any other) {
        ASSERT_TRUE(any);
        ASSERT_TRUE(other);

        ASSERT_TRUE(any.owner());
        ASSERT_TRUE(other.owner());
        ASSERT_EQ(any.type(), other.type());

        ASSERT_TRUE(any.assign(other));
        ASSERT_TRUE(any.assign(std::move(other)));

        entt::any copy{any};

        ASSERT_TRUE(any);
        ASSERT_TRUE(copy);

        ASSERT_TRUE(any.owner());
        ASSERT_TRUE(copy.owner());

        copy = any;

        ASSERT_TRUE(any);
        ASSERT_TRUE(copy);

        ASSERT_TRUE(any.owner());
        ASSERT_TRUE(copy.owner());
    };

    test(entt::any{std::in_place_type<not_movable>}, entt::any{std::in_place_type<not_movable>});
}

TEST_F(Any, Array) {
    entt::any any{std::in_place_type<int[1]>};
    entt::any copy{any};

    ASSERT_TRUE(any);
    ASSERT_FALSE(copy);

    ASSERT_EQ(any.type(), entt::type_id<int[1]>());
    ASSERT_NE(entt::any_cast<int[1]>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int[2]>(&any), nullptr);
    ASSERT_EQ(entt::any_cast<int *>(&any), nullptr);

    entt::any_cast<int(&)[1]>(any)[0] = 42;

    ASSERT_EQ(entt::any_cast<const int(&)[1]>(std::as_const(any))[0], 42);
}

TEST_F(Any, CopyMoveReference) {
    int value{};

    auto test = [&](auto &&ref) {
        value = 3;

        auto any = entt::forward_as_any(ref);
        entt::any move = std::move(any);
        entt::any copy = move;

        ASSERT_TRUE(any);
        ASSERT_TRUE(move);
        ASSERT_TRUE(copy);

        ASSERT_FALSE(any.owner());
        ASSERT_FALSE(move.owner());
        ASSERT_TRUE(copy.owner());

        ASSERT_EQ(move.type(), entt::type_id<int>());
        ASSERT_EQ(copy.type(), entt::type_id<int>());

        ASSERT_EQ(std::as_const(move).data(), &value);
        ASSERT_NE(std::as_const(copy).data(), &value);

        ASSERT_EQ(entt::any_cast<int>(move), 3);
        ASSERT_EQ(entt::any_cast<int>(copy), 3);

        value = 42;

        ASSERT_EQ(entt::any_cast<const int &>(move), 42);
        ASSERT_EQ(entt::any_cast<const int &>(copy), 3);
    };

    test(value);
    test(std::as_const(value));
}

TEST_F(Any, SBOVsZeroedSBOSize) {
    entt::any sbo{42};
    const auto *broken = sbo.data();
    entt::any other = std::move(sbo);

    ASSERT_NE(broken, other.data());

    entt::basic_any<0u> dyn{42};
    const auto *valid = dyn.data();
    entt::basic_any<0u> same = std::move(dyn);

    ASSERT_EQ(valid, same.data());
}

TEST_F(Any, Alignment) {
    static constexpr auto alignment = alignof(over_aligned);

    auto test = [](auto *target, auto cb) {
        const auto *data = target[0].data();

        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[0u].data()) % alignment) == 0u);
        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[1u].data()) % alignment) == 0u);

        std::swap(target[0], target[1]);

        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[0u].data()) % alignment) == 0u);
        ASSERT_TRUE((reinterpret_cast<std::uintptr_t>(target[1u].data()) % alignment) == 0u);

        cb(data, target[1].data());
    };

    entt::basic_any<alignment> nosbo[2] = {over_aligned{}, over_aligned{}};
    test(nosbo, [](auto *pre, auto *post) { ASSERT_EQ(pre, post); });

    entt::basic_any<alignment, alignment> sbo[2] = {over_aligned{}, over_aligned{}};
    test(sbo, [](auto *pre, auto *post) { ASSERT_NE(pre, post); });
}

TEST_F(Any, AggregatesMustWork) {
    struct aggregate_type {
        int value;
    };

    // the goal of this test is to enforce the requirements for aggregate types
    entt::any{std::in_place_type<aggregate_type>, 42}.emplace<aggregate_type>(42);
}

TEST_F(Any, DeducedArrayType) {
    entt::any any{"array of char"};

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<const char *>());
    ASSERT_EQ((strcmp("array of char", entt::any_cast<const char *>(any))), 0);

    any = "another array of char";

    ASSERT_TRUE(any);
    ASSERT_EQ(any.type(), entt::type_id<const char *>());
    ASSERT_EQ((strcmp("another array of char", entt::any_cast<const char *>(any))), 0);
}
