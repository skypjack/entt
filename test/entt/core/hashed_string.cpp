#include <cstdint>
#include <string_view>
#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>

struct BasicHashedString: ::testing::Test {
    static constexpr auto expected = std::conditional_t<
        std::is_same_v<entt::id_type, std::uint32_t>,
        std::integral_constant<std::uint32_t, 0xbf9cf968>,
        std::integral_constant<std::uint64_t, 0x85944171f73967e8>>::value;
};

using HashedString = BasicHashedString;
using HashedWString = BasicHashedString;

TEST_F(BasicHashedString, DeductionGuide) {
    testing::StaticAssertTypeEq<decltype(entt::basic_hashed_string{"foo"}), entt::hashed_string>();
    testing::StaticAssertTypeEq<decltype(entt::basic_hashed_string{L"foo"}), entt::hashed_wstring>();
}

TEST_F(HashedString, Functionalities) {
    using namespace entt::literals;
    using hash_type = entt::hashed_string::hash_type;

    const char *bar = "bar";

    auto foo_hs = entt::hashed_string{"foo"};
    auto bar_hs = entt::hashed_string{bar};

    ASSERT_NE(static_cast<hash_type>(foo_hs), static_cast<hash_type>(bar_hs));
    ASSERT_STREQ(static_cast<const char *>(foo_hs), "foo");
    ASSERT_STREQ(static_cast<const char *>(bar_hs), bar);
    ASSERT_STREQ(foo_hs.data(), "foo");
    ASSERT_STREQ(bar_hs.data(), bar);
    ASSERT_EQ(foo_hs.size(), 3u);
    ASSERT_EQ(bar_hs.size(), 3u);

    ASSERT_EQ(foo_hs, foo_hs);
    ASSERT_NE(foo_hs, bar_hs);

    const entt::hashed_string hs{"foobar"};

    ASSERT_EQ(static_cast<hash_type>(hs), expected);
    ASSERT_EQ(hs.value(), expected);

    ASSERT_EQ(foo_hs, "foo"_hs);
    ASSERT_NE(bar_hs, "foo"_hs);

    entt::hashed_string empty_hs{};

    ASSERT_EQ(empty_hs, entt::hashed_string{});
    ASSERT_NE(empty_hs, foo_hs);

    empty_hs = foo_hs;

    ASSERT_NE(empty_hs, entt::hashed_string{});
    ASSERT_EQ(empty_hs, foo_hs);
}

TEST_F(HashedString, Empty) {
    using hash_type = entt::hashed_string::hash_type;

    const entt::hashed_string hs{};

    ASSERT_EQ(hs.size(), 0u);
    ASSERT_EQ(static_cast<hash_type>(hs), entt::internal::fnv_1a_params<>::offset);
    ASSERT_EQ(static_cast<const char *>(hs), nullptr);
}

TEST_F(HashedString, Correctness) {
    const char *foobar = "foobar";
    const std::string_view view{"foobar__", 6};

    ASSERT_EQ(entt::hashed_string{foobar}, expected);
    ASSERT_EQ((entt::hashed_string{view.data(), view.size()}), expected);
    ASSERT_EQ(entt::hashed_string{"foobar"}, expected);

    ASSERT_EQ(entt::hashed_string::value(foobar), expected);
    ASSERT_EQ(entt::hashed_string::value(view.data(), view.size()), expected);
    ASSERT_EQ(entt::hashed_string::value("foobar"), expected);

    ASSERT_EQ(entt::hashed_string{foobar}.size(), 6u);
    ASSERT_EQ((entt::hashed_string{view.data(), view.size()}).size(), 6u);
    ASSERT_EQ(entt::hashed_string{"foobar"}.size(), 6u);
}

TEST_F(HashedString, Order) {
    using namespace entt::literals;
    const entt::hashed_string lhs = "foo"_hs;
    const entt::hashed_string rhs = "bar"_hs;

    ASSERT_FALSE(lhs < lhs);
    ASSERT_FALSE(rhs < rhs);

    ASSERT_LT(rhs, lhs);
    ASSERT_LE(rhs, lhs);

    ASSERT_GT(lhs, rhs);
    ASSERT_GE(lhs, rhs);
}

TEST_F(HashedString, Constexprness) {
    using namespace entt::literals;
    constexpr std::string_view view{"foobar__", 6};

    ASSERT_EQ(entt::hashed_string{"quux"}, "quux"_hs);
    ASSERT_EQ(entt::hashed_string{"foobar"}, expected);

    ASSERT_EQ(entt::hashed_string::value("quux"), "quux"_hs);
    ASSERT_EQ(entt::hashed_string::value("foobar"), expected);

    ASSERT_EQ((entt::hashed_string{"quux", 4}), "quux"_hs);
    ASSERT_EQ((entt::hashed_string{view.data(), view.size()}), expected);

    ASSERT_EQ((entt::hashed_string::value("quux", 4)), "quux"_hs);
    ASSERT_EQ((entt::hashed_string::value(view.data(), view.size())), expected);

    ASSERT_LT(entt::hashed_string{"bar"}, "foo"_hs);
    ASSERT_LE(entt::hashed_string{"bar"}, "bar"_hs);

    ASSERT_GT(entt::hashed_string{"foo"}, "bar"_hs);
    ASSERT_GE(entt::hashed_string{"foo"}, "foo"_hs);
}

