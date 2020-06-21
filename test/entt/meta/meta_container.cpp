#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

TEST(MetaSequenceContainer, Empty) {
    entt::meta_sequence_container container{};

    ASSERT_FALSE(container);

    entt::meta_any any{std::vector<int>{}};
    container = any.as_sequence_container();

    ASSERT_TRUE(container);
}

TEST(MetaSequenceContainer, StdVector) {
    std::vector<int> vec{};
    entt::meta_any any{std::ref(vec)};

    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());
    ASSERT_EQ(view.size(), 0u);

    ASSERT_TRUE(view.resize(3u));
    ASSERT_EQ(view.size(), 3u);

    view[0].cast<int>() = 2;
    view[1].cast<int>() = 3;
    view[2].cast<int>() = 4;

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

    ASSERT_EQ(view[1u].cast<int>(), 3);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_TRUE(ret.second);
    ASSERT_FALSE(view.insert(ret.first, 'c').second);
    ASSERT_TRUE(view.insert(++ret.first, 1).second);

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ((*view.begin()).cast<int>(), 0);
    ASSERT_EQ((*++view.begin()).cast<int>(), 1);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_TRUE(ret.second);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ((*ret.first).cast<int>(), 1);
}

TEST(MetaSequenceContainer, StdArray) {
    std::array<int, 3> arr{};
    entt::meta_any any{std::ref(arr)};

    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());
    ASSERT_EQ(view.size(), 3u);

    ASSERT_FALSE(view.resize(5u));
    ASSERT_EQ(view.size(), 3u);

    view[0].cast<int>() = 2;
    view[1].cast<int>() = 3;
    view[2].cast<int>() = 4;

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

    ASSERT_EQ(view[1u].cast<int>(), 3);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_FALSE(ret.second);
    ASSERT_FALSE(view.insert(ret.first, 'c').second);
    ASSERT_FALSE(view.insert(++ret.first, 1).second);

    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ((*view.begin()).cast<int>(), 2);
    ASSERT_EQ((*++view.begin()).cast<int>(), 3);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_FALSE(ret.second);
    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ((*it).cast<int>(), 2);
}

TEST(MetaAssociativeContainer, StdMap) {
    std::map<int, char> map{{2, 'c'}, {3, 'd'}, {4, 'e'}};
    entt::meta_any any{std::ref(map)};

    auto view = any.as_associative_container();

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

    ASSERT_EQ(((*view.find(3)).cast<std::pair<const int, char>>()), (std::pair<const int, char>{3, 'd'}));

    ASSERT_FALSE(view.insert('a', 'a'));
    ASSERT_FALSE(view.insert(1, 1));

    ASSERT_TRUE(view.insert(0, 'a'));
    ASSERT_TRUE(view.insert(1, 'b'));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(((*view.find(0)).cast<std::pair<const int, char>>()), (std::pair<const int, char>{0, 'a'}));
    ASSERT_EQ(((*view.find(1)).cast<std::pair<const int, char>>()), (std::pair<const int, char>{1, 'b'}));

    ASSERT_TRUE(view.erase(0));
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    (*view.find(1)).cast<std::pair<const int, char>>().second = 'f';

    ASSERT_EQ(((*view.find(1)).cast<std::pair<const int, char>>()), (std::pair<const int, char>{1, 'f'}));
}

TEST(MetaAssociativeContainer, StdSet) {
    std::set<int> set{2, 3, 4};
    entt::meta_any any{std::ref(set)};

    auto view = any.as_associative_container();

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

    ASSERT_EQ((*view.find(3)).cast<int>(), 3);

    ASSERT_FALSE(view.insert('0'));

    ASSERT_TRUE(view.insert(0));
    ASSERT_TRUE(view.insert(1));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ((*view.find(0)).cast<int>(), 0);
    ASSERT_EQ((*view.find(1)).cast<int>(), 1);

    ASSERT_TRUE(view.erase(0));
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    (*view.find(1)).cast<int>() = 42;

    ASSERT_EQ((*view.find(1)).cast<int>(), 1);
}
