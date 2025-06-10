#include <algorithm>
#include <array>
#include <iterator>
#include <tuple>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/storage.hpp>
#include <entt/entity/view.hpp>
#include "../../common/boxed_type.h"
#include "../../common/empty.h"
#include "../../common/pointer_stable.h"

TEST(SingleStorageView, Functionalities) {
    entt::storage<char> storage{};
    const entt::basic_view view{storage};
    const entt::basic_view cview{std::as_const(storage)};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    ASSERT_TRUE(view.empty());

    storage.emplace(entity[1u]);

    ASSERT_NO_THROW(view.begin()++);
    ASSERT_NO_THROW(++cview.begin());
    ASSERT_NO_THROW([](auto it) { return it++; }(view.rbegin()));
    ASSERT_NO_THROW([](auto it) { return ++it; }(cview.rbegin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_NE(view.rbegin(), view.rend());
    ASSERT_NE(cview.rbegin(), cview.rend());
    ASSERT_EQ(view.size(), 1u);
    ASSERT_FALSE(view.empty());

    storage.emplace(entity[0u]);

    ASSERT_EQ(view.size(), 2u);

    view.get<char>(entity[0u]) = '1';
    std::get<0>(view.get(entity[1u])) = '2';

    ASSERT_EQ(view.get<0u>(entity[0u]), '1');
    ASSERT_EQ(cview.get<0u>(entity[0u]), view.get<char>(entity[0u]));
    ASSERT_EQ(view.get<char>(entity[1u]), '2');

    for(auto entt: view) {
        ASSERT_TRUE(entt == entity[0u] || entt == entity[1u]);
        ASSERT_TRUE(entt != entity[0u] || cview.get<const char>(entt) == '1');
        ASSERT_TRUE(entt != entity[1u] || std::get<const char &>(cview.get(entt)) == '2');
    }

    storage.erase(entity.begin(), entity.end());

    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.rbegin(), view.rend());
    ASSERT_TRUE(view.empty());

    const decltype(view) invalid{};

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);
    ASSERT_FALSE(invalid);
}

TEST(SingleStorageView, InvalidView) {
    entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<>> view{};
    auto iterable = view.each();

    ASSERT_FALSE(view);

    ASSERT_EQ(view.size(), 0u);
    ASSERT_TRUE(view.empty());
    ASSERT_FALSE(view.contains(entt::null));
    ASSERT_EQ(view.find(entt::null), view.end());

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    ASSERT_EQ(view.begin(), typename decltype(view)::iterator{});
    ASSERT_EQ(view.begin(), view.end());

    ASSERT_EQ(view.rbegin(), typename decltype(view)::reverse_iterator{});
    ASSERT_EQ(view.rbegin(), view.rend());

    ASSERT_EQ(iterable.begin(), iterable.end());
    ASSERT_EQ(iterable.cbegin(), iterable.cend());

    view.each([](const int &) { FAIL(); });
    view.each([](const entt::entity, const int &) { FAIL(); });

    entt::storage<int> storage;
    view.storage(storage);

    ASSERT_TRUE(view);
}

TEST(SingleStorageView, Constructors) {
    entt::storage<int> storage{};

    const entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<>> invalid{};
    const entt::basic_view from_storage{storage};
    const entt::basic_view from_tuple{std::forward_as_tuple(storage)};

    ASSERT_FALSE(invalid);
    ASSERT_TRUE(from_storage);
    ASSERT_TRUE(from_tuple);

    ASSERT_NE(from_storage.handle(), nullptr);
    ASSERT_EQ(from_storage.handle(), from_tuple.handle());
}

TEST(SingleStorageView, Handle) {
    entt::storage<int> storage{};
    const entt::basic_view view{storage};
    const entt::entity entity{0};

    const auto *handle = view.handle();

    ASSERT_NE(handle, nullptr);

    ASSERT_TRUE(handle->empty());
    ASSERT_FALSE(handle->contains(entity));
    ASSERT_EQ(handle, view.handle());

    storage.emplace(entity);

    ASSERT_FALSE(handle->empty());
    ASSERT_TRUE(handle->contains(entity));
    ASSERT_EQ(handle, view.handle());
}

TEST(SingleStorageView, ElementAccess) {
    entt::storage<int> storage{};
    const entt::basic_view view{storage};
    const entt::basic_view cview{std::as_const(storage)};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    storage.emplace(entity[0u], 4);
    storage.emplace(entity[1u], 1);

    ASSERT_EQ(view[entity[0u]], 4);
    ASSERT_EQ(cview[entity[1u]], 1);
}

TEST(SingleStorageView, Contains) {
    entt::storage<int> storage{};
    const entt::basic_view view{storage};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    storage.emplace(entity[0u]);
    storage.emplace(entity[1u]);

    storage.erase(entity[0u]);

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));
}

TEST(SingleStorageView, Empty) {
    entt::storage<int> storage{};
    const entt::basic_view view{storage};

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_EQ(view.rbegin(), view.rend());
}

TEST(SingleStorageView, Each) {
    std::tuple<entt::storage<int>, entt::storage<double>> storage{};
    const entt::basic_view view{std::forward_as_tuple(std::get<0>(storage)), std::forward_as_tuple(std::get<1>(storage))};
    const entt::basic_view cview{std::as_const(std::get<0>(storage))};
    const std::array entity{entt::entity{0}, entt::entity{1}};

    std::get<0>(storage).emplace(entity[0u], 0);
    std::get<0>(storage).emplace(entity[1u], 1);

    auto iterable = view.each();
    auto citerable = cview.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_THROW(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(it.base(), view.begin());
    ASSERT_EQ((it++, ++it), iterable.end());
    ASSERT_EQ(it.base(), view.end());

    view.each([expected = 1](auto entt, int &value) mutable {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), expected);
        ASSERT_EQ(value, expected);
        --expected;
    });

    cview.each([expected = 1](const int &value) mutable {
        ASSERT_EQ(value, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    testing::StaticAssertTypeEq<decltype(std::get<1>(*iterable.begin())), int &>();
    testing::StaticAssertTypeEq<decltype(std::get<1>(*citerable.begin())), const int &>();

    // do not use iterable, make sure an iterable view works when created from a temporary
    for(auto [entt, value]: view.each()) {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), value);
    }
}

