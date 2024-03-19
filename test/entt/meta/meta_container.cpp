#include <array>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <entt/container/dense_map.hpp>
#include <entt/container/dense_set.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>
#include "../../common/config.h"
#include "../../common/empty.h"
#include "../../common/non_default_constructible.h"

TEST(MetaContainer, Invalid) {
    ASSERT_FALSE(entt::meta_any{0}.as_sequence_container());
    ASSERT_FALSE(entt::meta_any{0}.as_associative_container());

    ASSERT_FALSE((entt::meta_any{std::map<int, char>{}}.as_sequence_container()));
    ASSERT_FALSE(entt::meta_any{std::vector<int>{}}.as_associative_container());
}

TEST(SequenceContainer, Empty) {
    entt::meta_sequence_container container{};

    ASSERT_FALSE(container);

    entt::meta_any any{std::vector<int>{}};
    container = any.as_sequence_container();

    ASSERT_TRUE(container);
}

TEST(SequenceContainer, Iterator) {
    std::vector<int> vec{2, 3, 4};
    auto any = entt::forward_as_meta(vec);
    entt::meta_sequence_container::iterator first{};
    auto view = any.as_sequence_container();

    ASSERT_FALSE(first);

    first = view.begin();
    const auto last = view.end();

    ASSERT_TRUE(first);
    ASSERT_TRUE(last);

    ASSERT_FALSE(first == last);
    ASSERT_TRUE(first != last);

    ASSERT_EQ((first++)->cast<int>(), 2);
    ASSERT_EQ((++first)->cast<int>(), 4);

    ASSERT_NE(first++, last);
    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);
    ASSERT_EQ(first--, last);

    ASSERT_EQ((first--)->cast<int>(), 4);
    ASSERT_EQ((--first)->cast<int>(), 2);
}

TEST(SequenceContainer, StdVector) {
    std::vector<int> vec{};
    auto any = entt::forward_as_meta(vec);
    auto view = any.as_sequence_container();
    auto cview = std::as_const(any).as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.resize(3u));
    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    view[0].cast<int &>() = 2;
    view[1].cast<int &>() = 3;
    view[2].cast<int &>() = 4;

    ASSERT_EQ(view[1u].cast<int>(), 3);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_TRUE(ret);
    ASSERT_FALSE(view.insert(ret, test::empty{}));
    ASSERT_TRUE(view.insert(++ret, 1.));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.begin()->cast<int>(), 0);
    ASSERT_EQ((++view.begin())->cast<int>(), 1);

    ret = view.insert(cview.end(), 64);

    ASSERT_TRUE(ret);
    ASSERT_EQ(*ret, 64);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_TRUE(ret);
    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(ret->cast<int>(), 1);

    ret = view.erase(cview.begin());

    ASSERT_TRUE(ret);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(ret->cast<int>(), 2);

    ASSERT_TRUE(view.clear());
    ASSERT_TRUE(view.reserve(8u));
    ASSERT_EQ(view.size(), 0u);
}

TEST(SequenceContainer, StdVectorBool) {
    using proxy_type = typename std::vector<bool>::reference;
    using const_proxy_type = typename std::vector<bool>::const_reference;

    std::vector<bool> vec{};
    auto any = entt::forward_as_meta(vec);
    auto cany = std::as_const(any).as_ref();

    auto view = any.as_sequence_container();
    auto cview = cany.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<bool>());

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.resize(3u));
    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    view[0].cast<proxy_type>() = true;
    view[1].cast<proxy_type>() = true;
    view[2].cast<proxy_type>() = false;

    ASSERT_EQ(cview[1u].cast<const_proxy_type>(), true);

    auto it = view.begin();
    auto ret = view.insert(it, true);

    ASSERT_TRUE(ret);
    ASSERT_FALSE(view.insert(ret, test::empty{}));
    ASSERT_TRUE(view.insert(++ret, false));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.begin()->cast<proxy_type>(), true);
    ASSERT_EQ((++cview.begin())->cast<const_proxy_type>(), false);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_TRUE(ret);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(ret->cast<proxy_type>(), false);

    ASSERT_TRUE(view.clear());
    ASSERT_TRUE(view.reserve(8u));
    ASSERT_EQ(cview.size(), 0u);
}

