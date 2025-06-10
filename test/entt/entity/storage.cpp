#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/iterator.hpp>
#include <entt/core/type_info.hpp>
#include <entt/entity/component.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/storage.hpp>
#include "../../common/aggregate.h"
#include "../../common/config.h"
#include "../../common/linter.hpp"
#include "../../common/new_delete.h"
#include "../../common/pointer_stable.h"
#include "../../common/throwing_allocator.hpp"
#include "../../common/throwing_type.hpp"
#include "../../common/tracked_memory_resource.hpp"

struct update_from_destructor {
    update_from_destructor(entt::storage<update_from_destructor> &ref, entt::entity other)
        : storage{&ref},
          target{other} {}

    update_from_destructor(const update_from_destructor &) = delete;
    update_from_destructor &operator=(const update_from_destructor &) = delete;

    update_from_destructor(update_from_destructor &&other) noexcept
        : storage{std::exchange(other.storage, nullptr)},
          target{std::exchange(other.target, entt::null)} {}

    update_from_destructor &operator=(update_from_destructor &&other) noexcept {
        storage = std::exchange(other.storage, nullptr);
        target = std::exchange(other.target, entt::null);
        return *this;
    }

    ~update_from_destructor() {
        if(target != entt::null && storage->contains(target)) {
            storage->erase(target);
        }
    }

private:
    entt::storage<update_from_destructor> *storage{};
    entt::entity target{entt::null};
};

struct create_from_constructor {
    create_from_constructor(entt::storage<create_from_constructor> &ref, entt::entity other)
        : child{other} {
        if(child != entt::null) {
            ref.emplace(child, ref, entt::null);
        }
    }

    entt::entity child;
};

template<>
struct entt::component_traits<std::unordered_set<char>> {
    static constexpr auto in_place_delete = true;
    static constexpr auto page_size = 4u;
};

template<>
struct entt::component_traits<int> {
    static constexpr auto in_place_delete = false;
    static constexpr auto page_size = 128u;
};

template<typename Type>
struct Storage: testing::Test {
    static_assert(entt::component_traits<Type>::page_size != 0u, "Empty type not allowed");

    using type = Type;
};

template<typename Type>
using StorageDeathTest = Storage<Type>;

using StorageTypes = ::testing::Types<int, test::pointer_stable>;

TYPED_TEST_SUITE(Storage, StorageTypes, );
TYPED_TEST_SUITE(StorageDeathTest, StorageTypes, );

TYPED_TEST(Storage, Constructors) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<value_type>());

    pool = entt::storage<value_type>{std::allocator<value_type>{}};

    ASSERT_EQ(pool.policy(), entt::deletion_policy{traits_type::in_place_delete});
    ASSERT_NO_THROW([[maybe_unused]] auto alloc = pool.get_allocator());
    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
}

TYPED_TEST(Storage, Move) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{3}, entt::entity{2}};

    pool.emplace(entity[0u], 3);

    static_assert(std::is_move_constructible_v<decltype(pool)>, "Move constructible type required");
    static_assert(std::is_move_assignable_v<decltype(pool)>, "Move assignable type required");

    entt::storage<value_type> other{std::move(pool)};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.index(entity[0u]), 0u);
    ASSERT_EQ(other.get(entity[0u]), value_type{3});

    entt::storage<value_type> extended{std::move(other), std::allocator<value_type>{}};

    test::is_initialized(other);

    ASSERT_TRUE(other.empty());
    ASSERT_FALSE(extended.empty());

    ASSERT_EQ(extended.info(), entt::type_id<value_type>());
    ASSERT_EQ(extended.index(entity[0u]), 0u);
    ASSERT_EQ(extended.get(entity[0u]), value_type{3});

    pool = std::move(extended);
    test::is_initialized(extended);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_TRUE(extended.empty());

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(pool.get(entity[0u]), value_type{3});

    other = entt::storage<value_type>{};
    other.emplace(entity[1u], 2);
    other = std::move(pool);
    test::is_initialized(pool);

    ASSERT_FALSE(pool.empty());
    ASSERT_FALSE(other.empty());

    ASSERT_EQ(other.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.index(entity[0u]), 0u);
    ASSERT_EQ(other.get(entity[0u]), value_type{3});
}

TYPED_TEST(Storage, Swap) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;
    entt::storage<value_type> other;

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    pool.emplace(entt::entity{4}, 1);

    other.emplace(entt::entity{2}, 2);
    other.emplace(entt::entity{1}, 3);
    other.erase(entt::entity{2});

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(other.size(), 1u + traits_type::in_place_delete);

    pool.swap(other);

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(other.info(), entt::type_id<value_type>());

    ASSERT_EQ(pool.size(), 1u + traits_type::in_place_delete);
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(pool.index(entt::entity{1}), traits_type::in_place_delete);
    ASSERT_EQ(other.index(entt::entity{4}), 0u);

    ASSERT_EQ(pool.get(entt::entity{1}), value_type{3});
    ASSERT_EQ(other.get(entt::entity{4}), value_type{1});
}

TYPED_TEST(Storage, Capacity) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;

    pool.reserve(64);

    ASSERT_EQ(pool.capacity(), traits_type::page_size);
    ASSERT_TRUE(pool.empty());

    pool.reserve(0);

    ASSERT_EQ(pool.capacity(), traits_type::page_size);
    ASSERT_TRUE(pool.empty());
}

TYPED_TEST(Storage, ShrinkToFit) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;

    for(std::size_t next{}; next < traits_type::page_size; ++next) {
        pool.emplace(entt::entity(next));
    }

    pool.emplace(entt::entity{traits_type::page_size});
    pool.erase(entt::entity{traits_type::page_size});
    pool.compact();

    ASSERT_EQ(pool.capacity(), 2 * traits_type::page_size);
    ASSERT_EQ(pool.size(), traits_type::page_size);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), traits_type::page_size);
    ASSERT_EQ(pool.size(), traits_type::page_size);

    pool.clear();

    ASSERT_EQ(pool.capacity(), traits_type::page_size);
    ASSERT_EQ(pool.size(), 0u);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 0u);
}

TYPED_TEST(Storage, Raw) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{1}, 1);
    pool.emplace(entt::entity{3}, 3);

    ASSERT_EQ(pool.raw()[0u][0u], value_type{1});
    ASSERT_EQ(std::as_const(pool).raw()[0u][1u], value_type{3});
}

TYPED_TEST(Storage, Iterator) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, value_type>();
    testing::StaticAssertTypeEq<typename iterator::pointer, value_type *>();
    testing::StaticAssertTypeEq<typename iterator::reference, value_type &>();

    entt::storage<value_type> pool;
    pool.emplace(entt::entity{1}, 2);

    iterator end{pool.begin()};
    iterator begin{};

    begin = pool.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, pool.begin());
    ASSERT_EQ(end, pool.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.index(), 0);
    ASSERT_EQ(end.index(), -1);

    ASSERT_EQ(begin++, pool.begin());
    ASSERT_EQ(begin--, pool.end());

    ASSERT_EQ(begin + 1, pool.end());
    ASSERT_EQ(end - 1, pool.begin());

    ASSERT_EQ(++begin, pool.end());
    ASSERT_EQ(--begin, pool.begin());

    ASSERT_EQ(begin += 1, pool.end());
    ASSERT_EQ(begin -= 1, pool.begin());

    ASSERT_EQ(begin + (end - begin), pool.end());
    ASSERT_EQ(begin - (begin - end), pool.end());

    ASSERT_EQ(end - (end - begin), pool.begin());
    ASSERT_EQ(end + (begin - end), pool.begin());

    ASSERT_EQ(begin[0u], *pool.begin().operator->());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, pool.begin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, pool.end());

    ASSERT_EQ(begin.index(), 0);
    ASSERT_EQ(end.index(), -1);

    pool.emplace(entt::entity{3}, 4);
    begin = pool.begin();

    ASSERT_EQ(begin.index(), 1);
    ASSERT_EQ(end.index(), -1);

    ASSERT_EQ(begin[0u], value_type{4});
    ASSERT_EQ(begin[1u], value_type{2});
}