TEST(SingleStorageView, ConstNonConstAndAllInBetween) {
    entt::storage<int> storage{};
    const entt::basic_view view{storage};
    const entt::basic_view cview{std::as_const(storage)};
    const entt::entity entity{0};

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);

    storage.emplace(entity, 0);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(cview.size(), 1u);

    testing::StaticAssertTypeEq<decltype(view.get<0u>({})), int &>();
    testing::StaticAssertTypeEq<decltype(view.get<int>({})), int &>();
    testing::StaticAssertTypeEq<decltype(view.get({})), std::tuple<int &>>();

    testing::StaticAssertTypeEq<decltype(cview.get<0u>({})), const int &>();
    testing::StaticAssertTypeEq<decltype(cview.get<const int>({})), const int &>();
    testing::StaticAssertTypeEq<decltype(cview.get({})), std::tuple<const int &>>();

    view.each([](auto &&iv) {
        testing::StaticAssertTypeEq<decltype(iv), int &>();
    });

    cview.each([](auto &&iv) {
        testing::StaticAssertTypeEq<decltype(iv), const int &>();
    });

    for([[maybe_unused]] auto [entt, iv]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
    }

    for([[maybe_unused]] auto [entt, iv]: cview.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), const int &>();
    }
}

TEST(SingleStorageView, ConstNonConstAndAllInBetweenWithEmptyType) {
    entt::storage<test::empty> storage{};
    const entt::basic_view view{storage};
    const entt::basic_view cview{std::as_const(storage)};
    const entt::entity entity{0};

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);

    storage.emplace(entity);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(cview.size(), 1u);

    testing::StaticAssertTypeEq<decltype(view.get({})), std::tuple<>>();
    testing::StaticAssertTypeEq<decltype(cview.get({})), std::tuple<>>();

    for([[maybe_unused]] auto [entt]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
    }

    for([[maybe_unused]] auto [entt]: cview.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
    }
}

TEST(SingleStorageView, Find) {
    entt::storage<int> storage{};
    const entt::basic_view view{storage};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{2}};

    storage.emplace(entity[0u]);
    storage.emplace(entity[1u]);
    storage.emplace(entity[2u]);

    storage.erase(entity[1u]);

    ASSERT_NE(view.find(entity[0u]), view.end());
    ASSERT_EQ(view.find(entity[1u]), view.end());
    ASSERT_NE(view.find(entity[2u]), view.end());

    auto it = view.find(entity[2u]);

    ASSERT_EQ(*it, entity[2u]);
    ASSERT_EQ(*(++it), entity[0u]);
    ASSERT_EQ(++it, view.end());
}

TEST(SingleStorageView, EmptyType) {
    entt::storage<test::empty> storage{};
    const entt::basic_view view{storage};
    const entt::entity entity{0};

    storage.emplace(entity);

    view.each([&](const auto entt) {
        ASSERT_EQ(entity, entt);
    });

    view.each([check = true]() mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        ASSERT_EQ(entity, entt);
    }
}

TEST(SingleStorageView, FrontBack) {
    entt::storage<char> storage{};
    const entt::basic_view view{std::as_const(storage)};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    storage.emplace(entity[0u]);
    storage.emplace(entity[1u]);

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[0u]);
}

TEST(SingleStorageView, DeductionGuide) {
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<entt::storage<int> &>()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<const entt::storage<int> &>()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<test::pointer_stable>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<entt::storage<test::pointer_stable> &>()})>();

    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<int> &>()), std::make_tuple()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<const entt::storage<int> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<test::pointer_stable>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<test::pointer_stable> &>())})>();
}

TEST(SingleStorageView, IterableViewAlgorithmCompatibility) {
    entt::storage<char> storage{};
    const entt::basic_view view{storage};
    const entt::entity entity{0};

    storage.emplace(entity);

    const auto iterable = view.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TEST(SingleStorageView, StableType) {
    entt::storage<test::pointer_stable> storage{};
    entt::basic_view<entt::get_t<entt::storage<test::pointer_stable>>, entt::exclude_t<>> view{};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.find(entity[0u]), view.end());

    view.storage(storage);

    storage.emplace(entity[0u], 0);
    storage.emplace(entity[1u], 1);
    storage.erase(entity[0u]);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_EQ(view->size(), 2u);

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_FALSE(view->contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    ASSERT_EQ(std::distance(view.begin(), view.end()), 1);
    ASSERT_EQ(std::distance(view->begin(), view->end()), 2);

    ASSERT_EQ(*view.begin(), entity[1u]);
    ASSERT_EQ(++view.begin(), view.end());

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[1u]);

    ASSERT_EQ(view.find(entity[0u]), view.end());
    ASSERT_NE(view.find(entity[1u]), view.end());

    for(auto [entt, elem]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(elem), test::pointer_stable &>();
        ASSERT_EQ(entt, entity[1u]);
    }

    view.each([&](const auto entt, const auto &elem) {
        ASSERT_EQ(elem, view->get(entity[1u]));
        ASSERT_EQ(entt, entity[1u]);
    });

    view.each([&](const auto &elem) {
        ASSERT_EQ(elem, view->get(entity[1u]));
    });

    storage.erase(view.begin(), view.end());

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_EQ(view->size(), 2u);

    ASSERT_EQ(std::distance(view.begin(), view.end()), 0);
    ASSERT_EQ(std::distance(view->begin(), view->end()), 2);

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    storage.compact();

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(view->size(), 0u);
}

TEST(SingleStorageView, Storage) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    entt::basic_view view{std::get<0>(storage)};
    entt::basic_view cview{std::as_const(std::get<1>(storage))};
    const entt::entity entity{0};

    testing::StaticAssertTypeEq<decltype(view.storage()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<0u>()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<int>()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<const int>()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(cview.storage()), const entt::storage<char> *>();
    testing::StaticAssertTypeEq<decltype(cview.storage<0u>()), const entt::storage<char> *>();
    testing::StaticAssertTypeEq<decltype(cview.storage<char>()), const entt::storage<char> *>();
    testing::StaticAssertTypeEq<decltype(cview.storage<const char>()), const entt::storage<char> *>();

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);

    ASSERT_NE(view.storage<int>(), nullptr);
    ASSERT_NE(cview.storage<0u>(), nullptr);

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 0u);

    view.storage()->emplace(entity);
    std::get<1>(storage).emplace(entity);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(cview.size(), 1u);
    ASSERT_TRUE(view.storage<int>()->contains(entity));
    ASSERT_TRUE(cview.storage<0u>()->contains(entity));

    view.storage()->erase(entity);

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(cview.size(), 1u);
    ASSERT_FALSE(view.storage<0u>()->contains(entity));
    ASSERT_TRUE(cview.storage<const char>()->contains(entity));

    view = {};
    cview = {};

    ASSERT_FALSE(view);
    ASSERT_FALSE(cview);

    ASSERT_EQ(view.storage<0u>(), nullptr);
    ASSERT_EQ(cview.storage<const char>(), nullptr);
}