TEST(SequenceContainer, StdArray) {
    std::array<int, 3> arr{};
    auto any = entt::forward_as_meta(arr);
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());
    ASSERT_FALSE(view.resize(5u));
    ASSERT_EQ(view.size(), 3u);

    view[0].cast<int &>() = 2;
    view[1].cast<int &>() = 3;
    view[2].cast<int &>() = 4;

    ASSERT_EQ(view[1u].cast<int>(), 3);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_FALSE(ret);
    ASSERT_FALSE(view.insert(it, 'c'));
    ASSERT_FALSE(view.insert(++it, 1.));

    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ(view.begin()->cast<int>(), 2);
    ASSERT_EQ((++view.begin())->cast<int>(), 3);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_FALSE(ret);
    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ(it->cast<int>(), 2);

    ASSERT_FALSE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 3u);
}

TEST(SequenceContainer, StdList) {
    std::list<int> list{};
    auto any = entt::forward_as_meta(list);
    auto view = any.as_sequence_container();
    auto cview = std::as_const(any).as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.resize(3u));
    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    view[0].cast<int &>() = 2;
    view[1].cast<int &>() = 3;
    view[2].cast<int &>() = 4;

    ASSERT_EQ(view[1u].cast<int>(), 3);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_TRUE(ret);
    ASSERT_FALSE(view.insert(ret, test::empty{}));
    ASSERT_TRUE(view.insert(++ret, 1.));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.begin()->cast<int>(), 0);
    ASSERT_EQ((++view.begin())->cast<int>(), 1);

    ret = view.insert(cview.end(), 64);

    ASSERT_TRUE(ret);
    ASSERT_EQ(*ret, 64);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_TRUE(ret);
    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(ret->cast<int>(), 1);

    ret = view.erase(cview.begin());

    ASSERT_TRUE(ret);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(ret->cast<int>(), 2);

    ASSERT_TRUE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 0u);
}

TEST(SequenceContainer, StdDeque) {
    std::deque<int> deque{};
    auto any = entt::forward_as_meta(deque);
    auto view = any.as_sequence_container();
    auto cview = std::as_const(any).as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.resize(3u));
    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    view[0].cast<int &>() = 2;
    view[1].cast<int &>() = 3;
    view[2].cast<int &>() = 4;

    ASSERT_EQ(view[1u].cast<int>(), 3);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_TRUE(ret);
    ASSERT_FALSE(view.insert(ret, test::empty{}));
    ASSERT_TRUE(view.insert(++ret, 1.));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.begin()->cast<int>(), 0);
    ASSERT_EQ((++view.begin())->cast<int>(), 1);

    ret = view.insert(cview.end(), 64);

    ASSERT_TRUE(ret);
    ASSERT_EQ(*ret, 64);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_TRUE(ret);
    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(ret->cast<int>(), 1);

    ret = view.erase(cview.begin());

    ASSERT_TRUE(ret);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(ret->cast<int>(), 2);

    ASSERT_TRUE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 0u);
}

TEST(SequenceContainer, NonDefaultConstructible) {
    std::vector<test::non_default_constructible> vec{};
    auto any = entt::forward_as_meta(vec);
    auto view = any.as_sequence_container();

    ASSERT_FALSE(view.resize(5u));
}

TEST(SequenceContainer, Constness) {
    std::vector<int> vec{};
    auto any = entt::forward_as_meta(std::as_const(vec));
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_FALSE(view.resize(3u));
    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());

    vec.push_back(64);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view[0].cast<const int &>(), 64);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_FALSE(ret);
    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(it->cast<int>(), 64);
    ASSERT_EQ(++it, view.end());

    it = view.begin();
    ret = view.erase(it);

    ASSERT_FALSE(ret);
    ASSERT_EQ(view.size(), 1u);

    ASSERT_FALSE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 1u);
}

ENTT_DEBUG_TEST(SequenceContainerDeathTest, Constness) {
    std::vector<int> vec{};
    auto any = entt::forward_as_meta(std::as_const(vec));
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view[0].cast<int &>() = 2, "");
}

TEST(SequenceContainer, FromConstAny) {
    const std::vector<int> vec{64};
    const entt::meta_any any{vec};
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());
    ASSERT_EQ(view[0].cast<const int &>(), 64);
}