TYPED_TEST(Storage, ConstIterator) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::const_iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, value_type>();
    testing::StaticAssertTypeEq<typename iterator::pointer, const value_type *>();
    testing::StaticAssertTypeEq<typename iterator::reference, const value_type &>();

    entt::storage<value_type> pool;
    pool.emplace(entt::entity{1}, 2);

    iterator cend{pool.cbegin()};
    iterator cbegin{};

    cbegin = pool.cend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, std::as_const(pool).begin());
    ASSERT_EQ(cend, std::as_const(pool).end());
    ASSERT_EQ(cbegin, pool.cbegin());
    ASSERT_EQ(cend, pool.cend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin.index(), 0);
    ASSERT_EQ(cend.index(), -1);

    ASSERT_EQ(cbegin++, pool.cbegin());
    ASSERT_EQ(cbegin--, pool.cend());

    ASSERT_EQ(cbegin + 1, pool.cend());
    ASSERT_EQ(cend - 1, pool.cbegin());

    ASSERT_EQ(++cbegin, pool.cend());
    ASSERT_EQ(--cbegin, pool.cbegin());

    ASSERT_EQ(cbegin += 1, pool.cend());
    ASSERT_EQ(cbegin -= 1, pool.cbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), pool.cend());
    ASSERT_EQ(cbegin - (cbegin - cend), pool.cend());

    ASSERT_EQ(cend - (cend - cbegin), pool.cbegin());
    ASSERT_EQ(cend + (cbegin - cend), pool.cbegin());

    ASSERT_EQ(cbegin[0u], *pool.cbegin().operator->());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, pool.cbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, pool.cend());

    ASSERT_EQ(cbegin.index(), 0);
    ASSERT_EQ(cend.index(), -1);

    pool.emplace(entt::entity{3}, 4);
    cbegin = pool.cbegin();

    ASSERT_EQ(cbegin.index(), 1);
    ASSERT_EQ(cend.index(), -1);

    ASSERT_EQ(cbegin[0u], value_type{4});
    ASSERT_EQ(cbegin[1u], value_type{2});
}

TYPED_TEST(Storage, ReverseIterator) {
    using value_type = typename TestFixture::type;
    using reverse_iterator = typename entt::storage<value_type>::reverse_iterator;

    testing::StaticAssertTypeEq<typename reverse_iterator::value_type, value_type>();
    testing::StaticAssertTypeEq<typename reverse_iterator::pointer, value_type *>();
    testing::StaticAssertTypeEq<typename reverse_iterator::reference, value_type &>();

    entt::storage<value_type> pool;
    pool.emplace(entt::entity{1}, 2);

    reverse_iterator end{pool.rbegin()};
    reverse_iterator begin{};

    begin = pool.rend();
    std::swap(begin, end);

    ASSERT_EQ(begin, pool.rbegin());
    ASSERT_EQ(end, pool.rend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 0);

    ASSERT_EQ(begin++, pool.rbegin());
    ASSERT_EQ(begin--, pool.rend());

    ASSERT_EQ(begin + 1, pool.rend());
    ASSERT_EQ(end - 1, pool.rbegin());

    ASSERT_EQ(++begin, pool.rend());
    ASSERT_EQ(--begin, pool.rbegin());

    ASSERT_EQ(begin += 1, pool.rend());
    ASSERT_EQ(begin -= 1, pool.rbegin());

    ASSERT_EQ(begin + (end - begin), pool.rend());
    ASSERT_EQ(begin - (begin - end), pool.rend());

    ASSERT_EQ(end - (end - begin), pool.rbegin());
    ASSERT_EQ(end + (begin - end), pool.rbegin());

    ASSERT_EQ(begin[0u], *pool.rbegin().operator->());

    ASSERT_LT(begin, end);
    ASSERT_LE(begin, pool.rbegin());

    ASSERT_GT(end, begin);
    ASSERT_GE(end, pool.rend());

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 0);

    pool.emplace(entt::entity{3}, 4);
    begin = pool.rbegin();
    end = pool.rend();

    ASSERT_EQ(begin.base().index(), -1);
    ASSERT_EQ(end.base().index(), 1);

    ASSERT_EQ(begin[0u], value_type{2});
    ASSERT_EQ(begin[1u], value_type{4});
}

TYPED_TEST(Storage, ConstReverseIterator) {
    using value_type = typename TestFixture::type;
    using const_reverse_iterator = typename entt::storage<value_type>::const_reverse_iterator;

    testing::StaticAssertTypeEq<typename const_reverse_iterator::value_type, value_type>();
    testing::StaticAssertTypeEq<typename const_reverse_iterator::pointer, const value_type *>();
    testing::StaticAssertTypeEq<typename const_reverse_iterator::reference, const value_type &>();

    entt::storage<value_type> pool;
    pool.emplace(entt::entity{1}, 2);

    const_reverse_iterator cend{pool.crbegin()};
    const_reverse_iterator cbegin{};

    cbegin = pool.crend();
    std::swap(cbegin, cend);

    ASSERT_EQ(cbegin, std::as_const(pool).rbegin());
    ASSERT_EQ(cend, std::as_const(pool).rend());
    ASSERT_EQ(cbegin, pool.crbegin());
    ASSERT_EQ(cend, pool.crend());
    ASSERT_NE(cbegin, cend);

    ASSERT_EQ(cbegin.base().index(), -1);
    ASSERT_EQ(cend.base().index(), 0);

    ASSERT_EQ(cbegin++, pool.crbegin());
    ASSERT_EQ(cbegin--, pool.crend());

    ASSERT_EQ(cbegin + 1, pool.crend());
    ASSERT_EQ(cend - 1, pool.crbegin());

    ASSERT_EQ(++cbegin, pool.crend());
    ASSERT_EQ(--cbegin, pool.crbegin());

    ASSERT_EQ(cbegin += 1, pool.crend());
    ASSERT_EQ(cbegin -= 1, pool.crbegin());

    ASSERT_EQ(cbegin + (cend - cbegin), pool.crend());
    ASSERT_EQ(cbegin - (cbegin - cend), pool.crend());

    ASSERT_EQ(cend - (cend - cbegin), pool.crbegin());
    ASSERT_EQ(cend + (cbegin - cend), pool.crbegin());

    ASSERT_EQ(cbegin[0u], *pool.crbegin().operator->());

    ASSERT_LT(cbegin, cend);
    ASSERT_LE(cbegin, pool.crbegin());

    ASSERT_GT(cend, cbegin);
    ASSERT_GE(cend, pool.crend());

    ASSERT_EQ(cbegin.base().index(), -1);
    ASSERT_EQ(cend.base().index(), 0);

    pool.emplace(entt::entity{3}, 4);
    cbegin = pool.crbegin();
    cend = pool.crend();

    ASSERT_EQ(cbegin.base().index(), -1);
    ASSERT_EQ(cend.base().index(), 1);

    ASSERT_EQ(cbegin[0u], value_type{2});
    ASSERT_EQ(cbegin[1u], value_type{4});
}

