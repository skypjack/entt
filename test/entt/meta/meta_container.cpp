#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/container.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/resolve.hpp>

struct MetaContainer: ::testing::Test {
    void SetUp() override {
        using namespace entt::literals;

        entt::meta<double>()
            .type("double"_hs)
            .conv<int>();

        entt::meta<int>()
            .type("int"_hs)
            .conv<char>();
    }

    void TearDown() override {
        for(auto type: entt::resolve()) {
            type.reset();
        }
    }
};

TEST_F(MetaContainer, InvalidContainer) {
    ASSERT_FALSE(entt::meta_any{42}.as_sequence_container());
    ASSERT_FALSE(entt::meta_any{42}.as_associative_container());

    ASSERT_FALSE((entt::meta_any{std::map<int, char>{}}.as_sequence_container()));
    ASSERT_FALSE(entt::meta_any{std::vector<int>{}}.as_associative_container());
}

TEST_F(MetaContainer, EmptySequenceContainer) {
    entt::meta_sequence_container container{};

    ASSERT_FALSE(container);

    entt::meta_any any{std::vector<int>{}};
    container = any.as_sequence_container();

    ASSERT_TRUE(container);
}

TEST_F(MetaContainer, EmptyAssociativeContainer) {
    entt::meta_associative_container container{};

    ASSERT_FALSE(container);

    entt::meta_any any{std::map<int, char>{}};
    container = any.as_associative_container();

    ASSERT_TRUE(container);
}

TEST_F(MetaContainer, SequenceContainerIterator) {
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

    ASSERT_NE(first, last);
    ASSERT_EQ((*(first++)).cast<int>(), 2);
    ASSERT_EQ((*(++first)).cast<int>(), 4);
    ASSERT_NE(first++, last);
    ASSERT_EQ(first, last);

    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);
}

TEST_F(MetaContainer, AssociativeContainerIterator) {
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
    ASSERT_EQ((*(first++)).first.cast<int>(), 2);
    ASSERT_EQ((*(++first)).second.cast<char>(), 'e');
    ASSERT_NE(first++, last);
    ASSERT_EQ(first, last);

    ASSERT_TRUE(first == last);
    ASSERT_FALSE(first != last);
}

TEST_F(MetaContainer, StdVector) {
    std::vector<int> vec{};
    auto any = entt::forward_as_meta(vec);

    auto view = any.as_sequence_container();

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

    ASSERT_TRUE(ret.second);
    ASSERT_FALSE(view.insert(ret.first, 'c').second);
    ASSERT_TRUE(view.insert(++ret.first, 1.).second);

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ((*view.begin()).cast<int>(), 0);
    ASSERT_EQ((*++view.begin()).cast<int>(), 1);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_TRUE(ret.second);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ((*ret.first).cast<int>(), 1);

    ASSERT_TRUE(view.clear());
    ASSERT_EQ(view.size(), 0u);
}

TEST_F(MetaContainer, StdArray) {
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

    ASSERT_FALSE(ret.second);
    ASSERT_FALSE(view.insert(it, 'c').second);
    ASSERT_FALSE(view.insert(++it, 1).second);

    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ((*view.begin()).cast<int>(), 2);
    ASSERT_EQ((*++view.begin()).cast<int>(), 3);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_FALSE(ret.second);
    ASSERT_EQ(view.size(), 3u);
    ASSERT_EQ((*it).cast<int>(), 2);

    ASSERT_FALSE(view.clear());
    ASSERT_EQ(view.size(), 3u);
}