TEST(SingleStorageView, ArrowOperator) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    entt::basic_view view{std::get<0>(storage)};
    entt::basic_view cview{std::as_const(std::get<1>(storage))};
    const entt::entity entity{0};

    testing::StaticAssertTypeEq<decltype(view.operator->()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(cview.operator->()), const entt::storage<char> *>();

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);

    ASSERT_NE(view.operator->(), nullptr);
    ASSERT_NE(cview.operator->(), nullptr);

    view->emplace(entity);
    std::get<1>(storage).emplace(entity);

    ASSERT_EQ(view.operator->(), &std::get<0>(storage));
    ASSERT_EQ(cview.operator->(), &std::get<1>(storage));

    ASSERT_EQ(view.operator->(), view.storage());
    ASSERT_EQ(cview.operator->(), cview.storage());

    view = {};
    cview = {};

    ASSERT_EQ(view.operator->(), nullptr);
    ASSERT_EQ(cview.operator->(), nullptr);
}

TEST(SingleStorageView, SwapStorage) {
    std::tuple<entt::storage<int>, entt::storage<int>> storage{};
    entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<>> view{};
    entt::basic_view<entt::get_t<const entt::storage<int>>, entt::exclude_t<>> cview{};
    const entt::entity entity{0};

    ASSERT_FALSE(view);
    ASSERT_FALSE(cview);
    ASSERT_EQ(view.storage<0u>(), nullptr);
    ASSERT_EQ(cview.storage<const int>(), nullptr);

    std::get<0>(storage).emplace(entity);

    view.storage(std::get<0>(storage));
    cview.storage(std::get<0>(storage));

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);
    ASSERT_NE(view.storage<0u>(), nullptr);
    ASSERT_NE(cview.storage<const int>(), nullptr);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(cview.size(), 1u);
    ASSERT_TRUE(view.contains(entity));
    ASSERT_TRUE(cview.contains(entity));

    view.storage(std::get<1>(storage));
    cview.storage(std::get<1>(storage));

    ASSERT_TRUE(view.empty());
    ASSERT_TRUE(cview.empty());
}

TEST(SingleStorageView, StorageEntity) {
    entt::storage<entt::entity> storage{};
    entt::basic_view<entt::get_t<entt::storage<entt::entity>>, entt::exclude_t<>> view{};
    const std::array entity{storage.generate(), storage.generate()};

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.find(entity[0u]), view.end());

    view.storage(storage);

    storage.erase(entity[0u]);
    storage.bump(entity[0u]);

    ASSERT_EQ(view.size(), 1u);
    ASSERT_EQ(view->size(), 2u);

    ASSERT_FALSE(view.empty());
    ASSERT_FALSE(view->empty());

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view->contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    ASSERT_NE(view.begin(), view->begin());
    ASSERT_EQ(view.end(), view->end());

    ASSERT_EQ(std::distance(view.begin(), view.end()), 1);
    ASSERT_EQ(std::distance(view->begin(), view->end()), 2);

    ASSERT_EQ(*view.begin(), entity[1u]);
    ASSERT_EQ(*view->begin(), entity[0u]);

    ASSERT_EQ(++view->begin(), view.begin());
    ASSERT_EQ(++view.begin(), view.end());

    ASSERT_EQ(view.rbegin(), view->rbegin());
    ASSERT_NE(view.rend(), view->rend());

    ASSERT_EQ(std::distance(view.rbegin(), view.rend()), 1);
    ASSERT_EQ(std::distance(view->rbegin(), view->rend()), 2);

    ASSERT_EQ(++view.rbegin(), view.rend());
    ASSERT_EQ(++view.rend(), view->rend());

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[1u]);

    ASSERT_EQ(view.find(entity[0u]), view.end());
    ASSERT_NE(view->find(entity[0u]), view.end());
    ASSERT_NE(view.find(entity[1u]), view.end());

    for(auto elem: view.each()) {
        ASSERT_EQ(std::get<0>(elem), entity[1u]);
    }

    view.each([&entity](auto entt) {
        ASSERT_EQ(entt, entity[1u]);
    });

    view.each([&]() {
        storage.erase(entity[1u]);
    });

    ASSERT_EQ(view.size(), 0u);
    ASSERT_EQ(view->size(), 2u);

    ASSERT_TRUE(view.empty());
    ASSERT_FALSE(view->empty());

    ASSERT_EQ(std::distance(view.begin(), view.end()), 0);
    ASSERT_EQ(std::distance(view->begin(), view->end()), 2);

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));
}

TEST(MultiStorageView, Functionalities) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const entt::basic_view cview{std::as_const(std::get<0>(storage)), std::as_const(std::get<1>(storage))};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    std::get<1>(storage).emplace(entity[0u], '1');

    std::get<0>(storage).emplace(entity[1u], 4);
    std::get<1>(storage).emplace(entity[1u], '2');

    ASSERT_EQ(*view.begin(), entity[1u]);
    ASSERT_EQ(*cview.begin(), entity[1u]);
    ASSERT_EQ(++view.begin(), (view.end()));
    ASSERT_EQ(++cview.begin(), (cview.end()));

    ASSERT_NO_THROW((view.begin()++));
    ASSERT_NO_THROW((++cview.begin()));

    ASSERT_NE(view.begin(), view.end());
    ASSERT_NE(cview.begin(), cview.end());
    ASSERT_EQ(view.size_hint(), 1u);

    for(auto entt: view) {
        ASSERT_EQ(std::get<0>(cview.get<const int, const char>(entt)), 4);
        ASSERT_EQ(std::get<0>(cview.get<0u, 1u>(entt)), 4);

        ASSERT_EQ(std::get<1>(view.get<int, char>(entt)), '2');
        ASSERT_EQ(std::get<1>(view.get<0u, 1u>(entt)), '2');

        ASSERT_EQ(cview.get<const char>(entt), '2');
        ASSERT_EQ(cview.get<1u>(entt), '2');
    }

    const decltype(view) invalid{};

    ASSERT_TRUE(view);
    ASSERT_TRUE(cview);
    ASSERT_FALSE(invalid);
}