TYPED_TEST(Storage, IteratorConversion) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{1}, 2);

    const typename entt::storage<value_type>::iterator it = pool.begin();
    typename entt::storage<value_type>::const_iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), value_type &>();
    testing::StaticAssertTypeEq<decltype(*cit), const value_type &>();

    ASSERT_EQ(*it.operator->(), value_type{2});
    ASSERT_EQ(*it.operator->(), *cit);

    ASSERT_EQ(it - cit, 0);
    ASSERT_EQ(cit - it, 0);
    ASSERT_LE(it, cit);
    ASSERT_LE(cit, it);
    ASSERT_GE(it, cit);
    ASSERT_GE(cit, it);
    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TYPED_TEST(Storage, IteratorPageSizeAwareness) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;

    static_assert(!std::is_same_v<value_type, int> || (traits_type::page_size != entt::component_traits<value_type *>::page_size), "Different page size required");

    for(unsigned int next{}; next < traits_type::page_size; ++next) {
        pool.emplace(entt::entity{next});
    }

    pool.emplace(entt::entity{traits_type::page_size});

    // test the proper use of component traits by the storage iterator
    ASSERT_EQ(&pool.begin()[0], pool.raw()[1u]);
    ASSERT_EQ(&pool.begin()[traits_type::page_size], pool.raw()[0u]);
}

TYPED_TEST(Storage, Getters) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{1};

    pool.emplace(entity, 3);

    testing::StaticAssertTypeEq<decltype(pool.get({})), value_type &>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get({})), const value_type &>();

    testing::StaticAssertTypeEq<decltype(pool.get_as_tuple({})), std::tuple<value_type &>>();
    testing::StaticAssertTypeEq<decltype(std::as_const(pool).get_as_tuple({})), std::tuple<const value_type &>>();

    ASSERT_EQ(pool.get(entity), value_type{3});
    ASSERT_EQ(std::as_const(pool).get(entity), value_type{3});

    ASSERT_EQ(pool.get_as_tuple(entity), std::make_tuple(value_type{3}));
    ASSERT_EQ(std::as_const(pool).get_as_tuple(entity), std::make_tuple(value_type{3}));
}

ENTT_DEBUG_TYPED_TEST(StorageDeathTest, Getters) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{4};

    ASSERT_DEATH([[maybe_unused]] const auto &value = pool.get(entity), "");
    ASSERT_DEATH([[maybe_unused]] const auto &value = std::as_const(pool).get(entity), "");

    ASSERT_DEATH([[maybe_unused]] const auto value = pool.get_as_tuple(entity), "");
    ASSERT_DEATH([[maybe_unused]] const auto value = std::as_const(pool).get_as_tuple(entity), "");
}

TYPED_TEST(Storage, Value) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{2};

    pool.emplace(entity);

    ASSERT_EQ(pool.value(entity), &pool.get(entity));
}

ENTT_DEBUG_TYPED_TEST(StorageDeathTest, Value) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    ASSERT_DEATH([[maybe_unused]] const void *value = pool.value(entt::entity{2}), "");
}

TYPED_TEST(Storage, Emplace) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), value_type &>();

    ASSERT_EQ(pool.emplace(entt::entity{3}), value_type{});
    ASSERT_EQ(pool.emplace(entt::entity{1}, 2), value_type{2});
}

TEST(Storage, EmplaceAggregate) {
    entt::storage<test::aggregate> pool;

    testing::StaticAssertTypeEq<decltype(pool.emplace({})), test::aggregate &>();

    // aggregate types with no args enter the non-aggregate path
    ASSERT_EQ(pool.emplace(entt::entity{3}), test::aggregate{});
    // aggregate types with args work despite the lack of support in the standard library
    ASSERT_EQ(pool.emplace(entt::entity{1}, 2), test::aggregate{2});
}

TEST(Storage, EmplaceSelfMoveSupport) {
    // see #37 - this test shouldn't crash, that's all
    entt::storage<std::unordered_set<int>> pool;
    const entt::entity entity{1};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::swap_and_pop);

    pool.emplace(entity).insert(2);
    pool.erase(entity);

    ASSERT_FALSE(pool.contains(entity));
}

TEST(Storage, EmplaceSelfMoveSupportInPlaceDelete) {
    // see #37 - this test shouldn't crash, that's all
    entt::storage<std::unordered_set<char>> pool;
    const entt::entity entity{1};

    ASSERT_EQ(pool.policy(), entt::deletion_policy::in_place);

    pool.emplace(entity).insert(2);
    pool.erase(entity);

    ASSERT_FALSE(pool.contains(entity));
}

TYPED_TEST(Storage, TryEmplace) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};
    value_type instance{4};

    ASSERT_NE(base.push(entity[0u], &instance), base.end());

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(base.index(entity[0u]), 0u);
    ASSERT_EQ(base.value(entity[0u]), &pool.get(entity[0u]));
    ASSERT_EQ(pool.get(entity[0u]), value_type{4});

    base.erase(entity[0u]);

    ASSERT_NE(base.push(entity.begin(), entity.end()), base.end());

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 3u);
        ASSERT_EQ(base.index(entity[0u]), 1u);
        ASSERT_EQ(base.index(entity[1u]), 2u);
    } else {
        ASSERT_EQ(pool.size(), 2u);
        ASSERT_EQ(base.index(entity[0u]), 0u);
        ASSERT_EQ(base.index(entity[1u]), 1u);
    }

    ASSERT_EQ(pool.get(entity[0u]), value_type{});
    ASSERT_EQ(pool.get(entity[1u]), value_type{});

    base.erase(entity.begin(), entity.end());

    ASSERT_NE(base.push(entity.rbegin(), entity.rend()), base.end());

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 5u);
        ASSERT_EQ(base.index(entity[0u]), 4u);
        ASSERT_EQ(base.index(entity[1u]), 3u);
    } else {
        ASSERT_EQ(pool.size(), 2u);
        ASSERT_EQ(base.index(entity[0u]), 1u);
        ASSERT_EQ(base.index(entity[1u]), 0u);
    }

    ASSERT_EQ(pool.get(entity[0u]), value_type{});
    ASSERT_EQ(pool.get(entity[1u]), value_type{});
}