TEST(SequenceContainer, FromConstAnyRef) {
    std::vector<int> vec{64};
    const entt::meta_any any = entt::forward_as_meta(vec);
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());
    ASSERT_EQ(view[0].cast<const int &>(), 64);
}

TEST(SequenceContainer, FromConstAnyConstRef) {
    std::vector<int> vec{64};
    const entt::meta_any any = entt::forward_as_meta(std::as_const(vec));
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), entt::resolve<int>());
    ASSERT_EQ(view[0].cast<const int &>(), 64);
}

ENTT_DEBUG_TEST(SequenceContainerDeathTest, FromConstAny) {
    const std::vector<int> vec{64};
    const entt::meta_any any{vec};
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view[0].cast<int &>() = 2, "");
}

ENTT_DEBUG_TEST(SequenceContainerDeathTest, FromConstAnyRef) {
    std::vector<int> vec{64};
    const entt::meta_any any = entt::forward_as_meta(vec);
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view[0].cast<int &>() = 2, "");
}

ENTT_DEBUG_TEST(SequenceContainerDeathTest, FromConstAnyConstRef) {
    std::vector<int> vec{64};
    const entt::meta_any any = entt::forward_as_meta(std::as_const(vec));
    auto view = any.as_sequence_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view[0].cast<int &>() = 2, "");
}

TEST(AssociativeContainer, Empty) {
    entt::meta_associative_container container{};

    ASSERT_FALSE(container);

    entt::meta_any any{std::map<int, char>{}};
    container = any.as_associative_container();

    ASSERT_TRUE(container);
}

TEST(AssociativeContainer, Iterator) {
    std::map<int, char> map{{2, 'c'}, {3, 'd'}, {4, 'e'}};
    auto any = entt::forward_as_meta(map);
    entt::meta_associative_container::iterator first{};
    auto view = any.as_associative_container();

    ASSERT_FALSE(first);

    first = view.begin();
    const auto last = view.end();

    ASSERT_TRUE(first);
    ASSERT_TRUE(last);

    ASSERT_FALSE(first == last);
    ASSERT_TRUE(first != last);

    ASSERT_NE(first, last);
    ASSERT_EQ((first++)->first.cast<int>(), 2);
    ASSERT_EQ((++first)->second.cast<char>(), 'e');
    ASSERT_NE(first++, last);
    ASSERT_EQ(first, last);

    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);
}

TEST(AssociativeContainer, StdMap) {
    std::map<int, char> map{{2, 'c'}, {3, 'd'}, {4, 'e'}};
    auto any = entt::forward_as_meta(map);
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::resolve<char>());
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));

    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(view.find(3)->second.cast<char>(), 'd');

    ASSERT_FALSE(view.insert(test::empty{}, 'a'));
    ASSERT_FALSE(view.insert(1, test::empty{}));

    ASSERT_TRUE(view.insert(0, 'a'));
    ASSERT_TRUE(view.insert(1., static_cast<int>('b')));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.find(0)->second.cast<char>(), 'a');
    ASSERT_EQ(view.find(1.)->second.cast<char>(), 'b');

    ASSERT_EQ(view.erase(test::empty{}), 0u);
    ASSERT_FALSE(view.find(test::empty{}));
    ASSERT_EQ(view.size(), 5u);

    ASSERT_EQ(view.erase(0), 1u);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    view.find(1.)->second.cast<char &>() = 'f';

    ASSERT_EQ(view.find(1.f)->second.cast<char>(), 'f');

    ASSERT_EQ(view.erase(1.), 1u);
    ASSERT_TRUE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 0u);
}

TEST(AssociativeContainer, StdSet) {
    std::set<int> set{2, 3, 4};
    auto any = entt::forward_as_meta(set);
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::meta_type{});
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(view.find(3)->first.cast<int>(), 3);

    ASSERT_FALSE(view.insert(test::empty{}));

    ASSERT_TRUE(view.insert(.0));
    ASSERT_TRUE(view.insert(1));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.find(0)->first.cast<int>(), 0);
    ASSERT_EQ(view.find(1.)->first.cast<int>(), 1);

    ASSERT_EQ(view.erase(test::empty{}), 0u);
    ASSERT_FALSE(view.find(test::empty{}));
    ASSERT_EQ(view.size(), 5u);

    ASSERT_EQ(view.erase(0), 1u);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    ASSERT_EQ(view.find(1.f)->first.try_cast<int>(), nullptr);
    ASSERT_NE(view.find(1.)->first.try_cast<const int>(), nullptr);
    ASSERT_EQ(view.find(true)->first.cast<const int &>(), 1);

    ASSERT_EQ(view.erase(1.), 1u);
    ASSERT_TRUE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 0u);
}