TEST(MultiStorageView, InvalidView) {
    entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<entt::storage<char>>> view{};
    auto iterable = view.each();

    ASSERT_FALSE(view);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_FALSE(view.contains(entt::null));
    ASSERT_EQ(view.find(entt::null), view.end());

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    ASSERT_EQ(view.begin(), typename decltype(view)::iterator{});
    ASSERT_EQ(view.begin(), view.end());

    ASSERT_EQ(iterable.begin(), iterable.end());
    ASSERT_EQ(iterable.cbegin(), iterable.cend());

    view.each([](const int &) { FAIL(); });
    view.each([](const entt::entity, const int &) { FAIL(); });

    entt::storage<int> storage{};
    const entt::entity entity{0};

    view.storage(storage);
    storage.emplace(entity);

    ASSERT_FALSE(view);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_TRUE(view.contains(entity));
    ASSERT_NE(view.find(entity), view.end());

    ASSERT_EQ(view.front(), entity);
    ASSERT_EQ(view.back(), entity);

    ASSERT_NE(view.begin(), typename decltype(view)::iterator{});
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(iterable.begin(), iterable.end());
    ASSERT_EQ(iterable.cbegin(), iterable.cend());

    entt::storage<char> other{};
    view.storage(other);

    ASSERT_TRUE(view);
}

TEST(MultiStorageView, Constructors) {
    entt::storage<int> storage{};

    const entt::basic_view<entt::get_t<entt::storage<int>, entt::storage<int>>, entt::exclude_t<>> invalid{};
    const entt::basic_view from_storage{storage, storage};
    const entt::basic_view from_tuple{std::forward_as_tuple(storage, storage)};

    ASSERT_FALSE(invalid);
    ASSERT_TRUE(from_storage);
    ASSERT_TRUE(from_tuple);

    ASSERT_NE(from_storage.handle(), nullptr);
    ASSERT_EQ(from_storage.handle(), from_tuple.handle());
}

TEST(MultiStorageView, Handle) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};

    const auto *handle = view.handle();
    const entt::entity entity{0};

    ASSERT_NE(handle, nullptr);

    ASSERT_TRUE(handle->empty());
    ASSERT_FALSE(handle->contains(entity));
    ASSERT_EQ(handle, view.handle());

    std::get<0>(storage).emplace(entity);

    ASSERT_FALSE(handle->empty());
    ASSERT_TRUE(handle->contains(entity));
    ASSERT_EQ(handle, view.handle());

    view.refresh();
    const auto *other = view.handle();

    ASSERT_NE(other, nullptr);

    ASSERT_TRUE(other->empty());
    ASSERT_FALSE(other->contains(entity));
    ASSERT_EQ(other, view.handle());
    ASSERT_NE(handle, other);

    view.use<int>();

    ASSERT_NE(other, view.handle());
    ASSERT_EQ(handle, view.handle());
}

TEST(MultiStorageView, Iterator) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{entt::entity{0}, entt::entity{1}};

    std::get<0>(storage).insert(entity.begin(), entity.end());
    std::get<1>(storage).insert(entity.begin(), entity.end());

    using iterator = typename decltype(view)::iterator;

    iterator end{view.begin()};
    iterator begin{};
    begin = view.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, view.begin());
    ASSERT_EQ(end, view.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(*begin, entity[1u]);
    ASSERT_EQ(*begin.operator->(), entity[1u]);
    ASSERT_EQ(begin++, view.begin());

    ASSERT_EQ(*begin, entity[0u]);
    ASSERT_EQ(*begin.operator->(), entity[0u]);
    ASSERT_EQ(++begin, view.end());
}

TEST(MultiStorageView, ElementAccess) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const entt::basic_view cview{std::as_const(std::get<0>(storage)), std::as_const(std::get<1>(storage))};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u], 4);
    std::get<0>(storage).emplace(entity[1u], 1);

    std::get<1>(storage).emplace(entity[0u], '0');
    std::get<1>(storage).emplace(entity[1u], '1');

    ASSERT_EQ(view[entity[0u]], std::make_tuple(4, '0'));
    ASSERT_EQ(cview[entity[1u]], std::make_tuple(1, '1'));
}

TEST(MultiStorageView, Contains) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u]);
    std::get<0>(storage).emplace(entity[1u]);

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);

    std::get<0>(storage).erase(entity[0u]);

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));
}

TEST(MultiStorageView, SizeHint) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u]);

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);

    view.use<int>();

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(++view.begin(), view.end());

    view.use<char>();

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_NE(view.begin(), view.end());
    ASSERT_EQ(++view.begin(), view.end());
}

TEST(MultiStorageView, UseAndRefresh) {
    std::tuple<entt::storage<int>, entt::storage<char>, entt::storage<double>> storage{};
    entt::basic_view view{std::forward_as_tuple(std::get<0>(storage), std::get<1>(storage)), std::forward_as_tuple(std::get<2>(storage))};
    const std::array entity{entt::entity{0u}, entt::entity{1u}, entt::entity{2u}};

    std::get<0>(storage).emplace(entity[0u]);
    std::get<0>(storage).emplace(entity[1u]);

    std::get<1>(storage).emplace(entity[1u]);
    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[2u]);

    view.use<int>();

    ASSERT_EQ(view.handle()->info(), entt::type_id<int>());
    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[0u]);

    view.use<char>();

    ASSERT_EQ(view.handle()->info(), entt::type_id<char>());
    ASSERT_EQ(view.front(), entity[0u]);
    ASSERT_EQ(view.back(), entity[1u]);

    view.refresh();

    ASSERT_EQ(view.handle()->info(), entt::type_id<int>());
}

TEST(MultiStorageView, Each) {
    std::tuple<entt::storage<int>, entt::storage<char>, entt::storage<double>> storage{};
    const entt::basic_view view{std::forward_as_tuple(std::get<0>(storage), std::get<1>(storage)), std::forward_as_tuple(std::get<2>(storage))};
    const entt::basic_view cview{std::as_const(std::get<0>(storage)), std::as_const(std::get<1>(storage))};
    const std::array entity{entt::entity{0}, entt::entity{1}};

    std::get<0>(storage).emplace(entity[0u], 0);
    std::get<1>(storage).emplace(entity[0u], static_cast<char>(0));

    std::get<0>(storage).emplace(entity[1u], 1);
    std::get<1>(storage).emplace(entity[1u], static_cast<char>(1));

    auto iterable = view.each();
    auto citerable = cview.each();

    ASSERT_NE(citerable.begin(), citerable.end());
    ASSERT_NO_THROW(iterable.begin()->operator=(*iterable.begin()));
    ASSERT_EQ(decltype(iterable.end()){}, iterable.end());

    auto it = iterable.begin();

    ASSERT_EQ(it.base(), view.begin());
    ASSERT_EQ((it++, ++it), iterable.end());
    ASSERT_EQ(it.base(), view.end());

    view.each([expected = 1](auto entt, int &ivalue, char &cvalue) mutable {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), expected);
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    cview.each([expected = 1](const int &ivalue, const char &cvalue) mutable {
        ASSERT_EQ(ivalue, expected);
        ASSERT_EQ(cvalue, expected);
        --expected;
    });

    ASSERT_EQ(std::get<0>(*iterable.begin()), entity[1u]);
    ASSERT_EQ(std::get<0>(*++citerable.begin()), entity[0u]);

    testing::StaticAssertTypeEq<decltype(std::get<1>(*iterable.begin())), int &>();
    testing::StaticAssertTypeEq<decltype(std::get<2>(*citerable.begin())), const char &>();

    // do not use iterable, make sure an iterable view works when created from a temporary
    for(auto [entt, ivalue, cvalue]: entt::basic_view{std::get<0>(storage), std::get<1>(storage)}.each()) {
        ASSERT_EQ(static_cast<int>(entt::to_integral(entt)), ivalue);
        ASSERT_EQ(static_cast<char>(entt::to_integral(entt)), cvalue);
    }
}