TEST(Storage, TryEmplaceNonDefaultConstructible) {
    using value_type = std::pair<int &, int &>;
    static_assert(!std::is_default_constructible_v<value_type>, "Default constructible types not allowed");

    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(pool.info(), base.info());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    ASSERT_EQ(base.push(entity[0u]), base.end());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(base.find(entity[0u]), base.end());
    ASSERT_TRUE(pool.empty());

    int value = 4;
    value_type instance{value, value};

    ASSERT_NE(base.push(entity[0u], &instance), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    base.erase(entity[0u]);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(pool.contains(entity[0u]));

    ASSERT_EQ(base.push(entity.begin(), entity.end()), base.end());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(base.find(entity[0u]), base.end());
    ASSERT_EQ(base.find(entity[1u]), base.end());
    ASSERT_TRUE(pool.empty());
}

TEST(Storage, TryEmplaceNonCopyConstructible) {
    using value_type = std::unique_ptr<int>;
    static_assert(!std::is_copy_constructible_v<value_type>, "Copy constructible types not allowed");

    entt::storage<value_type> pool;
    entt::sparse_set &base = pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    ASSERT_EQ(pool.info(), entt::type_id<value_type>());
    ASSERT_EQ(pool.info(), base.info());

    ASSERT_FALSE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    ASSERT_NE(base.push(entity[0u]), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_NE(base.find(entity[0u]), base.end());
    ASSERT_FALSE(pool.empty());

    value_type instance = std::make_unique<int>(4);

    ASSERT_EQ(base.push(entity[1u], &instance), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    base.erase(entity[0u]);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(pool.contains(entity[0u]));

    ASSERT_NE(base.push(entity.begin(), entity.end()), base.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));
    ASSERT_NE(base.find(entity[0u]), base.end());
    ASSERT_NE(base.find(entity[1u]), base.end());
    ASSERT_FALSE(pool.empty());
}

TYPED_TEST(Storage, Patch) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const entt::entity entity{2};

    auto callback = [](auto &&elem) {
        if constexpr(std::is_class_v<std::remove_reference_t<decltype(elem)>>) {
            ++elem.value;
        } else {
            ++elem;
        }
    };

    pool.emplace(entity, 0);

    ASSERT_EQ(pool.get(entity), value_type{0});

    pool.patch(entity);
    pool.patch(entity, callback);
    pool.patch(entity, callback, callback);

    ASSERT_EQ(pool.get(entity), value_type{3});
}

ENTT_DEBUG_TYPED_TEST(StorageDeathTest, Patch) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    ASSERT_DEATH(pool.patch(entt::null), "");
}

TYPED_TEST(Storage, Insert) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{3}};
    typename entt::storage<value_type>::iterator it{};

    it = pool.insert(entity.begin(), entity.end(), value_type{4});

    ASSERT_EQ(it, pool.cbegin());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));

    ASSERT_FALSE(pool.empty());
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.get(entity[0u]), value_type{4});
    ASSERT_EQ(pool.get(entity[1u]), value_type{4});
    ASSERT_EQ(*it++.operator->(), value_type{4});
    ASSERT_EQ(*it.operator->(), value_type{4});

    const std::array value{value_type{3}, value_type{1}};

    pool.erase(entity.begin(), entity.end());
    it = pool.insert(entity.rbegin(), entity.rend(), value.begin());

    ASSERT_EQ(it, pool.cbegin());

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 4u);
        ASSERT_EQ(pool.index(entity[0u]), 3u);
        ASSERT_EQ(pool.index(entity[1u]), 2u);
    } else {
        ASSERT_EQ(pool.size(), 2u);
        ASSERT_EQ(pool.index(entity[0u]), 1u);
        ASSERT_EQ(pool.index(entity[1u]), 0u);
    }

    ASSERT_EQ(pool.get(entity[0u]), value_type{1});
    ASSERT_EQ(pool.get(entity[1u]), value_type{3});
    ASSERT_EQ(*it++.operator->(), value_type{1});
    ASSERT_EQ(*it.operator->(), value_type{3});
}

TYPED_TEST(Storage, Erase) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{3}, entt::entity{2}};
    const std::array value{value_type{1}, value_type{2}, value_type{4}};

    pool.insert(entity.begin(), entity.end(), value.begin());
    pool.erase(entity.begin(), entity.end());

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 3u);
        ASSERT_TRUE(pool.data()[2u] == entt::tombstone);
    } else {
        ASSERT_EQ(pool.size(), 0u);
    }

    pool.insert(entity.begin(), entity.end(), value.begin());
    pool.erase(entity.begin(), entity.begin() + 2u);

    ASSERT_EQ(*pool.begin(), value[2u]);

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 6u);
        ASSERT_EQ(pool.index(entity[2u]), 5u);
    } else {
        ASSERT_EQ(pool.size(), 1u);
    }

    pool.erase(entity[2u]);

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 6u);
        ASSERT_TRUE(pool.data()[5u] == entt::tombstone);
    } else {
        ASSERT_EQ(pool.size(), 0u);
    }
}

TYPED_TEST(Storage, CrossErase) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    entt::sparse_set set;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    pool.emplace(entity[0u], 1);
    pool.emplace(entity[1u], 3);
    set.push(entity[1u]);
    pool.erase(set.begin(), set.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.raw()[0u][0u], value_type{1});
}

TYPED_TEST(Storage, Remove) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{3}, entt::entity{2}};
    const std::array value{value_type{1}, value_type{2}, value_type{4}};

    pool.insert(entity.begin(), entity.end(), value.begin());

    ASSERT_EQ(pool.remove(entity.begin(), entity.end()), 3u);
    ASSERT_EQ(pool.remove(entity.begin(), entity.end()), 0u);

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 3u);
        ASSERT_TRUE(pool.data()[2u] == entt::tombstone);
    } else {
        ASSERT_EQ(pool.size(), 0u);
    }

    pool.insert(entity.begin(), entity.end(), value.begin());

    ASSERT_EQ(pool.remove(entity.begin(), entity.begin() + 2u), 2u);
    ASSERT_EQ(*pool.begin(), value[2u]);

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 6u);
        ASSERT_EQ(pool.index(entity[2u]), 5u);
    } else {
        ASSERT_EQ(pool.size(), 1u);
    }

    ASSERT_TRUE(pool.remove(entity[2u]));
    ASSERT_FALSE(pool.remove(entity[2u]));

    if constexpr(traits_type::in_place_delete) {
        ASSERT_EQ(pool.size(), 6u);
        ASSERT_TRUE(pool.data()[5u] == entt::tombstone);
    } else {
        ASSERT_EQ(pool.size(), 0u);
    }
}

TYPED_TEST(Storage, CrossRemove) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    entt::sparse_set set;
    const std::array entity{entt::entity{1}, entt::entity{3}};

    pool.emplace(entity[0u], 1);
    pool.emplace(entity[1u], 3);
    set.push(entity[1u]);
    pool.remove(set.begin(), set.end());

    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.raw()[0u][0u], value_type{1});
}

TYPED_TEST(Storage, Clear) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{1}, entt::entity{3}, entt::entity{2}};

    pool.insert(entity.begin(), entity.end());

    ASSERT_EQ(pool.size(), 3u);

    pool.clear();

    ASSERT_EQ(pool.size(), 0u);

    pool.insert(entity.begin(), entity.end());
    pool.erase(entity[2u]);

    ASSERT_EQ(pool.size(), 2u + traits_type::in_place_delete);

    pool.clear();

    ASSERT_EQ(pool.size(), 0u);
}