TEST_F(MetaContainer, StdMap) {
    std::map<int, char> map{{2, 'c'}, {3, 'd'}, {4, 'e'}};
    auto any = entt::forward_as_meta(map);

    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_FALSE(view.key_only());
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::resolve<char>());
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));

    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ((*view.find(3)).second.cast<char>(), 'd');

    ASSERT_FALSE(view.insert('a', 'a'));
    ASSERT_FALSE(view.insert(1, 1.));

    ASSERT_TRUE(view.insert(0, 'a'));
    ASSERT_TRUE(view.insert(1., static_cast<int>('b')));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ((*view.find(0)).second.cast<char>(), 'a');
    ASSERT_EQ((*view.find(1.)).second.cast<char>(), 'b');

    ASSERT_FALSE(view.erase('c'));
    ASSERT_EQ(view.size(), 5u);
    ASSERT_FALSE(view.find('c'));

    ASSERT_TRUE(view.erase(0));
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    (*view.find(1)).second.cast<char &>() = 'f';

    ASSERT_EQ((*view.find(1)).second.cast<char>(), 'f');

    ASSERT_TRUE(view.erase(1.));
    ASSERT_TRUE(view.clear());
    ASSERT_EQ(view.size(), 0u);
}

TEST_F(MetaContainer, StdSet) {
    std::set<int> set{2, 3, 4};
    auto any = entt::forward_as_meta(set);

    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_TRUE(view.key_only());
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::meta_type{});
    ASSERT_EQ(view.value_type(), entt::resolve<int>());

    ASSERT_EQ(view.size(), 3u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ((*view.find(3)).first.cast<int>(), 3);

    ASSERT_FALSE(view.insert('0'));

    ASSERT_TRUE(view.insert(0));
    ASSERT_TRUE(view.insert(1));

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ((*view.find(0)).first.cast<int>(), 0);
    ASSERT_EQ((*view.find(1.)).first.cast<int>(), 1);

    ASSERT_FALSE(view.erase('c'));
    ASSERT_EQ(view.size(), 5u);
    ASSERT_FALSE(view.find('c'));

    ASSERT_TRUE(view.erase(0));
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ(view.find(0), view.end());

    ASSERT_EQ((*view.find(1)).first.try_cast<int>(), nullptr);
    ASSERT_NE((*view.find(1)).first.try_cast<const int>(), nullptr);
    ASSERT_EQ((*view.find(1)).first.cast<const int &>(), 1);

    ASSERT_TRUE(view.erase(1.));
    ASSERT_TRUE(view.clear());
    ASSERT_EQ(view.size(), 0u);
}

TEST_F(MetaContainer, ConstSequenceContainer) {
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

    vec.push_back(42);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_DEATH(view[0].cast<int &>() = 2, "");
    ASSERT_EQ(view[0].cast<const int &>(), 42);

    auto it = view.begin();
    auto ret = view.insert(it, 0);

    ASSERT_FALSE(ret.second);
    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ((*it).cast<int>(), 42);
    ASSERT_EQ(++it, view.end());

    it = view.begin();
    ret = view.erase(it);

    ASSERT_FALSE(ret.second);
    ASSERT_EQ(view.size(), 1u);

    ASSERT_FALSE(view.clear());
    ASSERT_EQ(view.size(), 1u);
}

TEST_F(MetaContainer, ConstKeyValueAssociativeContainer) {
    std::map<int, char> map{};
    auto any = entt::forward_as_meta(std::as_const(map));

    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_FALSE(view.key_only());
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::resolve<char>());
    ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());

    map[2] = 'c';

    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_DEATH((*view.find(2)).second.cast<char &>() = 'a', "");
    ASSERT_EQ((*view.find(2)).second.cast<const char &>(), 'c');

    ASSERT_FALSE(view.insert(0, 'a'));
    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(view.find(0), view.end());
    ASSERT_EQ((*view.find(2)).second.cast<char>(), 'c');

    ASSERT_FALSE(view.erase(2));
    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.find(2), view.end());

    ASSERT_FALSE(view.clear());
    ASSERT_EQ(view.size(), 1u);
}