TEST(MultiStorageView, EachWithSuggestedType) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{2}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u], 0);
    std::get<1>(storage).emplace(entity[0u]);

    std::get<0>(storage).emplace(entity[1u], 1);
    std::get<1>(storage).emplace(entity[1u]);

    std::get<0>(storage).emplace(entity[2u], 2);
    std::get<1>(storage).emplace(entity[2u]);

    // makes char a better candidate during iterations
    std::get<0>(storage).emplace(entity[3u], 3);

    view.use<int>();
    view.each([value = 2](const auto curr, const auto) mutable {
        ASSERT_EQ(curr, value--);
    });

    std::get<0>(storage).sort([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    view.use<0u>();
    view.each([value = 0](const auto curr, const auto) mutable {
        ASSERT_EQ(curr, value++);
    });

    std::get<0>(storage).sort([](const auto lhs, const auto rhs) {
        return lhs > rhs;
    });

    auto value = entt::basic_view{std::get<0>(storage), std::get<1>(storage)}.size_hint();

    for(auto &&curr: entt::basic_view{std::get<0>(storage), std::get<1>(storage)}.each()) {
        ASSERT_EQ(std::get<1>(curr), static_cast<int>(--value));
    }

    std::get<0>(storage).sort([](const auto lhs, const auto rhs) {
        return lhs < rhs;
    });

    value = {};
    view.use<int>();

    for(auto &&curr: view.each()) {
        ASSERT_EQ(std::get<1>(curr), static_cast<int>(value++));
    }
}

TEST(MultiStorageView, EachWithHoles) {
    std::tuple<entt::storage<char>, entt::storage<test::boxed_int>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{2}};

    std::get<0>(storage).emplace(entity[0u], '0');
    std::get<0>(storage).emplace(entity[1u], '1');

    std::get<1>(storage).emplace(entity[0u], 0);
    std::get<1>(storage).emplace(entity[2u], 2);

    view.each([&entity](auto entt, const char &cv, const test::boxed_int &iv) {
        ASSERT_EQ(entt, entity[0u]);
        ASSERT_EQ(cv, '0');
        ASSERT_EQ(iv.value, 0);
    });

    for(auto &&curr: view.each()) {
        ASSERT_EQ(std::get<0>(curr), entity[0u]);
        ASSERT_EQ(std::get<1>(curr), '0');
        ASSERT_EQ(std::get<2>(curr).value, 0);
    }
}

TEST(MultiStorageView, ConstNonConstAndAllInBetween) {
    std::tuple<entt::storage<int>, entt::storage<test::empty>, entt::storage<char>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::get<1>(storage), std::as_const(std::get<2>(storage))};
    const entt::entity entity{0};

    ASSERT_EQ(view.size_hint(), 0u);

    std::get<0>(storage).emplace(entity, 0);
    std::get<1>(storage).emplace(entity);
    std::get<2>(storage).emplace(entity, 'c');

    ASSERT_EQ(view.size_hint(), 1u);

    testing::StaticAssertTypeEq<decltype(view.get<0u>({})), int &>();
    testing::StaticAssertTypeEq<decltype(view.get<2u>({})), const char &>();
    testing::StaticAssertTypeEq<decltype(view.get<0u, 2u>({})), std::tuple<int &, const char &>>();

    testing::StaticAssertTypeEq<decltype(view.get<int>({})), int &>();
    testing::StaticAssertTypeEq<decltype(view.get<const char>({})), const char &>();
    testing::StaticAssertTypeEq<decltype(view.get<int, const char>({})), std::tuple<int &, const char &>>();

    testing::StaticAssertTypeEq<decltype(view.get({})), std::tuple<int &, const char &>>();

    view.each([](auto &&iv, auto &&cv) {
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), const char &>();
    });

    for([[maybe_unused]] auto [entt, iv, cv]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(cv), const char &>();
    }
}

TEST(MultiStorageView, Find) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::as_const(std::get<1>(storage))};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{2}};

    std::get<0>(storage).emplace(entity[0u]);
    std::get<0>(storage).emplace(entity[1u]);
    std::get<0>(storage).emplace(entity[2u]);

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);
    std::get<1>(storage).emplace(entity[2u]);

    std::get<0>(storage).erase(entity[1u]);

    ASSERT_NE(view.find(entity[0u]), view.end());
    ASSERT_EQ(view.find(entity[1u]), view.end());
    ASSERT_NE(view.find(entity[2u]), view.end());

    auto it = view.find(entity[2u]);

    ASSERT_EQ(*it, entity[2u]);
    ASSERT_EQ(*(++it), entity[0u]);
    ASSERT_EQ(++it, view.end());
}

TEST(MultiStorageView, Exclude) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::forward_as_tuple(std::get<0>(storage)), std::forward_as_tuple(std::as_const(std::get<1>(storage)))};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{2}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u], 0);

    std::get<0>(storage).emplace(entity[1u], 1);
    std::get<1>(storage).emplace(entity[1u]);

    std::get<0>(storage).emplace(entity[2u], 2);

    std::get<0>(storage).emplace(entity[3u], 3);
    std::get<1>(storage).emplace(entity[3u]);

    for(const auto entt: view) {
        ASSERT_TRUE(entt == entity[0u] || entt == entity[2u]);

        if(entt == entity[0u]) {
            ASSERT_EQ(view.get<const int>(entity[0u]), 0);
            ASSERT_EQ(view.get<0u>(entity[0u]), 0);
        } else if(entt == entity[2u]) {
            ASSERT_EQ(std::get<0>(view.get(entity[2u])), 2);
        }
    }

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[2u]);
    std::get<1>(storage).erase(entity[1u]);
    std::get<1>(storage).erase(entity[3u]);

    for(const auto entt: view) {
        ASSERT_TRUE(entt == entity[1u] || entt == entity[3u]);

        if(entt == entity[1u]) {
            ASSERT_EQ(std::get<0>(view.get(entity[1u])), 1);
        } else if(entt == entity[3u]) {
            ASSERT_EQ(view.get<const int>(entity[3u]), 3);
            ASSERT_EQ(view.get<0u>(entity[3u]), 3);
        }
    }
}