TYPED_TEST(Storage, Compact) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;

    ASSERT_TRUE(pool.empty());

    pool.compact();

    ASSERT_TRUE(pool.empty());

    pool.emplace(entt::entity{0}, value_type{0});
    pool.compact();

    ASSERT_EQ(pool.size(), 1u);

    pool.emplace(entt::entity{4}, value_type{4});
    pool.erase(entt::entity{0});

    ASSERT_EQ(pool.size(), 1u + traits_type::in_place_delete);
    ASSERT_EQ(pool.index(entt::entity{4}), traits_type::in_place_delete);
    ASSERT_EQ(pool.get(entt::entity{4}), value_type{4});

    pool.compact();

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_EQ(pool.index(entt::entity{4}), 0u);
    ASSERT_EQ(pool.get(entt::entity{4}), value_type{4});

    pool.emplace(entt::entity{0}, value_type{0});
    pool.compact();

    ASSERT_EQ(pool.size(), 2u);
    ASSERT_EQ(pool.index(entt::entity{4}), 0u);
    ASSERT_EQ(pool.index(entt::entity{0}), 1u);
    ASSERT_EQ(pool.get(entt::entity{4}), value_type{4});
    ASSERT_EQ(pool.get(entt::entity{0}), value_type{0});

    pool.erase(entt::entity{0});
    pool.erase(entt::entity{4});
    pool.compact();

    ASSERT_TRUE(pool.empty());
}

TYPED_TEST(Storage, SwapElements) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{1}, 1);
    pool.emplace(entt::entity{2}, 3);
    pool.emplace(entt::entity{4}, 8);

    pool.erase(entt::entity{2});

    ASSERT_EQ(pool.get(entt::entity{1}), value_type{1});
    ASSERT_EQ(pool.get(entt::entity{4}), value_type{8});
    ASSERT_EQ(pool.index(entt::entity{1}), 0u);
    ASSERT_EQ(pool.index(entt::entity{4}), 1u + traits_type::in_place_delete);

    pool.swap_elements(entt::entity{1}, entt::entity{4});

    ASSERT_EQ(pool.get(entt::entity{1}), value_type{1});
    ASSERT_EQ(pool.get(entt::entity{4}), value_type{8});
    ASSERT_EQ(pool.index(entt::entity{1}), 1u + traits_type::in_place_delete);
    ASSERT_EQ(pool.index(entt::entity{4}), 0u);
}

TYPED_TEST(Storage, Iterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity, value_type &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, value_type &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 2);
    pool.emplace(entt::entity{3}, 4);

    auto iterable = pool.each();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.begin());
    ASSERT_EQ(end.base(), base.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{3});
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), value_type{4});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{3});
    ASSERT_EQ(std::get<1>(*begin), value_type{4});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.begin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.end());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), value_type &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == value_type{2});
        ASSERT_TRUE(entity != entt::entity{3} || element == value_type{4});
    }
}

TYPED_TEST(Storage, ConstIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::const_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity, const value_type &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, const value_type &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 2);
    pool.emplace(entt::entity{3}, 4);

    auto iterable = std::as_const(pool).each();

    iterator end{iterable.cbegin()};
    iterator begin{};

    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.begin());
    ASSERT_EQ(end.base(), base.end());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{3});
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), value_type{4});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{3});
    ASSERT_EQ(std::get<1>(*begin), value_type{4});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.begin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.end());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), const value_type &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == value_type{2});
        ASSERT_TRUE(entity != entt::entity{3} || element == value_type{4});
    }
}

TYPED_TEST(Storage, IterableIteratorConversion) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3}, 1);

    const typename entt::storage<value_type>::iterable::iterator it = pool.each().begin();
    typename entt::storage<value_type>::const_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity, value_type &>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity, const value_type &>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TYPED_TEST(Storage, IterableAlgorithmCompatibility) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3}, 1);

    const auto iterable = pool.each();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TYPED_TEST(Storage, ReverseIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity, value_type &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, value_type &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 2);
    pool.emplace(entt::entity{3}, 4);

    auto iterable = pool.reach();

    iterator end{iterable.begin()};
    iterator begin{};

    begin = iterable.end();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.begin());
    ASSERT_EQ(end, iterable.end());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.rbegin());
    ASSERT_EQ(end.base(), base.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), value_type{2});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});
    ASSERT_EQ(std::get<1>(*begin), value_type{2});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.rbegin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.rend());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), value_type &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == value_type{2});
        ASSERT_TRUE(entity != entt::entity{3} || element == value_type{4});
    }
}

TYPED_TEST(Storage, ConstReverseIterable) {
    using value_type = typename TestFixture::type;
    using iterator = typename entt::storage<value_type>::const_reverse_iterable::iterator;

    testing::StaticAssertTypeEq<typename iterator::value_type, std::tuple<entt::entity, const value_type &>>();
    testing::StaticAssertTypeEq<typename iterator::pointer, entt::input_iterator_pointer<std::tuple<entt::entity, const value_type &>>>();
    testing::StaticAssertTypeEq<typename iterator::reference, typename iterator::value_type>();

    entt::storage<value_type> pool;
    const entt::sparse_set &base = pool;

    pool.emplace(entt::entity{1}, 2);
    pool.emplace(entt::entity{3}, 4);

    auto iterable = std::as_const(pool).reach();

    iterator end{iterable.cbegin()};
    iterator begin{};

    begin = iterable.cend();
    std::swap(begin, end);

    ASSERT_EQ(begin, iterable.cbegin());
    ASSERT_EQ(end, iterable.cend());
    ASSERT_NE(begin, end);

    ASSERT_EQ(begin.base(), base.rbegin());
    ASSERT_EQ(end.base(), base.rend());

    ASSERT_EQ(std::get<0>(*begin.operator->().operator->()), entt::entity{1});
    ASSERT_EQ(std::get<1>(*begin.operator->().operator->()), value_type{2});
    ASSERT_EQ(std::get<0>(*begin), entt::entity{1});
    ASSERT_EQ(std::get<1>(*begin), value_type{2});

    ASSERT_EQ(begin++, iterable.begin());
    ASSERT_EQ(begin.base(), ++base.rbegin());
    ASSERT_EQ(++begin, iterable.end());
    ASSERT_EQ(begin.base(), base.rend());

    for(auto [entity, element]: iterable) {
        testing::StaticAssertTypeEq<decltype(entity), entt::entity>();
        testing::StaticAssertTypeEq<decltype(element), const value_type &>();
        ASSERT_TRUE(entity != entt::entity{1} || element == value_type{2});
        ASSERT_TRUE(entity != entt::entity{3} || element == value_type{4});
    }
}

TYPED_TEST(Storage, ReverseIterableIteratorConversion) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3}, 1);

    const typename entt::storage<value_type>::reverse_iterable::iterator it = pool.reach().begin();
    typename entt::storage<value_type>::const_reverse_iterable::iterator cit = it;

    testing::StaticAssertTypeEq<decltype(*it), std::tuple<entt::entity, value_type &>>();
    testing::StaticAssertTypeEq<decltype(*cit), std::tuple<entt::entity, const value_type &>>();

    ASSERT_EQ(it, cit);
    ASSERT_NE(++cit, it);
}

TYPED_TEST(Storage, ReverseIterableAlgorithmCompatibility) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{3}, 1);

    const auto iterable = pool.reach();
    const auto it = std::find_if(iterable.begin(), iterable.end(), [](auto args) { return std::get<0>(args) == entt::entity{3}; });

    ASSERT_EQ(std::get<0>(*it), entt::entity{3});
}

