#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/core/type_traits.hpp>
#include "../../common/non_comparable.h"

struct nlohmann_json_like final {
    using value_type = nlohmann_json_like;

    bool operator==(const nlohmann_json_like &) const {
        return true;
    }
};

struct clazz {
    char foo(int value) {
        return static_cast<char>(quux = (value != 0));
    }

    [[nodiscard]] int bar(double, float) const {
        return static_cast<int>(quux);
    }

    bool quux;
};

int free_function(int, const double &) {
    return 64;
}

template<typename, typename Type = void>
struct multi_argument_operation {
    using type = Type;
};

struct UnpackAsType: ::testing::Test {
    template<typename Type, typename... Args>
    static auto test_for() {
        return [](entt::unpack_as_type<Type, Args>... value) {
            return (value + ... + Type{});
        };
    }
};

struct UnpackAsValue: ::testing::Test {
    template<auto Value>
    static auto test_for() {
        return [](auto &&...args) {
            return (entt::unpack_as_value<Value, decltype(args)> + ... + 0);
        };
    }
};

TEST(SizeOf, Functionalities) {
    ASSERT_EQ(entt::size_of_v<void>, 0u);
    ASSERT_EQ(entt::size_of_v<char>, sizeof(char));
    // NOLINTBEGIN(*-avoid-c-arrays)
    ASSERT_EQ(entt::size_of_v<int[]>, 0u);
    ASSERT_EQ(entt::size_of_v<int[3]>, sizeof(int[3]));
    // NOLINTEND(*-avoid-c-arrays)
}

TEST_F(UnpackAsType, Functionalities) {
    ASSERT_EQ((this->test_for<int, char, double, bool>()(1, 2, 3)), 6);
    ASSERT_EQ((this->test_for<float, void, int>()(2.f, 2.2f)), 4.2f);
}

TEST_F(UnpackAsValue, Functionalities) {
    ASSERT_EQ((this->test_for<2>()('c', 1., true)), 6);
    ASSERT_EQ((this->test_for<true>()('c', 2.)), 2);
}

TEST(IntegralConstant, Functionalities) {
    const entt::integral_constant<3> constant{};

    testing::StaticAssertTypeEq<typename entt::integral_constant<3>::value_type, int>();
    ASSERT_EQ(constant.value, 3);
}

TEST(Choice, Functionalities) {
    static_assert(std::is_base_of_v<entt::choice_t<0>, entt::choice_t<1>>, "Base type required");
    static_assert(!std::is_base_of_v<entt::choice_t<1>, entt::choice_t<0>>, "Base type not allowed");
}

