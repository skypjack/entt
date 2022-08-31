#include <type_traits>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/locator/locator.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/range.hpp>
#include <entt/meta/resolve.hpp>

struct MetaRange: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<int>().type("int"_hs).data<42>("answer"_hs);
    }

    void TearDown() override {
        entt::meta_reset();
    }
};

TEST_F(MetaRange, EmptyRange) {
    entt::meta_reset();
    auto range = entt::resolve();
    ASSERT_EQ(range.begin(), range.end());
}

TEST_F(MetaRange, Iterator) {
    using namespace entt::literals;

    using iterator = typename decltype(entt::resolve())::iterator;

    static_assert(std::is_same_v<iterator::value_type, std::pair<entt::id_type, entt::meta_type>>);
    static_assert(std::is_same_v<iterator::pointer, entt::input_iterator_pointer<std::pair<entt::id_type, entt::meta_type>>>);
    static_assert(std::is_same_v<iterator::reference, std::pair<entt::id_type, entt::meta_type>>);

    auto range = entt::resolve();

    iterator end{range.begin()};
    iterator begin{};
    begin = range.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, range.begin());
    ASSERT_EQ(end, range.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin++, range.begin());
    ASSERT_EQ(begin--, range.end());

    ASSERT_EQ(begin + 1, range.end());
    ASSERT_EQ(end - 1, range.begin());

    ASSERT_EQ(++begin, range.end());
    ASSERT_EQ(--begin, range.begin());

    ASSERT_EQ(begin += 1, range.end());
    ASSERT_EQ(begin -= 1, range.begin());

    ASSERT_EQ(begin + (end - begin), range.end());
    ASSERT_EQ(begin - (begin - end), range.end());

    ASSERT_EQ(end - (end - begin), range.begin());
    ASSERT_EQ(end + (begin - end), range.begin());

    ASSERT_EQ(begin[0u].first, range.begin()->first);
    ASSERT_EQ(begin[0u].second, (*range.begin()).second);

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, range.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, range.end());

    entt::meta<double>().type("double"_hs);
    range = entt::resolve();
    begin = range.begin();

    ASSERT_EQ(begin[0u].first, entt::resolve<int>().info().hash());
    ASSERT_EQ(begin[1u].second, entt::resolve("double"_hs));
}

TEST_F(MetaRange, DirectValue) {
    using namespace entt::literals;

    auto type = entt::resolve<int>();
    auto range = type.data();

    ASSERT_NE(range.cbegin(), range.cend());

    for(auto &&[id, data]: range) {
        ASSERT_EQ(id, "answer"_hs);
        ASSERT_EQ(data.get({}).cast<int>(), 42);
    }
}