TEST(MultiStorageView, EmptyType) {
    std::tuple<entt::storage<int>, entt::storage<test::empty>> storage{};
    entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const entt::entity entity{0};

    std::get<0>(storage).emplace(entity, 3);
    std::get<1>(storage).emplace(entity);

    view.each([](int value) mutable {
        ASSERT_EQ(value, 3);
    });

    for(auto [entt, value]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(value), int &>();
        ASSERT_EQ(entity, entt);
        ASSERT_EQ(value, 3);
    }

    view.use<1u>();
    view.each([](int value) mutable {
        ASSERT_EQ(value, 3);
    });

    view.use<test::empty>();
    for(auto [entt, value]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(value), int &>();
        ASSERT_EQ(entity, entt);
        ASSERT_EQ(value, 3);
    }
}

TEST(MultiStorageView, FrontBack) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::as_const(std::get<0>(storage)), std::as_const(std::get<1>(storage))};
    const std::array entity{entt::entity{0}, entt::entity{1}, entt::entity{2}};

    ASSERT_EQ(view.front(), static_cast<entt::entity>(entt::null));
    ASSERT_EQ(view.back(), static_cast<entt::entity>(entt::null));

    std::get<0>(storage).emplace(entity[0u]);
    std::get<0>(storage).emplace(entity[1u]);

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);

    std::get<1>(storage).emplace(entity[2u]);

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[0u]);
}

TEST(MultiStorageView, ExtendedGet) {
    using type = decltype(std::declval<entt::basic_view<entt::get_t<entt::storage<int>, entt::storage<test::empty>, entt::storage<char>>, entt::exclude_t<>>>().get({}));

    ASSERT_EQ(std::tuple_size_v<type>, 2u);

    testing::StaticAssertTypeEq<std::tuple_element_t<0, type>, int &>();
    testing::StaticAssertTypeEq<std::tuple_element_t<1, type>, char &>();
}

TEST(MultiStorageView, DeductionGuide) {
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>, entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<entt::storage<int> &>(), std::declval<entt::storage<double> &>()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>, entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<const entt::storage<int> &>(), std::declval<entt::storage<double> &>()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>, const entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<entt::storage<int> &>(), std::declval<const entt::storage<double> &>()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>, const entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<const entt::storage<int> &>(), std::declval<const entt::storage<double> &>()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>, entt::storage<test::pointer_stable>>, entt::exclude_t<>>, decltype(entt::basic_view{std::declval<entt::storage<int> &>(), std::declval<entt::storage<test::pointer_stable> &>()})>();

    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>, entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<int> &>(), std::declval<entt::storage<double> &>()), std::make_tuple()})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>, entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<const entt::storage<int> &>(), std::declval<entt::storage<double> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>, const entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<int> &>(), std::declval<const entt::storage<double> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>, const entt::storage<double>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<const entt::storage<int> &>(), std::declval<const entt::storage<double> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>, entt::storage<test::pointer_stable>>, entt::exclude_t<>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<int> &>(), std::declval<entt::storage<test::pointer_stable> &>())})>();

    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<entt::storage<double>>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<int> &>()), std::forward_as_tuple(std::declval<entt::storage<double> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>>, entt::exclude_t<entt::storage<double>>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<const entt::storage<int> &>()), std::forward_as_tuple(std::declval<entt::storage<double> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<const entt::storage<double>>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<int> &>()), std::forward_as_tuple(std::declval<const entt::storage<double> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>>, entt::exclude_t<const entt::storage<double>>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<const entt::storage<int> &>()), std::forward_as_tuple(std::declval<const entt::storage<double> &>())})>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<entt::storage<test::pointer_stable>>>, decltype(entt::basic_view{std::forward_as_tuple(std::declval<entt::storage<int> &>()), std::forward_as_tuple(std::declval<entt::storage<test::pointer_stable> &>())})>();
}

TEST(MultiStorageView, IterableViewAlgorithmCompatibility) {
    std::tuple<entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::as_const(std::get<0>(storage)), std::as_const(std::get<1>(storage))};
    const entt::entity entity{0};

    std::get<0>(storage).emplace(entity);
    std::get<1>(storage).emplace(entity);

    const auto iterable = view.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [entity](auto args) { return std::get<0>(args) == entity; });

    ASSERT_EQ(std::get<0>(*it), entity);
}

TEST(MultiStorageView, StableType) {
    std::tuple<entt::storage<int>, entt::storage<test::pointer_stable>> storage{};
    entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u]);
    std::get<0>(storage).emplace(entity[1u]);
    std::get<0>(storage).erase(entity[0u]);

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);
    std::get<1>(storage).erase(entity[0u]);

    ASSERT_EQ(view.size_hint(), 1u);

    view.use<test::pointer_stable>();

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[1u]);

    ASSERT_EQ(*view.begin(), entity[1u]);
    ASSERT_EQ(++view.begin(), view.end());

    view.each([&entity](const auto entt, int, test::pointer_stable) {
        ASSERT_EQ(entity[1u], entt);
    });

    view.each([check = true](int, test::pointer_stable) mutable {
        ASSERT_TRUE(check);
        check = false;
    });

    for(auto [entt, iv, st]: view.each()) {
        testing::StaticAssertTypeEq<decltype(entt), entt::entity>();
        testing::StaticAssertTypeEq<decltype(iv), int &>();
        testing::StaticAssertTypeEq<decltype(st), test::pointer_stable &>();
        ASSERT_EQ(entity[1u], entt);
    }

    std::get<1>(storage).compact();

    ASSERT_EQ(view.size_hint(), 1u);
}

TEST(MultiStorageView, StableTypeWithExclude) {
    std::tuple<entt::storage<test::pointer_stable>, entt::storage<int>> storage{};
    const entt::basic_view view{std::forward_as_tuple(std::get<0>(storage)), std::forward_as_tuple(std::get<1>(storage))};
    const std::array entity{entt::entity{1}, entt::entity{3}};
    const entt::entity tombstone = entt::tombstone;

    std::get<0>(storage).emplace(entity[0u], 0);
    std::get<0>(storage).emplace(entity[1u], 4);
    std::get<1>(storage).emplace(entity[0u]);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    std::get<0>(storage).erase(entity[0u]);
    std::get<1>(storage).erase(entity[0u]);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    for(auto entt: view) {
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, entity[1u]);
    }

    for(auto [entt, comp]: view.each()) {
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, entity[1u]);
        ASSERT_EQ(comp.value, 4);
    }

    view.each([&entity, tombstone](const auto entt, auto &&...) {
        ASSERT_NE(entt, tombstone);
        ASSERT_EQ(entt, entity[1u]);
    });
}