TEST(TypeList, Functionalities) {
    using type = entt::type_list<int, char>;
    using other = entt::type_list<double>;

    ASSERT_EQ(type::size, 2u);
    ASSERT_EQ(other::size, 1u);

    testing::StaticAssertTypeEq<decltype(type{} + other{}), entt::type_list<int, char, double>>();
    testing::StaticAssertTypeEq<entt::type_list_cat_t<type, other, type, other>, entt::type_list<int, char, double, int, char, double>>();
    testing::StaticAssertTypeEq<entt::type_list_cat_t<type, other>, entt::type_list<int, char, double>>();
    testing::StaticAssertTypeEq<entt::type_list_cat_t<type, type>, entt::type_list<int, char, int, char>>();
    testing::StaticAssertTypeEq<entt::type_list_unique_t<entt::type_list_cat_t<type, type>>, type>();

    ASSERT_TRUE((entt::type_list_contains_v<type, int>));
    ASSERT_TRUE((entt::type_list_contains_v<type, char>));
    ASSERT_FALSE((entt::type_list_contains_v<type, double>));

    testing::StaticAssertTypeEq<entt::type_list_element_t<0u, type>, int>();
    testing::StaticAssertTypeEq<entt::type_list_element_t<1u, type>, char>();
    testing::StaticAssertTypeEq<entt::type_list_element_t<0u, other>, double>();

    ASSERT_EQ((entt::type_list_index_v<int, type>), 0u);
    ASSERT_EQ((entt::type_list_index_v<char, type>), 1u);
    ASSERT_EQ((entt::type_list_index_v<double, other>), 0u);

    testing::StaticAssertTypeEq<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<float, bool>>, entt::type_list<int, char, double>>();
    testing::StaticAssertTypeEq<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<int, char, double>>, entt::type_list<>>();
    testing::StaticAssertTypeEq<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<int, char>>, entt::type_list<double>>();
    testing::StaticAssertTypeEq<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<char, double>>, entt::type_list<int>>();
    testing::StaticAssertTypeEq<entt::type_list_diff_t<entt::type_list<int, char, double>, entt::type_list<char>>, entt::type_list<int, double>>();

    testing::StaticAssertTypeEq<entt::type_list_transform_t<entt::type_list<int, char>, entt::type_identity>, entt::type_list<int, char>>();
    testing::StaticAssertTypeEq<entt::type_list_transform_t<entt::type_list<int, char>, std::add_const>, entt::type_list<const int, const char>>();
    testing::StaticAssertTypeEq<entt::type_list_transform_t<entt::type_list<int, char>, multi_argument_operation>, entt::type_list<void, void>>();

    ASSERT_EQ(std::tuple_size_v<entt::type_list<>>, 0u);
    ASSERT_EQ(std::tuple_size_v<entt::type_list<int>>, 1u);
    ASSERT_EQ((std::tuple_size_v<entt::type_list<int, float>>), 2u);

    testing::StaticAssertTypeEq<int, std::tuple_element_t<0, entt::type_list<int>>>();
    testing::StaticAssertTypeEq<int, std::tuple_element_t<0, entt::type_list<int, float>>>();
    testing::StaticAssertTypeEq<float, std::tuple_element_t<1, entt::type_list<int, float>>>();
}

TEST(ValueList, Functionalities) {
    using value = entt::value_list<0, 2>;
    using other = entt::value_list<1>;

    ASSERT_EQ(value::size, 2u);
    ASSERT_EQ(other::size, 1u);

    testing::StaticAssertTypeEq<decltype(value{} + other{}), entt::value_list<0, 2, 1>>();
    testing::StaticAssertTypeEq<entt::value_list_cat_t<value, other, value, other>, entt::value_list<0, 2, 1, 0, 2, 1>>();
    testing::StaticAssertTypeEq<entt::value_list_cat_t<value, other>, entt::value_list<0, 2, 1>>();
    testing::StaticAssertTypeEq<entt::value_list_cat_t<value, value>, entt::value_list<0, 2, 0, 2>>();
    testing::StaticAssertTypeEq<entt::value_list_unique_t<entt::value_list_cat_t<value, value>>, value>();

    ASSERT_TRUE((entt::value_list_contains_v<value, 0>));
    ASSERT_TRUE((entt::value_list_contains_v<value, 2>));
    ASSERT_FALSE((entt::value_list_contains_v<value, 1>));

    ASSERT_EQ((entt::value_list_element_v<0u, value>), 0);
    ASSERT_EQ((entt::value_list_element_v<1u, value>), 2);
    ASSERT_EQ((entt::value_list_element_v<0u, other>), 1);

    ASSERT_EQ((entt::value_list_index_v<0, value>), 0u);
    ASSERT_EQ((entt::value_list_index_v<2, value>), 1u);
    ASSERT_EQ((entt::value_list_index_v<1, other>), 0u);

    testing::StaticAssertTypeEq<entt::value_list_diff_t<entt::value_list<0, 1, 2>, entt::value_list<3, 4>>, entt::value_list<0, 1, 2>>();
    testing::StaticAssertTypeEq<entt::value_list_diff_t<entt::value_list<0, 1, 2>, entt::value_list<0, 1, 2>>, entt::value_list<>>();
    testing::StaticAssertTypeEq<entt::value_list_diff_t<entt::value_list<0, 1, 2>, entt::value_list<0, 1>>, entt::value_list<2>>();
    testing::StaticAssertTypeEq<entt::value_list_diff_t<entt::value_list<0, 1, 2>, entt::value_list<1, 2>>, entt::value_list<0>>();
    testing::StaticAssertTypeEq<entt::value_list_diff_t<entt::value_list<0, 1, 2>, entt::value_list<1>>, entt::value_list<0, 2>>();

    ASSERT_EQ((std::tuple_size_v<entt::value_list<>>), 0u);
    ASSERT_EQ((std::tuple_size_v<entt::value_list<4>>), 1u);
    ASSERT_EQ((std::tuple_size_v<entt::value_list<4, 'a'>>), 2u);

    testing::StaticAssertTypeEq<int, std::tuple_element_t<0, entt::value_list<4>>>();
    testing::StaticAssertTypeEq<int, std::tuple_element_t<0, entt::value_list<4, 'a'>>>();
    testing::StaticAssertTypeEq<char, std::tuple_element_t<1, entt::value_list<4, 'a'>>>();
}

