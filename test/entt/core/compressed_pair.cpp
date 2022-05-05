#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/compressed_pair.hpp>

struct empty_type {};

struct move_only_type {
    move_only_type()
        : value{new int{99}} {}

    move_only_type(int v)
        : value{new int{v}} {}

    ~move_only_type() {
        delete value;
    }

    move_only_type(const move_only_type &) = delete;
    move_only_type &operator=(const move_only_type &) = delete;

    move_only_type(move_only_type &&other) noexcept
        : value{std::exchange(other.value, nullptr)} {}

    move_only_type &operator=(move_only_type &&other) noexcept {
        delete value;
        value = std::exchange(other.value, nullptr);
        return *this;
    }

    int *value;
};

struct non_default_constructible {
    non_default_constructible(int v)
        : value{v} {}

    int value;
};

TEST(CompressedPair, Size) {
    struct local {
        int value;
        empty_type empty;
    };

    static_assert(sizeof(entt::compressed_pair<int, int>) == sizeof(int[2u]));
    static_assert(sizeof(entt::compressed_pair<empty_type, int>) == sizeof(int));
    static_assert(sizeof(entt::compressed_pair<int, empty_type>) == sizeof(int));
    static_assert(sizeof(entt::compressed_pair<int, empty_type>) < sizeof(local));
    static_assert(sizeof(entt::compressed_pair<int, empty_type>) < sizeof(std::pair<int, empty_type>));
}

TEST(CompressedPair, ConstructCopyMove) {
    static_assert(!std::is_default_constructible_v<entt::compressed_pair<non_default_constructible, empty_type>>);
    static_assert(std::is_default_constructible_v<entt::compressed_pair<move_only_type, empty_type>>);

    static_assert(std::is_copy_constructible_v<entt::compressed_pair<non_default_constructible, empty_type>>);
    static_assert(!std::is_copy_constructible_v<entt::compressed_pair<move_only_type, empty_type>>);
    static_assert(std::is_copy_assignable_v<entt::compressed_pair<non_default_constructible, empty_type>>);
    static_assert(!std::is_copy_assignable_v<entt::compressed_pair<move_only_type, empty_type>>);

    static_assert(std::is_move_constructible_v<entt::compressed_pair<move_only_type, empty_type>>);
    static_assert(std::is_move_assignable_v<entt::compressed_pair<move_only_type, empty_type>>);

    entt::compressed_pair copyable{non_default_constructible{42}, empty_type{}};
    auto by_copy{copyable};

    ASSERT_EQ(by_copy.first().value, 42);

    by_copy.first().value = 3;
    copyable = by_copy;

    ASSERT_EQ(copyable.first().value, 3);

    entt::compressed_pair<empty_type, move_only_type> movable{};
    auto by_move{std::move(movable)};

    ASSERT_EQ(*by_move.second().value, 99);
    ASSERT_EQ(movable.second().value, nullptr);

    *by_move.second().value = 3;
    movable = std::move(by_move);

    ASSERT_EQ(*movable.second().value, 3);
    ASSERT_EQ(by_move.second().value, nullptr);
}

TEST(CompressedPair, PiecewiseConstruct) {
    std::vector<int> vec{42};
    entt::compressed_pair<empty_type, empty_type> empty{std::piecewise_construct, std::make_tuple(), std::make_tuple()};
    entt::compressed_pair<std::vector<int>, std::size_t> pair{std::piecewise_construct, std::forward_as_tuple(std::move(vec)), std::make_tuple(sizeof(empty))};

    ASSERT_EQ(pair.first().size(), 1u);
    ASSERT_EQ(pair.second(), sizeof(empty));
    ASSERT_EQ(vec.size(), 0u);
}

TEST(CompressedPair, DeductionGuide) {
    int value = 42;
    empty_type empty{};
    entt::compressed_pair pair{value, 3};

    static_assert(std::is_same_v<decltype(entt::compressed_pair{empty_type{}, empty}), entt::compressed_pair<empty_type, empty_type>>);

    ASSERT_TRUE((std::is_same_v<decltype(pair), entt::compressed_pair<int, int>>));
    ASSERT_EQ(pair.first(), 42);
    ASSERT_EQ(pair.second(), 3);
}

TEST(CompressedPair, Getters) {
    entt::compressed_pair pair{3, empty_type{}};
    const auto &cpair = pair;

    static_assert(std::is_same_v<decltype(pair.first()), int &>);
    static_assert(std::is_same_v<decltype(pair.second()), empty_type &>);

    static_assert(std::is_same_v<decltype(cpair.first()), const int &>);
    static_assert(std::is_same_v<decltype(cpair.second()), const empty_type &>);

    ASSERT_EQ(pair.first(), cpair.first());
    ASSERT_EQ(&pair.second(), &cpair.second());
}

TEST(CompressedPair, Swap) {
    entt::compressed_pair pair{1, 2};
    entt::compressed_pair other{3, 4};

    swap(pair, other);

    ASSERT_EQ(pair.first(), 3);
    ASSERT_EQ(pair.second(), 4);
    ASSERT_EQ(other.first(), 1);
    ASSERT_EQ(other.second(), 2);

    pair.swap(other);

    ASSERT_EQ(pair.first(), 1);
    ASSERT_EQ(pair.second(), 2);
    ASSERT_EQ(other.first(), 3);
    ASSERT_EQ(other.second(), 4);
}

TEST(CompressedPair, Get) {
    entt::compressed_pair pair{1, 2};

    ASSERT_EQ(pair.get<0>(), 1);
    ASSERT_EQ(pair.get<1>(), 2);

    ASSERT_EQ(&pair.get<0>(), &pair.first());
    ASSERT_EQ(&pair.get<1>(), &pair.second());

    auto &&[first, second] = pair;

    ASSERT_EQ(first, 1);
    ASSERT_EQ(second, 2);

    first = 3;
    second = 4;

    ASSERT_EQ(pair.first(), 3);
    ASSERT_EQ(pair.second(), 4);

    auto &[cfirst, csecond] = std::as_const(pair);

    ASSERT_EQ(cfirst, 3);
    ASSERT_EQ(csecond, 4);

    static_assert(std::is_same_v<decltype(cfirst), const int>);
    static_assert(std::is_same_v<decltype(csecond), const int>);

    auto [tfirst, tsecond] = entt::compressed_pair{9, 99};

    ASSERT_EQ(tfirst, 9);
    ASSERT_EQ(tsecond, 99);

    static_assert(std::is_same_v<decltype(cfirst), const int>);
    static_assert(std::is_same_v<decltype(csecond), const int>);
}