TEST(MultiStorageView, SameStorageTypes) {
    std::tuple<entt::storage<int>, entt::storage<int>> storage{};
    entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u], 2);

    std::get<1>(storage).emplace(entity[0u], 3);
    std::get<1>(storage).emplace(entity[1u], 1);

    ASSERT_TRUE(view.contains(entity[0u]));
    ASSERT_FALSE(view.contains(entity[1u]));

    ASSERT_EQ((view.get<0u, 1u>(entity[0u])), (std::make_tuple(2, 3)));
    ASSERT_EQ(view.get<1u>(entity[0u]), 3);

    for(auto entt: view) {
        ASSERT_EQ(entt, entity[0u]);
    }

    view.each([&](auto entt, auto &&first, auto &&second) {
        ASSERT_EQ(entt, entity[0u]);
        ASSERT_EQ(first, 2);
        ASSERT_EQ(second, 3);
    });

    for(auto [entt, first, second]: view.each()) {
        ASSERT_EQ(entt, entity[0u]);
        ASSERT_EQ(first, 2);
        ASSERT_EQ(second, 3);
    }

    ASSERT_EQ(view.handle(), &std::get<0>(storage));

    view.use<1u>();

    ASSERT_EQ(view.handle(), &std::get<1>(storage));
}

TEST(MultiStorageView, Storage) {
    std::tuple<entt::storage<int>, entt::storage<char>, entt::storage<double>, entt::storage<float>> storage{};
    entt::basic_view view{std::forward_as_tuple(std::get<0>(storage), std::as_const(std::get<1>(storage))), std::forward_as_tuple(std::get<2>(storage), std::as_const(std::get<3>(storage)))};
    const entt::entity entity{0};

    testing::StaticAssertTypeEq<decltype(view.storage<0u>()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<int>()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<const int>()), entt::storage<int> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<1u>()), const entt::storage<char> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<char>()), const entt::storage<char> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<const char>()), const entt::storage<char> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<2u>()), entt::storage<double> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<double>()), entt::storage<double> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<const double>()), entt::storage<double> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<3u>()), const entt::storage<float> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<float>()), const entt::storage<float> *>();
    testing::StaticAssertTypeEq<decltype(view.storage<const float>()), const entt::storage<float> *>();

    ASSERT_TRUE(view);

    ASSERT_NE(view.storage<int>(), nullptr);
    ASSERT_NE(view.storage<1u>(), nullptr);
    ASSERT_NE(view.storage<double>(), nullptr);
    ASSERT_NE(view.storage<3u>(), nullptr);

    ASSERT_EQ(view.size_hint(), 0u);

    view.storage<int>()->emplace(entity);
    view.storage<double>()->emplace(entity);
    std::get<1>(storage).emplace(entity);
    std::get<3>(storage).emplace(entity);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_TRUE(view.storage<int>()->contains(entity));
    ASSERT_TRUE(view.storage<const char>()->contains(entity));
    ASSERT_TRUE(view.storage<double>()->contains(entity));
    ASSERT_TRUE(view.storage<const float>()->contains(entity));

    view.storage<double>()->erase(entity);
    std::get<3>(storage).erase(entity);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_NE(view.begin(), view.end());
    ASSERT_TRUE(view.storage<const int>()->contains(entity));
    ASSERT_TRUE(view.storage<char>()->contains(entity));
    ASSERT_FALSE(view.storage<const double>()->contains(entity));
    ASSERT_FALSE(view.storage<float>()->contains(entity));

    view.storage<0u>()->erase(entity);

    ASSERT_EQ(view.size_hint(), 0u);
    ASSERT_EQ(view.begin(), view.end());
    ASSERT_FALSE(view.storage<0u>()->contains(entity));
    ASSERT_TRUE(view.storage<1u>()->contains(entity));
    ASSERT_FALSE(view.storage<2u>()->contains(entity));
    ASSERT_FALSE(view.storage<3u>()->contains(entity));

    view = {};

    ASSERT_FALSE(view);

    ASSERT_EQ(view.storage<0u>(), nullptr);
    ASSERT_EQ(view.storage<const char>(), nullptr);
    ASSERT_EQ(view.storage<2u>(), nullptr);
    ASSERT_EQ(view.storage<const float>(), nullptr);
}

TEST(MultiStorageView, SwapStorage) {
    std::tuple<entt::storage<int>, entt::storage<char>, entt::storage<int>, entt::storage<char>> storage{};
    entt::basic_view<entt::get_t<entt::storage<int>>, entt::exclude_t<const entt::storage<char>>> view{};
    const entt::entity entity{0};

    ASSERT_FALSE(view);
    ASSERT_EQ(view.storage<0u>(), nullptr);
    ASSERT_EQ(view.storage<const char>(), nullptr);

    std::get<0>(storage).emplace(entity);
    std::get<1>(storage).emplace(entity);

    view.storage(std::get<0>(storage));
    view.storage<1u>(std::get<1>(storage));

    ASSERT_TRUE(view);
    ASSERT_NE(view.storage<int>(), nullptr);
    ASSERT_NE(view.storage<1u>(), nullptr);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_FALSE(view.contains(entity));

    view.storage(std::get<3>(storage));

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_TRUE(view.contains(entity));

    view.storage(std::get<2>(storage));

    ASSERT_EQ(view.size_hint(), 0u);
}

TEST(MultiStorageView, StorageEntity) {
    std::tuple<entt::storage<entt::entity>, entt::storage<char>> storage{};
    const entt::basic_view view{std::get<0>(storage), std::get<1>(storage)};
    const std::array entity{std::get<0>(storage).generate(), std::get<0>(storage).generate()};

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);

    std::get<1>(storage).erase(entity[0u]);
    std::get<0>(storage).erase(entity[0u]);
    std::get<0>(storage).bump(entity[0u]);

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[1u]);

    ASSERT_EQ(view.size_hint(), 1u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(std::distance(view.begin(), view.end()), 1);
    ASSERT_EQ(*view.begin(), entity[1u]);

    for(auto elem: view.each()) {
        ASSERT_EQ(std::get<0>(elem), entity[1u]);
    }

    view.each([&entity](auto entt, auto &&...) {
        ASSERT_EQ(entt, entity[1u]);
    });
}