TYPED_TEST(Storage, SortOrdered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{8}, entt::entity{16}, entt::entity{2}, entt::entity{1}, entt::entity{4}};
    const std::array value{value_type{8}, value_type{4}, value_type{2}, value_type{1}, value_type{0}};

    pool.insert(entity.begin(), entity.end(), value.begin());
    pool.sort([&pool](auto lhs, auto rhs) { return pool.get(lhs) < pool.get(rhs); });

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(value.rbegin(), value.rend(), pool.begin(), pool.end()));
}

TYPED_TEST(Storage, SortReverse) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{8}, entt::entity{16}, entt::entity{2}, entt::entity{1}, entt::entity{4}};
    const std::array value{value_type{0}, value_type{1}, value_type{2}, value_type{4}, value_type{8}};

    pool.insert(entity.begin(), entity.end(), value.begin());
    pool.sort([&pool](auto lhs, auto rhs) { return pool.get(lhs) < pool.get(rhs); });

    ASSERT_TRUE(std::equal(entity.begin(), entity.end(), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(value.begin(), value.end(), pool.begin(), pool.end()));
}

TYPED_TEST(Storage, SortUnordered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{8}, entt::entity{16}, entt::entity{2}, entt::entity{1}, entt::entity{4}};
    const std::array value{value_type{2}, value_type{1}, value_type{0}, value_type{4}, value_type{8}};

    pool.insert(entity.begin(), entity.end(), value.begin());
    pool.sort([&pool](auto lhs, auto rhs) { return pool.get(lhs) < pool.get(rhs); });

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ(*(begin++), value[2u]);
    ASSERT_EQ(*(begin++), value[1u]);
    ASSERT_EQ(*(begin++), value[0u]);
    ASSERT_EQ(*(begin++), value[3u]);
    ASSERT_EQ(*(begin++), value[4u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(pool.data()[0u], entity[4u]);
    ASSERT_EQ(pool.data()[1u], entity[3u]);
    ASSERT_EQ(pool.data()[2u], entity[0u]);
    ASSERT_EQ(pool.data()[3u], entity[1u]);
    ASSERT_EQ(pool.data()[4u], entity[2u]);
}

TYPED_TEST(Storage, SortN) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;
    const std::array entity{entt::entity{8}, entt::entity{16}, entt::entity{2}, entt::entity{1}, entt::entity{4}};
    const std::array value{value_type{1}, value_type{2}, value_type{0}, value_type{4}, value_type{8}};

    pool.insert(entity.begin(), entity.end(), value.begin());
    pool.sort_n(0u, [&pool](auto lhs, auto rhs) { return pool.get(lhs) < pool.get(rhs); });

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), pool.entt::sparse_set::begin(), pool.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(value.rbegin(), value.rend(), pool.begin(), pool.end()));

    pool.sort_n(2u, [&pool](auto lhs, auto rhs) { return pool.get(lhs) < pool.get(rhs); });

    ASSERT_EQ(pool.raw()[0u][0u], value[1u]);
    ASSERT_EQ(pool.raw()[0u][1u], value[0u]);
    ASSERT_EQ(pool.raw()[0u][2u], value[2u]);

    ASSERT_EQ(pool.data()[0u], entity[1u]);
    ASSERT_EQ(pool.data()[1u], entity[0u]);
    ASSERT_EQ(pool.data()[2u], entity[2u]);

    const auto length = 5u;
    pool.sort_n(length, [&pool](auto lhs, auto rhs) { return pool.get(lhs) < pool.get(rhs); });

    auto begin = pool.begin();
    auto end = pool.end();

    ASSERT_EQ(*(begin++), value[2u]);
    ASSERT_EQ(*(begin++), value[0u]);
    ASSERT_EQ(*(begin++), value[1u]);
    ASSERT_EQ(*(begin++), value[3u]);
    ASSERT_EQ(*(begin++), value[4u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(pool.data()[0u], entity[4u]);
    ASSERT_EQ(pool.data()[1u], entity[3u]);
    ASSERT_EQ(pool.data()[2u], entity[1u]);
    ASSERT_EQ(pool.data()[3u], entity[0u]);
    ASSERT_EQ(pool.data()[4u], entity[2u]);
}

TYPED_TEST(Storage, SortAsDisjoint) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    const entt::storage<value_type> rhs;
    const std::array entity{entt::entity{1}, entt::entity{2}, entt::entity{4}};
    const std::array value{value_type{0}, value_type{1}, value_type{2}};

    lhs.insert(entity.begin(), entity.end(), value.begin());

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(value.rbegin(), value.rend(), lhs.begin(), lhs.end()));

    lhs.sort_as(rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end());

    ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(value.rbegin(), value.rend(), lhs.begin(), lhs.end()));
}

