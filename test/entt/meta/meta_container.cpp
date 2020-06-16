#include <array>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

TEST(MetaContainer, Empty) {
    entt::meta_container container{};

    ASSERT_FALSE(container);

    entt::meta_any any{std::vector<int>{}};
    container = any.view();

    ASSERT_TRUE(container);
}

TEST(MetaContainer, DynamicSequenceContainer) {
    std::vector<int> vec{2, 3, 4};
    entt::meta_any any{std::ref(vec)};

    auto view = any.view();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.size(), 3u);

    auto first = view.begin();
    const auto last = view.end();

    ASSERT_FALSE(first == last);
    ASSERT_TRUE(first != last);

    ASSERT_NE(first, last);
    ASSERT_EQ((*(first++)).cast<int>(), 2);
    ASSERT_EQ((*(++first)).cast<int>(), 4);
    ASSERT_NE(first++, last);
    ASSERT_EQ(first, last);

    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);

    ASSERT_EQ((*view[std::size_t{1u}]).cast<int>(), 3);

    auto it = view.begin();

    ASSERT_TRUE(view.insert(it.handle(), 0));
    ASSERT_TRUE(view.insert((++it).handle(), 1));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ((*view.begin()).cast<int>(), 0);
    ASSERT_EQ((*++view.begin()).cast<int>(), 1);

    it = view.begin();

    ASSERT_TRUE(view.erase(it.handle()));
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ((*it).cast<int>(), 1);

    (*view[std::size_t{}]).cast<int>() = 5;

    ASSERT_EQ((*view.begin()).cast<int>(), 5);
}

TEST(MetaContainer, FixedSizeSequenceContainer) {
    std::array<int, 3> arr{2, 3, 4};
    entt::meta_any any{std::ref(arr)};

    auto view = any.view();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.size(), 3u);

    auto first = view.begin();
    const auto last = view.end();

    ASSERT_FALSE(first == last);
    ASSERT_TRUE(first != last);

    ASSERT_NE(first, last);
    ASSERT_EQ((*(first++)).cast<int>(), 2);
    ASSERT_EQ((*(++first)).cast<int>(), 4);
    ASSERT_NE(first++, last);
    ASSERT_EQ(first, last);

    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);

    ASSERT_EQ((*view[std::size_t{1u}]).cast<int>(), 3);

    auto it = view.begin();

    ASSERT_FALSE(view.insert(it.handle(), 0));
    ASSERT_FALSE(view.insert((++it).handle(), 1));

    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ((*view.begin()).cast<int>(), 2);
    ASSERT_EQ((*++view.begin()).cast<int>(), 3);

    it = view.begin();

    ASSERT_FALSE(view.erase(it.handle()));
    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ((*it).cast<int>(), 2);

    (*view[std::size_t{}]).cast<int>() = 5;

    ASSERT_EQ((*view.begin()).cast<int>(), 5);
}

TEST(MetaContainer, KeyValueAssociativeContainer) {
    std::map<int, char> map{{2, 'c'}, {3, 'd'}, {4, 'e'}};
    entt::meta_any any{std::ref(map)};

    auto view = any.view();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.size(), 3u);

    auto first = view.begin();
    const auto last = view.end();

    ASSERT_FALSE(first == last);
    ASSERT_TRUE(first != last);

    ASSERT_NE(first, last);
    ASSERT_EQ(((*(first++)).cast<std::pair<const int, char>>()), (std::pair<const int, char>{2, 'c'}));
    ASSERT_EQ(((*(++first++)).cast<std::pair<const int, char>>()), (std::pair<const int, char>{4, 'e'}));
    ASSERT_NE(first++, last);
    ASSERT_EQ(first, last);

    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);

    ASSERT_EQ(((*view[3]).cast<std::pair<const int, char>>()), (std::pair<const int, char>{3, 'd'}));

    ASSERT_TRUE(view.insert(0, 'a'));
    ASSERT_TRUE(view.insert(1, 'b'));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(((*view[0]).cast<std::pair<const int, char>>()), (std::pair<const int, char>{0, 'a'}));
    ASSERT_EQ(((*view[1]).cast<std::pair<const int, char>>()), (std::pair<const int, char>{1, 'b'}));

    ASSERT_TRUE(view.erase(0));
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view[0], view.end());

    (*view[1]).cast<std::pair<const int, char>>().second = 'f';

    ASSERT_EQ(((*view[1]).cast<std::pair<const int, char>>()), (std::pair<const int, char>{1, 'f'}));
}

TEST(MetaContainer, KeyOnlyAssociativeContainer) {
    std::set<int> set{2, 3, 4};
    entt::meta_any any{std::ref(set)};

    auto view = any.view();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.size(), 3u);

    auto first = view.begin();
    const auto last = view.end();

    ASSERT_FALSE(first == last);
    ASSERT_TRUE(first != last);

    ASSERT_NE(first, last);
    ASSERT_EQ((*(first++)).cast<int>(), 2);
    ASSERT_EQ((*(++first++)).cast<int>(), 4);
    ASSERT_NE(first++, last);
    ASSERT_EQ(first, last);

    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);

    ASSERT_NE(view[3], view.end());

    ASSERT_TRUE(view.insert(0, 'a'));
    ASSERT_TRUE(view.insert(1));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_NE(view[0], view.end());
    ASSERT_NE(view[1], view.end());

    ASSERT_TRUE(view.erase(0));
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view[0], view.end());
}