TEST(MultiStorageView, StorageEntityWithExclude) {
    std::tuple<entt::storage<entt::entity>, entt::storage<int>, entt::storage<char>> storage{};
    const entt::basic_view view{std::forward_as_tuple(std::get<0>(storage), std::get<1>(storage)), std::forward_as_tuple(std::get<2>(storage))};
    const std::array entity{std::get<0>(storage).generate(), std::get<0>(storage).generate(), std::get<0>(storage).generate()};

    std::get<1>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[1u]);
    std::get<1>(storage).emplace(entity[2u]);

    std::get<2>(storage).emplace(entity[2u]);

    std::get<1>(storage).erase(entity[0u]);
    std::get<0>(storage).erase(entity[0u]);
    std::get<0>(storage).bump(entity[0u]);

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));
    ASSERT_FALSE(view.contains(entity[2u]));

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[1u]);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(std::distance(view.begin(), view.end()), 1);
    ASSERT_EQ(*view.begin(), entity[1u]);

    for(auto elem: view.each()) {
        ASSERT_EQ(std::get<0>(elem), entity[1u]);
    }

    view.each([&entity](auto entt, auto &&...) {
        ASSERT_EQ(entt, entity[1u]);
    });
}

TEST(MultiStorageView, StorageEntityExcludeOnly) {
    std::tuple<entt::storage<entt::entity>, entt::storage<int>> storage{};
    const entt::basic_view view{std::forward_as_tuple(std::get<0>(storage)), std::forward_as_tuple(std::get<1>(storage))};
    const std::array entity{std::get<0>(storage).generate(), std::get<0>(storage).generate(), std::get<0>(storage).generate()};

    std::get<1>(storage).emplace(entity[2u]);

    std::get<0>(storage).erase(entity[0u]);
    std::get<0>(storage).bump(entity[0u]);

    ASSERT_FALSE(view.contains(entity[0u]));
    ASSERT_TRUE(view.contains(entity[1u]));
    ASSERT_FALSE(view.contains(entity[2u]));

    ASSERT_EQ(view.front(), entity[1u]);
    ASSERT_EQ(view.back(), entity[1u]);

    ASSERT_EQ(view.size_hint(), 2u);
    ASSERT_NE(view.begin(), view.end());

    ASSERT_EQ(std::distance(view.begin(), view.end()), 1);
    ASSERT_EQ(*view.begin(), entity[1u]);

    for(auto [entt]: view.each()) {
        ASSERT_EQ(entt, entity[1u]);
    }

    view.each([&entity](auto entt) {
        ASSERT_EQ(entt, entity[1u]);
    });
}

TEST(View, Pipe) {
    std::tuple<entt::storage<int>, entt::storage<double>, entt::storage<test::empty>, entt::storage<test::pointer_stable>, entt::storage<float>> storage{};
    const std::array entity{entt::entity{1}, entt::entity{3}};

    std::get<0>(storage).emplace(entity[0u]);
    std::get<1>(storage).emplace(entity[0u]);
    std::get<2>(storage).emplace(entity[0u]);

    std::get<0>(storage).emplace(entity[1u]);
    std::get<3>(storage).emplace(entity[1u]);

    entt::basic_view view1{std::forward_as_tuple(std::get<0>(storage)), std::forward_as_tuple(std::as_const(std::get<1>(storage)))};
    const entt::basic_view view2{std::forward_as_tuple(std::as_const(std::get<0>(storage))), std::forward_as_tuple(std::get<4>(storage))};
    entt::basic_view view3{std::get<2>(storage)};
    const entt::basic_view view4{std::as_const(std::get<3>(storage))};

    testing::StaticAssertTypeEq<decltype(view1 | std::get<2>(storage)), decltype(view1 | view3)>();
    testing::StaticAssertTypeEq<decltype(view1 | std::get<3>(std::as_const(storage))), decltype(view1 | view4)>();
    testing::StaticAssertTypeEq<decltype(view1 | std::get<2>(storage) | view4), decltype(view1 | view3 | view4)>();

    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<entt::storage<int>, const entt::storage<int>>, entt::exclude_t<const entt::storage<double>, entt::storage<float>>>, decltype(view1 | view2)>();
    testing::StaticAssertTypeEq<entt::basic_view<entt::get_t<const entt::storage<int>, entt::storage<int>>, entt::exclude_t<entt::storage<float>, const entt::storage<double>>>, decltype(view2 | view1)>();
    testing::StaticAssertTypeEq<decltype((view3 | view2) | view1), decltype(view3 | (view2 | view1))>();

    ASSERT_FALSE((view1 | view2).contains(entity[0u]));
    ASSERT_TRUE((view1 | view2).contains(entity[1u]));

    ASSERT_TRUE((view3 | view2).contains(entity[0u]));
    ASSERT_FALSE((view3 | view2).contains(entity[1u]));

    ASSERT_FALSE((view1 | view2 | view3).contains(entity[0u]));
    ASSERT_FALSE((view1 | view2 | view3).contains(entity[1u]));

    ASSERT_FALSE((view1 | view4 | view2).contains(entity[0u]));
    ASSERT_TRUE((view1 | view4 | view2).contains(entity[1u]));

    view1 = {};
    view3 = {};

    ASSERT_FALSE(view1);
    ASSERT_TRUE(view2);
    ASSERT_FALSE(view3);
    ASSERT_TRUE(view4);

    auto pack14 = view1 | view4;
    auto pack32 = view3 | view2;

    ASSERT_FALSE(pack14);
    ASSERT_FALSE(pack32);

    ASSERT_EQ(pack14.storage<int>(), nullptr);
    ASSERT_EQ(pack14.storage<const double>(), nullptr);
    ASSERT_NE(pack14.storage<test::pointer_stable>(), nullptr);

    ASSERT_EQ(pack32.storage<test::empty>(), nullptr);
    ASSERT_NE(pack32.storage<const int>(), nullptr);
    ASSERT_NE(pack32.storage<float>(), nullptr);
}

TEST(View, PipeWithPlaceholder) {
    entt::storage<void> storage{};
    const entt::entity entity{0};

    const entt::basic_view view{storage};
    entt::basic_view<entt::get_t<entt::storage<void>>, entt::exclude_t<entt::storage<int>>> other{};

    other.storage(storage);

    ASSERT_FALSE(view.contains(entity));
    ASSERT_FALSE(other.contains(entity));

    auto pack = view | other;

    ASSERT_FALSE(pack.contains(entity));

    storage.emplace(entity);

    ASSERT_TRUE(view.contains(entity));
    ASSERT_TRUE(other.contains(entity));

    pack = view | other;

    ASSERT_TRUE(pack.contains(entity));
}