TYPED_TEST(Storage, SortAsOverlap) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}};
    const std::array lhs_value{value_type{0}, value_type{1}, value_type{2}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end(), lhs_value.begin());

    const std::array rhs_entity{entt::entity{2}};
    const std::array rhs_value{value_type{1}};

    rhs.insert(rhs_entity.begin(), rhs_entity.end(), rhs_value.begin());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(lhs_value.rbegin(), lhs_value.rend(), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(rhs_value.rbegin(), rhs_value.rend(), rhs.begin(), rhs.end()));

    lhs.sort_as(rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end());

    auto begin = lhs.begin();
    auto end = lhs.end();

    ASSERT_EQ(*(begin++), lhs_value[1u]);
    ASSERT_EQ(*(begin++), lhs_value[2u]);
    ASSERT_EQ(*(begin++), lhs_value[0u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(lhs.data()[0u], lhs_entity[0u]);
    ASSERT_EQ(lhs.data()[1u], lhs_entity[2u]);
    ASSERT_EQ(lhs.data()[2u], lhs_entity[1u]);
}

TYPED_TEST(Storage, SortAsOrdered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array lhs_value{value_type{0}, value_type{1}, value_type{2}, value_type{4}, value_type{8}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end(), lhs_value.begin());

    const std::array rhs_entity{entt::entity{32}, entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_value{value_type{16}, value_type{0}, value_type{1}, value_type{2}, value_type{4}, value_type{8}};

    rhs.insert(rhs_entity.begin(), rhs_entity.end(), rhs_value.begin());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(lhs_value.rbegin(), lhs_value.rend(), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(rhs_value.rbegin(), rhs_value.rend(), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end());

    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(rhs_value.rbegin(), rhs_value.rend(), rhs.begin(), rhs.end()));
}

TYPED_TEST(Storage, SortAsReverse) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array lhs_value{value_type{0}, value_type{1}, value_type{2}, value_type{4}, value_type{8}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end(), lhs_value.begin());

    const std::array rhs_entity{entt::entity{16}, entt::entity{8}, entt::entity{4}, entt::entity{2}, entt::entity{1}, entt::entity{32}};
    const std::array rhs_value{value_type{8}, value_type{4}, value_type{2}, value_type{1}, value_type{0}, value_type{16}};

    rhs.insert(rhs_entity.begin(), rhs_entity.end(), rhs_value.begin());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(lhs_value.rbegin(), lhs_value.rend(), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(rhs_value.rbegin(), rhs_value.rend(), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end());

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), rhs_value[0u]);
    ASSERT_EQ(*(begin++), rhs_value[1u]);
    ASSERT_EQ(*(begin++), rhs_value[2u]);
    ASSERT_EQ(*(begin++), rhs_value[3u]);
    ASSERT_EQ(*(begin++), rhs_value[4u]);
    ASSERT_EQ(*(begin++), rhs_value[5u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(rhs.data()[0u], rhs_entity[5u]);
    ASSERT_EQ(rhs.data()[1u], rhs_entity[4u]);
    ASSERT_EQ(rhs.data()[2u], rhs_entity[3u]);
    ASSERT_EQ(rhs.data()[3u], rhs_entity[2u]);
    ASSERT_EQ(rhs.data()[4u], rhs_entity[1u]);
    ASSERT_EQ(rhs.data()[5u], rhs_entity[0u]);
}

TYPED_TEST(Storage, SortAsUnordered) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> lhs;
    entt::storage<value_type> rhs;
    const std::array lhs_entity{entt::entity{1}, entt::entity{2}, entt::entity{4}, entt::entity{8}, entt::entity{16}};
    const std::array lhs_value{value_type{0}, value_type{1}, value_type{2}, value_type{4}, value_type{8}};

    lhs.insert(lhs_entity.begin(), lhs_entity.end(), lhs_value.begin());

    const std::array rhs_entity{entt::entity{4}, entt::entity{2}, entt::entity{32}, entt::entity{1}, entt::entity{8}, entt::entity{16}};
    const std::array rhs_value{value_type{2}, value_type{1}, value_type{16}, value_type{0}, value_type{4}, value_type{8}};

    rhs.insert(rhs_entity.begin(), rhs_entity.end(), rhs_value.begin());

    ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(lhs_value.rbegin(), lhs_value.rend(), lhs.begin(), lhs.end()));

    ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.entt::sparse_set::begin(), rhs.entt::sparse_set::end()));
    ASSERT_TRUE(std::equal(rhs_value.rbegin(), rhs_value.rend(), rhs.begin(), rhs.end()));

    rhs.sort_as(lhs.entt::sparse_set::begin(), lhs.entt::sparse_set::end());

    auto begin = rhs.begin();
    auto end = rhs.end();

    ASSERT_EQ(*(begin++), rhs_value[5u]);
    ASSERT_EQ(*(begin++), rhs_value[4u]);
    ASSERT_EQ(*(begin++), rhs_value[0u]);
    ASSERT_EQ(*(begin++), rhs_value[1u]);
    ASSERT_EQ(*(begin++), rhs_value[3u]);
    ASSERT_EQ(*(begin++), rhs_value[2u]);
    ASSERT_EQ(begin, end);

    ASSERT_EQ(rhs.data()[0u], rhs_entity[2u]);
    ASSERT_EQ(rhs.data()[1u], rhs_entity[3u]);
    ASSERT_EQ(rhs.data()[2u], rhs_entity[1u]);
    ASSERT_EQ(rhs.data()[3u], rhs_entity[0u]);
    ASSERT_EQ(rhs.data()[4u], rhs_entity[4u]);
    ASSERT_EQ(rhs.data()[5u], rhs_entity[5u]);
}

TEST(Storage, MoveOnlyComponent) {
    using value_type = std::unique_ptr<int>;
    static_assert(!std::is_copy_assignable_v<value_type>, "Copy assignable types not allowed");
    static_assert(std::is_move_assignable_v<value_type>, "Move assignable type required");
    // the purpose is to ensure that move only types are always accepted
    [[maybe_unused]] const entt::storage<value_type> pool;
}

TEST(Storage, NonMovableComponent) {
    using value_type = std::pair<const int, const int>;
    static_assert(!std::is_move_assignable_v<value_type>, "Move assignable types not allowed");
    // the purpose is to ensure that non-movable types are always accepted
    [[maybe_unused]] const entt::storage<value_type> pool;
}

ENTT_DEBUG_TEST(StorageDeathTest, NonMovableComponent) {
    entt::storage<std::pair<const int, const int>> pool;
    const entt::entity entity{0};
    const entt::entity destroy{1};
    const entt::entity other{2};

    pool.emplace(entity);
    pool.emplace(destroy);
    pool.emplace(other);

    pool.erase(destroy);

    ASSERT_DEATH(pool.swap_elements(entity, other), "");
    ASSERT_DEATH(pool.compact(), "");
    ASSERT_DEATH(pool.sort([](auto &&lhs, auto &&rhs) { return lhs < rhs; }), "");
}

TYPED_TEST(Storage, CanModifyDuringIteration) {
    using value_type = typename TestFixture::type;
    using traits_type = entt::component_traits<value_type>;

    entt::storage<value_type> pool;
    auto *ptr = &pool.emplace(entt::entity{0}, 2);

    ASSERT_EQ(pool.capacity(), traits_type::page_size);

    const auto it = pool.cbegin();
    pool.reserve(traits_type::page_size + 1u);

    ASSERT_EQ(pool.capacity(), 2 * traits_type::page_size);
    ASSERT_EQ(&pool.get(entt::entity{0}), ptr);

    // this should crash with asan enabled if we break the constraint
    [[maybe_unused]] const auto &value = *it;
}

TYPED_TEST(Storage, ReferencesGuaranteed) {
    using value_type = typename TestFixture::type;

    entt::storage<value_type> pool;

    pool.emplace(entt::entity{0}, 0);
    pool.emplace(entt::entity{1}, 1);

    ASSERT_EQ(pool.get(entt::entity{0}), value_type{0});
    ASSERT_EQ(pool.get(entt::entity{1}), value_type{1});

    for(auto &&elem: pool) {
        if(!(elem == value_type{})) {
            elem = value_type{4};
        }
    }

    ASSERT_EQ(pool.get(entt::entity{0}), value_type{0});
    ASSERT_EQ(pool.get(entt::entity{1}), value_type{4});

    auto begin = pool.begin();

    while(begin != pool.end()) {
        *(begin++) = value_type{3};
    }

    ASSERT_EQ(pool.get(entt::entity{0}), value_type{3});
    ASSERT_EQ(pool.get(entt::entity{1}), value_type{3});
}

TEST(Storage, UpdateFromDestructor) {
    constexpr auto size = 10u;
    const std::array entity{entt::entity{4u}, entt::entity{2u}, entt::entity{0u}};

    for(auto target: entity) {
        entt::storage<update_from_destructor> pool;

        for(std::size_t next{}; next < size; ++next) {
            const auto other = entt::entity(next);
            pool.emplace(other, pool, other == entt::entity(size / 2) ? target : other);
        }

        pool.erase(entt::entity(size / 2));

        ASSERT_EQ(pool.size(), size - 1u - (target != entt::null));
        ASSERT_FALSE(pool.contains(entt::entity(size / 2)));
        ASSERT_FALSE(pool.contains(target));

        pool.clear();

        ASSERT_TRUE(pool.empty());

        for(std::size_t next{}; next < size; ++next) {
            ASSERT_FALSE(pool.contains(entt::entity(next)));
        }
    }
}