TEST_F(MetaContainer, ConstKeyOnlyAssociativeContainer) {
    std::set<int> set{};
    auto any = entt::forward_as_meta(std::as_const(set));

    auto view = any.as_associative_container();

    ASSERT_TRUE(view);
    ASSERT_TRUE(view.key_only());
    ASSERT_EQ(view.key_type(), entt::resolve<int>());
    ASSERT_EQ(view.mapped_type(), entt::meta_type{});
    ASSERT_EQ(view.value_type(), (entt::resolve<int>()));

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());

    set.insert(2);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ((*view.find(2)).first.try_cast<int>(), nullptr);
    ASSERT_NE((*view.find(2)).first.try_cast<const int>(), nullptr);
    ASSERT_EQ((*view.find(2)).first.cast<int>(), 2);
    ASSERT_EQ((*view.find(2)).first.cast<const int &>(), 2);

    ASSERT_FALSE(view.insert(0));
    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(view.find(0), view.end());
    ASSERT_EQ((*view.find(2)).first.cast<int>(), 2);

    ASSERT_FALSE(view.erase(2));
    ASSERT_EQ(view.size(), 1u);
    ASSERT_NE(view.find(2), view.end());

    ASSERT_FALSE(view.clear());
    ASSERT_EQ(view.size(), 1u);
}

TEST_F(MetaContainer, SequenceContainerConstMetaAny) {
    auto test = [](const entt::meta_any any) {
        auto view = any.as_sequence_container();

        ASSERT_TRUE(view);
        ASSERT_EQ(view.value_type(), entt::resolve<int>());
        ASSERT_DEATH(view[0].cast<int &>() = 2, "");
        ASSERT_EQ(view[0].cast<const int &>(), 42);
    };

    std::vector<int> vec{42};

    test(vec);
    test(entt::forward_as_meta(vec));
    test(entt::forward_as_meta(std::as_const(vec)));
}

TEST_F(MetaContainer, KeyValueAssociativeContainerConstMetaAny) {
    auto test = [](const entt::meta_any any) {
        auto view = any.as_associative_container();

        ASSERT_TRUE(view);
        ASSERT_EQ(view.value_type(), (entt::resolve<std::pair<const int, char>>()));
        ASSERT_DEATH((*view.find(2)).second.cast<char &>() = 'a', "");
        ASSERT_EQ((*view.find(2)).second.cast<const char &>(), 'c');
    };

    std::map<int, char> map{{2, 'c'}};

    test(map);
    test(entt::forward_as_meta(map));
    test(entt::forward_as_meta(std::as_const(map)));
}

TEST_F(MetaContainer, KeyOnlyAssociativeContainerConstMetaAny) {
    auto test = [](const entt::meta_any any) {
        auto view = any.as_associative_container();

        ASSERT_TRUE(view);
        ASSERT_EQ(view.value_type(), (entt::resolve<int>()));

        ASSERT_EQ((*view.find(2)).first.try_cast<int>(), nullptr);
        ASSERT_NE((*view.find(2)).first.try_cast<const int>(), nullptr);
        ASSERT_EQ((*view.find(2)).first.cast<int>(), 2);
        ASSERT_EQ((*view.find(2)).first.cast<const int &>(), 2);
    };

    std::set<int> set{2};

    test(set);
    test(entt::forward_as_meta(set));
    test(entt::forward_as_meta(std::as_const(set)));
}

TEST_F(MetaContainer, StdVectorBool) {
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

    ASSERT_TRUE(ret.second);
    ASSERT_FALSE(view.insert(ret.first, 'c').second);
    ASSERT_TRUE(view.insert(++ret.first, false).second);

    ASSERT_EQ(view.size(), 5u);
    ASSERT_EQ((*view.begin()).cast<proxy_type>(), true);
    ASSERT_EQ((*++cview.begin()).cast<const_proxy_type>(), false);

    it = view.begin();
    ret = view.erase(it);

    ASSERT_TRUE(ret.second);
    ASSERT_EQ(view.size(), 4u);
    ASSERT_EQ((*ret.first).cast<proxy_type>(), false);

    ASSERT_TRUE(view.clear());
    ASSERT_EQ(cview.size(), 0u);
}