TEST_F(HashedWString, Functionalities) {
    using namespace entt::literals;
    using hash_type = entt::hashed_wstring::hash_type;

    const wchar_t *bar = L"bar";

    auto foo_hws = entt::hashed_wstring{L"foo"};
    auto bar_hws = entt::hashed_wstring{bar};

    ASSERT_NE(static_cast<hash_type>(foo_hws), static_cast<hash_type>(bar_hws));
    ASSERT_STREQ(static_cast<const wchar_t *>(foo_hws), L"foo");
    ASSERT_STREQ(static_cast<const wchar_t *>(bar_hws), bar);
    ASSERT_STREQ(foo_hws.data(), L"foo");
    ASSERT_STREQ(bar_hws.data(), bar);
    ASSERT_EQ(foo_hws.size(), 3u);
    ASSERT_EQ(bar_hws.size(), 3u);

    ASSERT_EQ(foo_hws, foo_hws);
    ASSERT_NE(foo_hws, bar_hws);

    const entt::hashed_wstring hws{L"foobar"};

    ASSERT_EQ(static_cast<hash_type>(hws), expected);
    ASSERT_EQ(hws.value(), expected);

    ASSERT_EQ(foo_hws, L"foo"_hws);
    ASSERT_NE(bar_hws, L"foo"_hws);
}

TEST_F(HashedWString, Empty) {
    using hash_type = entt::hashed_wstring::hash_type;

    const entt::hashed_wstring hws{};

    ASSERT_EQ(hws.size(), 0u);
    ASSERT_EQ(static_cast<hash_type>(hws), entt::internal::fnv_1a_params<>::offset);
    ASSERT_EQ(static_cast<const wchar_t *>(hws), nullptr);
}

TEST_F(HashedWString, Correctness) {
    const wchar_t *foobar = L"foobar";
    const std::wstring_view view{L"foobar__", 6};

    ASSERT_EQ(entt::hashed_wstring{foobar}, expected);
    ASSERT_EQ((entt::hashed_wstring{view.data(), view.size()}), expected);
    ASSERT_EQ(entt::hashed_wstring{L"foobar"}, expected);

    ASSERT_EQ(entt::hashed_wstring::value(foobar), expected);
    ASSERT_EQ(entt::hashed_wstring::value(view.data(), view.size()), expected);
    ASSERT_EQ(entt::hashed_wstring::value(L"foobar"), expected);

    ASSERT_EQ(entt::hashed_wstring{foobar}.size(), 6u);
    ASSERT_EQ((entt::hashed_wstring{view.data(), view.size()}).size(), 6u);
    ASSERT_EQ(entt::hashed_wstring{L"foobar"}.size(), 6u);
}

TEST_F(HashedWString, Order) {
    using namespace entt::literals;
    const entt::hashed_wstring lhs = L"foo"_hws;
    const entt::hashed_wstring rhs = L"bar"_hws;

    ASSERT_FALSE(lhs < lhs);
    ASSERT_FALSE(rhs < rhs);

    ASSERT_LT(rhs, lhs);
    ASSERT_LE(rhs, lhs);

    ASSERT_GT(lhs, rhs);
    ASSERT_GE(lhs, rhs);
}

TEST_F(HashedWString, Constexprness) {
    using namespace entt::literals;
    constexpr std::wstring_view view{L"foobar__", 6};

    ASSERT_EQ(entt::hashed_wstring{L"quux"}, L"quux"_hws);
    ASSERT_EQ(entt::hashed_wstring{L"foobar"}, expected);

    ASSERT_EQ(entt::hashed_wstring::value(L"quux"), L"quux"_hws);
    ASSERT_EQ(entt::hashed_wstring::value(L"foobar"), expected);

    ASSERT_EQ((entt::hashed_wstring{L"quux", 4}), L"quux"_hws);
    ASSERT_EQ((entt::hashed_wstring{view.data(), view.size()}), expected);

    ASSERT_EQ((entt::hashed_wstring::value(L"quux", 4)), L"quux"_hws);
    ASSERT_EQ((entt::hashed_wstring::value(view.data(), view.size())), expected);

    ASSERT_LT(entt::hashed_wstring{L"bar"}, L"foo"_hws);
    ASSERT_LE(entt::hashed_wstring{L"bar"}, L"bar"_hws);

    ASSERT_GT(entt::hashed_wstring{L"foo"}, L"bar"_hws);
    ASSERT_GE(entt::hashed_wstring{L"foo"}, L"foo"_hws);
}