TEST(IsApplicable, Functionalities) {
    ASSERT_TRUE((entt::is_applicable_v<void(int, char), std::tuple<double, char>>));
    ASSERT_FALSE((entt::is_applicable_v<void(int, char), std::tuple<int>>));

    ASSERT_TRUE((entt::is_applicable_r_v<float, int(int, char), std::tuple<double, char>>));
    ASSERT_FALSE((entt::is_applicable_r_v<float, void(int, char), std::tuple<double, char>>));
    ASSERT_FALSE((entt::is_applicable_r_v<int, int(int, char), std::tuple<void>>));
}

TEST(IsComplete, Functionalities) {
    ASSERT_FALSE(entt::is_complete_v<void>);
    ASSERT_TRUE(entt::is_complete_v<int>);
}

TEST(IsIterator, Functionalities) {
    ASSERT_FALSE(entt::is_iterator_v<void>);
    ASSERT_FALSE(entt::is_iterator_v<int>);

    ASSERT_FALSE(entt::is_iterator_v<void *>);
    ASSERT_TRUE(entt::is_iterator_v<int *>);

    ASSERT_TRUE(entt::is_iterator_v<std::vector<int>::iterator>);
    ASSERT_TRUE(entt::is_iterator_v<std::vector<int>::const_iterator>);
    ASSERT_TRUE(entt::is_iterator_v<std::vector<int>::reverse_iterator>);
}

TEST(IsEBCOEligible, Functionalities) {
    ASSERT_TRUE(entt::is_ebco_eligible_v<test::non_comparable>);
    ASSERT_FALSE(entt::is_ebco_eligible_v<nlohmann_json_like>);
    ASSERT_FALSE(entt::is_ebco_eligible_v<double>);
    ASSERT_FALSE(entt::is_ebco_eligible_v<void>);
}

TEST(IsTransparent, Functionalities) {
    ASSERT_FALSE(entt::is_transparent_v<std::less<int>>);
    ASSERT_TRUE(entt::is_transparent_v<std::less<void>>);
    ASSERT_FALSE(entt::is_transparent_v<std::logical_not<double>>);
    ASSERT_TRUE(entt::is_transparent_v<std::logical_not<void>>);
}