TEST(Storage, CreateFromConstructor) {
    entt::storage<create_from_constructor> pool;
    const entt::entity entity{0u};
    const entt::entity other{1u};

    pool.emplace(entity, pool, other);

    ASSERT_EQ(pool.get(entity).child, other);
    ASSERT_EQ(pool.get(other).child, static_cast<entt::entity>(entt::null));
}

TEST(Storage, ClassLevelNewDelete) {
    entt::storage<test::new_delete> pool;
    const entt::entity entity{0u};

    // yeah, that's for code coverage purposes only :)
    pool.emplace(entity, *std::make_unique<test::new_delete>(test::new_delete{3}));

    ASSERT_EQ(pool.get(entity).value, 3);
}

TYPED_TEST(Storage, CustomAllocator) {
    using value_type = typename TestFixture::type;

    const test::throwing_allocator<entt::entity> allocator{};
    entt::basic_storage<value_type, entt::entity, test::throwing_allocator<value_type>> pool{allocator};

    pool.reserve(1u);

    ASSERT_NE(pool.capacity(), 0u);

    pool.emplace(entt::entity{0});
    pool.emplace(entt::entity{1});

    decltype(pool) other{std::move(pool), allocator};

    test::is_initialized(pool);

    ASSERT_TRUE(pool.empty());
    ASSERT_FALSE(other.empty());
    ASSERT_NE(other.capacity(), 0u);
    ASSERT_EQ(other.size(), 2u);

    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_NE(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 2u);

    other = {};
    pool.swap(other);
    pool = std::move(other);
    test::is_initialized(other);

    ASSERT_FALSE(pool.empty());
    ASSERT_TRUE(other.empty());
    ASSERT_NE(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 2u);

    pool.clear();

    ASSERT_NE(pool.capacity(), 0u);
    ASSERT_EQ(pool.size(), 0u);
}

TYPED_TEST(Storage, ThrowingAllocator) {
    using value_type = typename TestFixture::type;

    entt::basic_storage<value_type, entt::entity, test::throwing_allocator<value_type>> pool{};
    typename std::decay_t<decltype(pool)>::base_type &base = pool;

    constexpr auto packed_page_size = entt::component_traits<value_type>::page_size;
    constexpr auto sparse_page_size = entt::entt_traits<entt::entity>::page_size;

    pool.get_allocator().template throw_counter<value_type>(0u);

    ASSERT_THROW(pool.reserve(1u), test::throwing_allocator_exception);
    ASSERT_EQ(pool.capacity(), 0u);

    pool.get_allocator().template throw_counter<value_type>(1u);

    ASSERT_THROW(pool.reserve(2 * packed_page_size), test::throwing_allocator_exception);
    ASSERT_EQ(pool.capacity(), packed_page_size);

    pool.shrink_to_fit();

    ASSERT_EQ(pool.capacity(), 0u);

    pool.get_allocator().template throw_counter<entt::entity>(0u);

    ASSERT_THROW(pool.emplace(entt::entity{0}, 0), test::throwing_allocator_exception);
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_TRUE(pool.empty());

    pool.get_allocator().template throw_counter<entt::entity>(0u);

    ASSERT_THROW(base.push(entt::entity{0}), test::throwing_allocator_exception);
    ASSERT_FALSE(base.contains(entt::entity{0}));
    ASSERT_TRUE(base.empty());

    pool.get_allocator().template throw_counter<value_type>(0u);

    ASSERT_THROW(pool.emplace(entt::entity{0}, 0), test::throwing_allocator_exception);
    ASSERT_FALSE(pool.contains(entt::entity{0}));
    ASSERT_NO_THROW(pool.compact());
    ASSERT_TRUE(pool.empty());

    pool.emplace(entt::entity{0}, 0);
    const std::array entity{entt::entity{1}, entt::entity{sparse_page_size}};
    pool.get_allocator().template throw_counter<entt::entity>(1u);

    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), value_type{0}), test::throwing_allocator_exception);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));

    pool.erase(entity[0u]);
    const std::array component{value_type{1}, value_type{sparse_page_size}};
    pool.get_allocator().template throw_counter<entt::entity>(0u);
    pool.compact();

    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), component.begin()), test::throwing_allocator_exception);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
}

TEST(Storage, ThrowingComponent) {
    entt::storage<test::throwing_type> pool;
    const std::array entity{entt::entity{4}, entt::entity{1}};
    const std::array value{test::throwing_type{true}, test::throwing_type{false}};

    // strong exception safety
    ASSERT_THROW(pool.emplace(entity[0u], value[0u]), test::throwing_type_exception);
    ASSERT_TRUE(pool.empty());

    // basic exception safety
    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), value[0u]), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entity[1u]));

    // basic exception safety
    ASSERT_THROW(pool.insert(entity.begin(), entity.end(), value.begin()), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 0u);
    ASSERT_FALSE(pool.contains(entity[1u]));

    // basic exception safety
    ASSERT_THROW(pool.insert(entity.rbegin(), entity.rend(), value.rbegin()), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.get(entity[1u]), value[1u]);

    pool.clear();
    pool.emplace(entity[1u], value[0u].throw_on_copy());
    pool.emplace(entity[0u], value[1u].throw_on_copy());

    // basic exception safety
    ASSERT_THROW(pool.erase(entity[1u]), test::throwing_type_exception);
    ASSERT_EQ(pool.size(), 2u);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_TRUE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.index(entity[0u]), 1u);
    ASSERT_EQ(pool.index(entity[1u]), 0u);
    ASSERT_EQ(pool.get(entity[0u]), value[1u]);
    // the element may have been moved but it's still there
    ASSERT_EQ(pool.get(entity[1u]), value[0u]);

    pool.get(entity[1u]).throw_on_copy(false);
    pool.erase(entity[1u]);

    ASSERT_EQ(pool.size(), 1u);
    ASSERT_TRUE(pool.contains(entity[0u]));
    ASSERT_FALSE(pool.contains(entity[1u]));
    ASSERT_EQ(pool.index(entity[0u]), 0u);
    ASSERT_EQ(pool.get(entity[0u]), value[1u]);
}

#if defined(ENTT_HAS_TRACKED_MEMORY_RESOURCE)

TYPED_TEST(Storage, NoUsesAllocatorConstruction) {
    using value_type = typename TestFixture::type;

    test::tracked_memory_resource memory_resource{};
    entt::basic_storage<value_type, entt::entity, std::pmr::polymorphic_allocator<value_type>> pool{&memory_resource};
    const entt::entity entity{2};

    pool.emplace(entity);
    pool.erase(entity);
    memory_resource.reset();
    pool.emplace(entity, 0);

    ASSERT_TRUE(pool.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_EQ(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

TEST(Storage, UsesAllocatorConstruction) {
    using string_type = typename test::tracked_memory_resource::string_type;

    test::tracked_memory_resource memory_resource{};
    entt::basic_storage<string_type, entt::entity, std::pmr::polymorphic_allocator<string_type>> pool{&memory_resource};
    const entt::entity entity{2};

    pool.emplace(entity);
    pool.erase(entity);
    memory_resource.reset();
    pool.emplace(entity, test::tracked_memory_resource::default_value);

    ASSERT_TRUE(pool.get_allocator().resource()->is_equal(memory_resource));
    ASSERT_GT(memory_resource.do_allocate_counter(), 0u);
    ASSERT_EQ(memory_resource.do_deallocate_counter(), 0u);
}

#endif