TEST(AssociativeContainer, DenseMap) {
    entt::dense_map<int, char> map{};
    auto any = entt::forward_as_meta(map);
    auto view = any.as_associative_container();

    map.emplace(2, 'c');
    map.emplace(3, 'd');
    map.emplace(4, '3');

    ASSERT_TRUE(view);
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::resolve<char>());
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));

    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(view.find(3)->second.cast<char>(), 'd');

    ASSERT_FALSE(view.insert(test::empty{}, 'a'));
    ASSERT_FALSE(view.insert(1, test::empty{}));

    ASSERT_TRUE(view.insert(0, 'a'));
    ASSERT_TRUE(view.insert(1., static_cast<int>('b')));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.find(0)->second.cast<char>(), 'a');
    ASSERT_EQ(view.find(1.)->second.cast<char>(), 'b');

    ASSERT_EQ(view.erase(test::empty{}), 0u);
    ASSERT_FALSE(view.find(test::empty{}));
    ASSERT_EQ(view.size(), 5u);

    ASSERT_EQ(view.erase(0), 1u);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    view.find(1.)->second.cast<char &>() = 'f';

    ASSERT_EQ(view.find(1.f)->second.cast<char>(), 'f');

    ASSERT_EQ(view.erase(1.), 1u);
    ASSERT_TRUE(view.clear());
    ASSERT_TRUE(view.reserve(8u));
    ASSERT_EQ(view.size(), 0u);
}

TEST(AssociativeContainer, DenseSet) {
    entt::dense_set<int> set{};
    auto any = entt::forward_as_meta(set);
    auto view = any.as_associative_container();

    set.emplace(2);
    set.emplace(3);
    set.emplace(4);

    ASSERT_TRUE(view);
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::meta_type{});
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(view.find(3)->first.cast<int>(), 3);

    ASSERT_FALSE(view.insert(test::empty{}));

    ASSERT_TRUE(view.insert(.0));
    ASSERT_TRUE(view.insert(1));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ(view.find(0)->first.cast<int>(), 0);
    ASSERT_EQ(view.find(1.)->first.cast<int>(), 1);

    ASSERT_EQ(view.erase(test::empty{}), 0u);
    ASSERT_FALSE(view.find(test::empty{}));
    ASSERT_EQ(view.size(), 5u);

    ASSERT_EQ(view.erase(0), 1u);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    ASSERT_EQ(view.find(1.f)->first.try_cast<int>(), nullptr);
    ASSERT_NE(view.find(1.)->first.try_cast<const int>(), nullptr);
    ASSERT_EQ(view.find(true)->first.cast<const int &>(), 1);

    ASSERT_EQ(view.erase(1.), 1u);
    ASSERT_TRUE(view.clear());
    ASSERT_TRUE(view.reserve(8u));
    ASSERT_EQ(view.size(), 0u);
}

TEST(KeyValueAssociativeContainer, Constness) {
    std::map<int, char> map{};
    auto any = entt::forward_as_meta(std::as_const(map));
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::resolve<char>());
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());

    map[2] = 'c';

    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(view.find(2)->second.cast<const char &>(), 'c');

    ASSERT_FALSE(view.insert(0, 'a'));
    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(view.find(0), view.end());
    ASSERT_EQ(view.find(2)->second.cast<char>(), 'c');

    ASSERT_EQ(view.erase(2), 0u);
    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.find(2), view.end());

    ASSERT_FALSE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 1u);
}

ENTT_DEBUG_TEST(KeyValueAssociativeContainerDeathTest, Constness) {
    std::map<int, char> map{};
    auto any = entt::forward_as_meta(std::as_const(map));
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view.find(2)->second.cast<char &>() = 'a', "");
}