TEST(IsEqualityComparable, Functionalities) {
    ASSERT_TRUE(entt::is_equality_comparable_v<int>);
    ASSERT_TRUE(entt::is_equality_comparable_v<const int>);
    ASSERT_TRUE(entt::is_equality_comparable_v<std::vector<int>>);
    ASSERT_TRUE(entt::is_equality_comparable_v<std::vector<std::vector<int>>>);
    ASSERT_TRUE((entt::is_equality_comparable_v<std::unordered_map<int, int>>));
    ASSERT_TRUE((entt::is_equality_comparable_v<std::unordered_map<int, std::unordered_map<int, char>>>));
    ASSERT_TRUE((entt::is_equality_comparable_v<std::pair<const int, int>>));
    ASSERT_TRUE((entt::is_equality_comparable_v<std::pair<const int, std::unordered_map<int, char>>>));
    ASSERT_TRUE(entt::is_equality_comparable_v<std::vector<test::non_comparable>::iterator>);
    ASSERT_TRUE((entt::is_equality_comparable_v<std::optional<int>>));
    ASSERT_TRUE(entt::is_equality_comparable_v<nlohmann_json_like>);

    // NOLINTNEXTLINE(*-avoid-c-arrays)
    ASSERT_FALSE(entt::is_equality_comparable_v<int[3u]>);
    ASSERT_FALSE(entt::is_equality_comparable_v<test::non_comparable>);
    ASSERT_FALSE(entt::is_equality_comparable_v<const test::non_comparable>);
    ASSERT_FALSE(entt::is_equality_comparable_v<std::vector<test::non_comparable>>);
    ASSERT_FALSE(entt::is_equality_comparable_v<std::vector<std::vector<test::non_comparable>>>);
    ASSERT_FALSE((entt::is_equality_comparable_v<std::unordered_map<int, test::non_comparable>>));
    ASSERT_FALSE((entt::is_equality_comparable_v<std::unordered_map<int, std::unordered_map<int, test::non_comparable>>>));
    ASSERT_FALSE((entt::is_equality_comparable_v<std::pair<const int, test::non_comparable>>));
    ASSERT_FALSE((entt::is_equality_comparable_v<std::pair<const int, std::unordered_map<int, test::non_comparable>>>));
    ASSERT_FALSE((entt::is_equality_comparable_v<std::optional<test::non_comparable>>));
    ASSERT_FALSE(entt::is_equality_comparable_v<void>);
}

TEST(ConstnessAs, Functionalities) {
    testing::StaticAssertTypeEq<entt::constness_as_t<int, char>, int>();
    testing::StaticAssertTypeEq<entt::constness_as_t<const int, char>, int>();
    testing::StaticAssertTypeEq<entt::constness_as_t<int, const char>, const int>();
    testing::StaticAssertTypeEq<entt::constness_as_t<const int, const char>, const int>();
}

TEST(MemberClass, Functionalities) {
    testing::StaticAssertTypeEq<clazz, entt::member_class_t<decltype(&clazz::foo)>>();
    testing::StaticAssertTypeEq<clazz, entt::member_class_t<decltype(&clazz::bar)>>();
    testing::StaticAssertTypeEq<clazz, entt::member_class_t<decltype(&clazz::quux)>>();
}

TEST(NthArgument, Functionalities) {
    testing::StaticAssertTypeEq<entt::nth_argument_t<0u, void(int, char, bool)>, int>();
    testing::StaticAssertTypeEq<entt::nth_argument_t<1u, void(int, char, bool)>, char>();
    testing::StaticAssertTypeEq<entt::nth_argument_t<2u, void(int, char, bool)>, bool>();

    testing::StaticAssertTypeEq<entt::nth_argument_t<0u, decltype(&free_function)>, int>();
    testing::StaticAssertTypeEq<entt::nth_argument_t<1u, decltype(&free_function)>, const double &>();

    testing::StaticAssertTypeEq<entt::nth_argument_t<0u, decltype(&clazz::bar)>, double>();
    testing::StaticAssertTypeEq<entt::nth_argument_t<1u, decltype(&clazz::bar)>, float>();
    testing::StaticAssertTypeEq<entt::nth_argument_t<0u, decltype(&clazz::quux)>, bool>();

    ASSERT_EQ(free_function(entt::nth_argument_t<0u, decltype(&free_function)>{}, entt::nth_argument_t<1u, decltype(&free_function)>{}), 64);

    [[maybe_unused]] auto lambda = [value = 0u](int, float &) { return value; };

    testing::StaticAssertTypeEq<entt::nth_argument_t<0u, decltype(lambda)>, int>();
    testing::StaticAssertTypeEq<entt::nth_argument_t<1u, decltype(lambda)>, float &>();
}

TEST(Tag, Functionalities) {
    using namespace entt::literals;
    ASSERT_EQ(entt::tag<"foobar"_hs>::value, entt::hashed_string::value("foobar"));
    testing::StaticAssertTypeEq<typename entt::tag<"foobar"_hs>::value_type, entt::id_type>();
}