TEST(KeyOnlyAssociativeContainer, Constness) {
    std::set<int> set{};
    auto any = entt::forward_as_meta(std::as_const(set));
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::meta_type{});
    ASSERT_EQ(view.value_type(), (entt::resolve<int>()));

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());

    set.insert(2);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(view.find(2)->first.try_cast<int>(), nullptr);
    ASSERT_NE(view.find(2)->first.try_cast<const int>(), nullptr);
    ASSERT_EQ(view.find(2)->first.cast<int>(), 2);
    ASSERT_EQ(view.find(2)->first.cast<const int &>(), 2);

    ASSERT_FALSE(view.insert(0));
    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(view.find(0), view.end());
    ASSERT_EQ(view.find(2)->first.cast<int>(), 2);

    ASSERT_EQ(view.erase(2), 0u);
    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.find(2), view.end());

    ASSERT_FALSE(view.clear());
    ASSERT_FALSE(view.reserve(8u));
    ASSERT_EQ(view.size(), 1u);
}

TEST(KeyValueAssociativeContainer, FromConstAny) {
    const std::map<int, char> map{{2, 'c'}};
    const entt::meta_any any{map};
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));
    ASSERT_EQ(view.find(2)->second.cast<const char &>(), 'c');
}

TEST(KeyValueAssociativeContainer, FromConstAnyRef) {
    std::map<int, char> map{{2, 'c'}};
    const entt::meta_any any = entt::forward_as_meta(map);
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));
    ASSERT_EQ(view.find(2)->second.cast<const char &>(), 'c');
}

TEST(KeyValueAssociativeContainer, FromConstAnyConstRef) {
    std::map<int, char> map{{2, 'c'}};
    const entt::meta_any any = entt::forward_as_meta(std::as_const(map));
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));
    ASSERT_EQ(view.find(2)->second.cast<const char &>(), 'c');
}

ENTT_DEBUG_TEST(KeyValueAssociativeContainerDeathTest, FromConstAny) {
    const std::map<int, char> map{{2, 'c'}};
    const entt::meta_any any{map};
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view.find(2)->second.cast<char &>() = 'a', "");
}

ENTT_DEBUG_TEST(KeyValueAssociativeContainerDeathTest, FromConstAnyRef) {
    std::map<int, char> map{{2, 'c'}};
    const entt::meta_any any = entt::forward_as_meta(map);
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view.find(2)->second.cast<char &>() = 'a', "");
}

ENTT_DEBUG_TEST(KeyValueAssociativeContainerDeathTest, FromConstAnyConstRef) {
    std::map<int, char> map{{2, 'c'}};
    const entt::meta_any any = entt::forward_as_meta(std::as_const(map));
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_DEATH(view.find(2)->second.cast<char &>() = 'a', "");
}

TEST(KeyOnlyAssociativeContainer, FromConstAny) {
    const std::set<int> set{2};
    const entt::meta_any any{set};
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), (entt::resolve<int>()));

    ASSERT_EQ(view.find(2)->first.try_cast<int>(), nullptr);
    ASSERT_NE(view.find(2)->first.try_cast<const int>(), nullptr);
    ASSERT_EQ(view.find(2)->first.cast<int>(), 2);
    ASSERT_EQ(view.find(2)->first.cast<const int &>(), 2);
}

TEST(KeyOnlyAssociativeContainer, FromConstAnyRef) {
    std::set<int> set{2};
    const entt::meta_any any = entt::forward_as_meta(set);
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), (entt::resolve<int>()));

    ASSERT_EQ(view.find(2)->first.try_cast<int>(), nullptr);
    ASSERT_NE(view.find(2)->first.try_cast<const int>(), nullptr);
    ASSERT_EQ(view.find(2)->first.cast<int>(), 2);
    ASSERT_EQ(view.find(2)->first.cast<const int &>(), 2);
}

TEST(KeyOnlyAssociativeContainer, FromConstAnyConstRef) {
    std::set<int> set{2};
    const entt::meta_any any = entt::forward_as_meta(std::as_const(set));
    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_EQ(view.value_type(), (entt::resolve<int>()));

    ASSERT_EQ(view.find(2)->first.try_cast<int>(), nullptr);
    ASSERT_NE(view.find(2)->first.try_cast<const int>(), nullptr);
    ASSERT_EQ(view.find(2)->first.cast<int>(), 2);
    ASSERT_EQ(view.find(2)->first.cast<const int &>(), 2);
}
