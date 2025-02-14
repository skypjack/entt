#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>
#include <entt/config/config.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/sparse_set.hpp>
#include "../../common/config.h"
#include "../../common/entity.h"
#include "../../common/linter.hpp"
#include "../../common/throwing_allocator.hpp"

struct entity_traits {
    using value_type = test::entity;
    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;
    static constexpr entity_type entity_mask = 0x3FFFF; // 18b
    static constexpr entity_type version_mask = 0x0FFF; // 12b
};

template<>
struct entt::entt_traits<test::entity>: entt::basic_entt_traits<entity_traits> {
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};

template<typename Type>
struct SparseSet: testing::Test {
    using type = Type;

    inline static const std::array<entt::deletion_policy, 3u> deletion_policy{
        entt::deletion_policy::swap_and_pop,
        entt::deletion_policy::in_place,
        entt::deletion_policy::swap_only,
    };
};

template<typename Type>
using SparseSetDeathTest = SparseSet<Type>;

using SparseSetTypes = ::testing::Types<entt::entity, test::entity>;

TYPED_TEST_SUITE(SparseSet, SparseSetTypes, );
TYPED_TEST_SUITE(SparseSetDeathTest, SparseSetTypes, );

TYPED_TEST(SparseSet, Constructors) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using allocator_type = typename sparse_set_type::allocator_type;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{};

        ASSERT_EQ(set.policy(), entt::deletion_policy::swap_and_pop);
        ASSERT_NO_THROW([[maybe_unused]] auto alloc = set.get_allocator());
        ASSERT_EQ(set.type(), entt::type_id<void>());

        set = sparse_set_type{allocator_type{}};

        ASSERT_EQ(set.policy(), entt::deletion_policy::swap_and_pop);
        ASSERT_NO_THROW([[maybe_unused]] auto alloc = set.get_allocator());
        ASSERT_EQ(set.type(), entt::type_id<void>());

        set = sparse_set_type{policy, allocator_type{}};

        ASSERT_EQ(set.policy(), policy);
        ASSERT_NO_THROW([[maybe_unused]] auto alloc = set.get_allocator());
        ASSERT_EQ(set.type(), entt::type_id<void>());

        set = sparse_set_type{entt::type_id<int>(), policy, allocator_type{}};

        ASSERT_EQ(set.policy(), policy);
        ASSERT_NO_THROW([[maybe_unused]] auto alloc = set.get_allocator());
        ASSERT_EQ(set.type(), entt::type_id<int>());
    }
}

TYPED_TEST(SparseSet, Move) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using allocator_type = typename sparse_set_type::allocator_type;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        set.push(entity_type{1});

        static_assert(std::is_move_constructible_v<decltype(set)>, "Move constructible type required");
        static_assert(std::is_move_assignable_v<decltype(set)>, "Move assignable type required");

        sparse_set_type other{std::move(set)};

        test::is_initialized(set);

        ASSERT_TRUE(set.empty());
        ASSERT_FALSE(other.empty());

        ASSERT_EQ(other.policy(), policy);
        ASSERT_EQ(other.index(entity_type{1}), 0u);

        sparse_set_type extended{std::move(other), allocator_type{}};

        test::is_initialized(other);

        ASSERT_TRUE(other.empty());
        ASSERT_FALSE(extended.empty());

        ASSERT_EQ(extended.policy(), policy);
        ASSERT_EQ(extended.index(entity_type{1}), 0u);

        set = std::move(extended);
        test::is_initialized(extended);

        ASSERT_FALSE(set.empty());
        ASSERT_TRUE(other.empty());
        ASSERT_TRUE(extended.empty());

        ASSERT_EQ(set.policy(), policy);
        ASSERT_EQ(set.index(entity_type{1}), 0u);

        other = sparse_set_type{policy};
        other.push(entity_type{3});
        other = std::move(set);
        test::is_initialized(set);

        ASSERT_FALSE(set.empty());
        ASSERT_FALSE(other.empty());

        ASSERT_EQ(other.policy(), policy);
        ASSERT_EQ(other.index(entity_type{1}), 0u);
    }
}

TYPED_TEST(SparseSet, Swap) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};
        sparse_set_type other{entt::deletion_policy::in_place};

        ASSERT_EQ(set.policy(), policy);
        ASSERT_EQ(other.policy(), entt::deletion_policy::in_place);

        set.push(entity_type{4});

        other.push(entity_type{3});
        other.push(entity_type{1});
        other.erase(entity_type{3});

        ASSERT_EQ(set.size(), 1u);
        ASSERT_EQ(other.size(), 2u);

        set.swap(other);

        ASSERT_EQ(set.policy(), entt::deletion_policy::in_place);
        ASSERT_EQ(other.policy(), policy);

        ASSERT_EQ(set.size(), 2u);
        ASSERT_EQ(other.size(), 1u);

        ASSERT_EQ(set.index(entity_type{1}), 1u);
        ASSERT_EQ(other.index(entity_type{4}), 0u);
    }
}

TYPED_TEST(SparseSet, FreeList) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{1};
        const entity_type other{2};

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::to_entity(entt::tombstone));

            set.push(other);
            set.push(entity);
            set.erase(other);

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.free_list(), traits_type::to_entity(entt::tombstone));

            set.clear();

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::to_entity(entt::tombstone));
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::to_entity(entt::tombstone));

            set.push(other);
            set.push(entity);
            set.erase(other);

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.free_list(), 0u);

            set.clear();

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::to_entity(entt::tombstone));
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), 0u);

            set.push(other);
            set.push(entity);
            set.erase(other);

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.free_list(), 1u);

            set.free_list(0u);

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.free_list(), 0u);

            set.free_list(2u);

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.free_list(), 2u);

            set.clear();

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), 0u);
        } break;
        }
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, FreeList) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        set.push(entity_type{3});

        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
        case entt::deletion_policy::in_place: {
            ASSERT_DEATH(set.free_list(0u), "");
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_NO_THROW(set.free_list(0u));
            ASSERT_NO_THROW(set.free_list(1u));
            ASSERT_DEATH(set.free_list(2u), "");
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Capacity) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        set.reserve(64u);

        ASSERT_EQ(set.capacity(), 64u);
        ASSERT_TRUE(set.empty());

        set.reserve(0u);

        ASSERT_EQ(set.capacity(), 64u);
        ASSERT_TRUE(set.empty());
    }
}

TYPED_TEST(SparseSet, ShrinkToFit) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        ASSERT_EQ(set.capacity(), 0u);
        ASSERT_EQ(set.extent(), 0u);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            set.push(entity_type{traits_type::page_size - 1u});
            set.push(entity_type{traits_type::page_size});

            set.erase(entity_type{traits_type::page_size - 1u});

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            set.shrink_to_fit();

            ASSERT_EQ(set.capacity(), 1u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_TRUE(set.contains(entity_type{traits_type::page_size}));

            set.push(entity_type{traits_type::page_size - 1u});
            set.erase(entity_type{traits_type::page_size});

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            set.shrink_to_fit();

            ASSERT_EQ(set.capacity(), 1u);
            ASSERT_EQ(set.extent(), traits_type::page_size);

            ASSERT_TRUE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));

            set.erase(entity_type{traits_type::page_size - 1u});

            set.shrink_to_fit();

            ASSERT_EQ(set.capacity(), 0u);
            ASSERT_EQ(set.extent(), 0u);

            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));
        } break;
        case entt::deletion_policy::in_place: {
            set.push(entity_type{traits_type::page_size - 1u});
            set.push(entity_type{traits_type::page_size});

            set.erase(entity_type{traits_type::page_size - 1u});

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            set.shrink_to_fit();

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_TRUE(set.contains(entity_type{traits_type::page_size}));

            set.push(entity_type{traits_type::page_size - 1u});
            set.erase(entity_type{traits_type::page_size});

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            set.shrink_to_fit();

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), traits_type::page_size);

            ASSERT_TRUE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));

            set.erase(entity_type{traits_type::page_size - 1u});

            set.shrink_to_fit();

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 0u);

            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));
        } break;
        case entt::deletion_policy::swap_only: {
            set.push(entity_type{traits_type::page_size - 1u});
            set.push(entity_type{traits_type::page_size});

            set.erase(entity_type{traits_type::page_size - 1u});

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            set.shrink_to_fit();

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_TRUE(set.contains(entity_type{traits_type::page_size}));

            set.push(entity_type{traits_type::page_size - 1u});
            set.erase(entity_type{traits_type::page_size});

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            set.shrink_to_fit();

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            ASSERT_TRUE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));

            set.erase(entity_type{traits_type::page_size - 1u});

            set.shrink_to_fit();

            ASSERT_GE(set.capacity(), 2u);
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);

            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
            ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Pagination) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        ASSERT_EQ(set.extent(), 0u);

        set.push(entity_type{traits_type::page_size - 1u});

        ASSERT_EQ(set.extent(), traits_type::page_size);
        ASSERT_TRUE(set.contains(entity_type{traits_type::page_size - 1u}));

        set.push(entity_type{traits_type::page_size});

        ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
        ASSERT_TRUE(set.contains(entity_type{traits_type::page_size - 1u}));
        ASSERT_TRUE(set.contains(entity_type{traits_type::page_size}));
        ASSERT_FALSE(set.contains(entity_type{traits_type::page_size + 1u}));

        set.erase(entity_type{traits_type::page_size - 1u});

        ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
        ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
        ASSERT_TRUE(set.contains(entity_type{traits_type::page_size}));

        set.shrink_to_fit();
        set.erase(entity_type{traits_type::page_size});

        ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
        ASSERT_FALSE(set.contains(entity_type{traits_type::page_size - 1u}));
        ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));

        set.shrink_to_fit();

        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.extent(), 0u);
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Contiguous) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{1};
        const entity_type other{2};

        ASSERT_TRUE(set.contiguous());

        set.push(entity);
        set.push(other);

        ASSERT_TRUE(set.contiguous());

        set.erase(entity);

        switch(policy) {
        case entt::deletion_policy::swap_only:
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_TRUE(set.contiguous());

            set.clear();

            ASSERT_TRUE(set.contiguous());
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_FALSE(set.contiguous());

            set.compact();

            ASSERT_TRUE(set.contiguous());

            set.push(entity);
            set.erase(entity);

            ASSERT_FALSE(set.contiguous());

            set.clear();

            ASSERT_TRUE(set.contiguous());
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Data) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{1};
        const entity_type other{2};

        ASSERT_EQ(set.data(), nullptr);

        set.push(entity);
        set.push(other);
        set.erase(entity);

        ASSERT_FALSE(set.contains(entity));

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_FALSE(set.contains(traits_type::next(entity)));

            ASSERT_EQ(set.size(), 1u);

            ASSERT_EQ(set.index(other), 0u);

            ASSERT_EQ(set.data()[0u], other);
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_FALSE(set.contains(traits_type::next(entity)));

            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(other), 1u);

            ASSERT_EQ(set.data()[0u], static_cast<entity_type>(entt::tombstone));
            ASSERT_EQ(set.data()[1u], other);
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_TRUE(set.contains(traits_type::next(entity)));

            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(other), 0u);
            ASSERT_EQ(set.index(traits_type::next(entity)), 1u);

            ASSERT_EQ(set.data()[0u], other);
            ASSERT_EQ(set.data()[1u], traits_type::next(entity));
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Bind) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        ASSERT_NO_THROW(set.bind(0));
    }
}

TYPED_TEST(SparseSet, Iterator) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using iterator = typename sparse_set_type::iterator;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        testing::StaticAssertTypeEq<typename iterator::value_type, entity_type>();
        testing::StaticAssertTypeEq<typename iterator::pointer, const entity_type *>();
        testing::StaticAssertTypeEq<typename iterator::reference, const entity_type &>();

        set.push(entity_type{3});

        iterator end{set.begin()};
        iterator begin{};

        ASSERT_EQ(end.data(), set.data());
        ASSERT_EQ(begin.data(), nullptr);

        begin = set.end();
        std::swap(begin, end);

        ASSERT_EQ(end.data(), set.data());
        ASSERT_EQ(begin.data(), set.data());

        ASSERT_EQ(begin, set.cbegin());
        ASSERT_EQ(end, set.cend());
        ASSERT_NE(begin, end);

        ASSERT_EQ(begin.index(), 0);
        ASSERT_EQ(end.index(), -1);

        ASSERT_EQ(begin++, set.begin());
        ASSERT_EQ(begin--, set.end());

        ASSERT_EQ(begin + 1, set.end());
        ASSERT_EQ(end - 1, set.begin());

        ASSERT_EQ(++begin, set.end());
        ASSERT_EQ(--begin, set.begin());

        ASSERT_EQ(begin += 1, set.end());
        ASSERT_EQ(begin -= 1, set.begin());

        ASSERT_EQ(begin + (end - begin), set.end());
        ASSERT_EQ(begin - (begin - end), set.end());

        ASSERT_EQ(end - (end - begin), set.begin());
        ASSERT_EQ(end + (begin - end), set.begin());

        ASSERT_EQ(begin[0u], *set.begin());

        ASSERT_LT(begin, end);
        ASSERT_LE(begin, set.begin());

        ASSERT_GT(end, begin);
        ASSERT_GE(end, set.end());

        ASSERT_EQ(*begin, entity_type{3});
        ASSERT_EQ(*begin.operator->(), entity_type{3});

        ASSERT_EQ(begin.index(), 0);
        ASSERT_EQ(end.index(), -1);

        set.push(entity_type{1});
        begin = set.begin();

        ASSERT_EQ(begin.index(), 1);
        ASSERT_EQ(end.index(), -1);

        ASSERT_EQ(begin[0u], entity_type{1});
        ASSERT_EQ(begin[1u], entity_type{3});
    }
}

TYPED_TEST(SparseSet, ReverseIterator) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using reverse_iterator = typename sparse_set_type::reverse_iterator;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        testing::StaticAssertTypeEq<typename reverse_iterator::value_type, entity_type>();
        testing::StaticAssertTypeEq<typename reverse_iterator::pointer, const entity_type *>();
        testing::StaticAssertTypeEq<typename reverse_iterator::reference, const entity_type &>();

        set.push(entity_type{3});

        reverse_iterator end{set.rbegin()};
        reverse_iterator begin{};
        begin = set.rend();
        std::swap(begin, end);

        ASSERT_EQ(begin, set.crbegin());
        ASSERT_EQ(end, set.crend());
        ASSERT_NE(begin, end);

        ASSERT_EQ(begin.base().index(), -1);
        ASSERT_EQ(end.base().index(), 0);

        ASSERT_EQ(begin++, set.rbegin());
        ASSERT_EQ(begin--, set.rend());

        ASSERT_EQ(begin + 1, set.rend());
        ASSERT_EQ(end - 1, set.rbegin());

        ASSERT_EQ(++begin, set.rend());
        ASSERT_EQ(--begin, set.rbegin());

        ASSERT_EQ(begin += 1, set.rend());
        ASSERT_EQ(begin -= 1, set.rbegin());

        ASSERT_EQ(begin + (end - begin), set.rend());
        ASSERT_EQ(begin - (begin - end), set.rend());

        ASSERT_EQ(end - (end - begin), set.rbegin());
        ASSERT_EQ(end + (begin - end), set.rbegin());

        ASSERT_EQ(begin[0u], *set.rbegin());

        ASSERT_LT(begin, end);
        ASSERT_LE(begin, set.rbegin());

        ASSERT_GT(end, begin);
        ASSERT_GE(end, set.rend());

        ASSERT_EQ(*begin, entity_type{3});
        ASSERT_EQ(*begin.operator->(), entity_type{3});

        ASSERT_EQ(begin.base().index(), -1);
        ASSERT_EQ(end.base().index(), 0);

        set.push(entity_type{1});
        end = set.rend();

        ASSERT_EQ(begin.base().index(), -1);
        ASSERT_EQ(end.base().index(), 1);

        ASSERT_EQ(begin[0u], entity_type{3});
        ASSERT_EQ(begin[1u], entity_type{1});
    }
}

TYPED_TEST(SparseSet, Find) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        ASSERT_EQ(set.find(entt::tombstone), set.cend());
        ASSERT_EQ(set.find(entt::null), set.cend());

        const entity_type entity{3};
        const entity_type other{traits_type::construct(2, 1)};

        ASSERT_EQ(set.find(entity), set.cend());
        ASSERT_EQ(set.find(other), set.cend());

        set.push(entity);
        set.push(other);

        ASSERT_NE(set.find(entity), set.end());
        ASSERT_EQ(set.find(traits_type::next(entity)), set.end());
        ASSERT_EQ(*set.find(other), other);
    }
}

TYPED_TEST(SparseSet, FindErased) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{3};

        set.push(entity);
        set.erase(entity);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.find(entity), set.cend());
            ASSERT_EQ(set.find(traits_type::next(entity)), set.cend());
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.find(entity), set.cend());
            ASSERT_NE(set.find(traits_type::next(entity)), set.cend());
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Contains) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{3};
        const entity_type other{traits_type::construct(4, 1)};

        set.push(entity);
        set.push(other);

        ASSERT_FALSE(set.contains(entt::null));
        ASSERT_FALSE(set.contains(entt::tombstone));

        ASSERT_TRUE(set.contains(entity));
        ASSERT_TRUE(set.contains(other));

        ASSERT_FALSE(set.contains(entity_type{1}));
        ASSERT_FALSE(set.contains(traits_type::construct(3, 1)));
        ASSERT_FALSE(set.contains(traits_type::construct(4, traits_type::to_version(entt::tombstone))));

        set.erase(entity);
        set.remove(other);

        ASSERT_FALSE(set.contains(entity));
        ASSERT_FALSE(set.contains(other));

        if constexpr(traits_type::to_integral(entt::tombstone) != ~typename traits_type::entity_type{}) {
            // test reserved bits, if any
            constexpr entity_type reserved{traits_type::to_integral(entity) | (traits_type::to_integral(entt::tombstone) + 1u)};

            ASSERT_NE(entity, reserved);

            set.push(reserved);

            ASSERT_TRUE(set.contains(entity));
            ASSERT_TRUE(set.contains(reserved));

            ASSERT_NE(*set.find(entity), entity);
            ASSERT_EQ(*set.find(entity), reserved);

            set.bump(entity);

            ASSERT_TRUE(set.contains(entity));
            ASSERT_TRUE(set.contains(reserved));

            ASSERT_NE(*set.find(reserved), reserved);
            ASSERT_EQ(*set.find(reserved), entity);

            set.erase(reserved);

            ASSERT_FALSE(set.contains(entity));
            ASSERT_FALSE(set.contains(reserved));

            ASSERT_EQ(set.find(reserved), set.end());
        }
    }
}

TYPED_TEST(SparseSet, ContainsErased) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{3};

        set.push(entity);
        set.erase(entity);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_FALSE(set.contains(entity));
            ASSERT_FALSE(set.contains(traits_type::next(entity)));
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 1u);
            ASSERT_FALSE(set.contains(entity));
            ASSERT_FALSE(set.contains(traits_type::next(entity)));
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 1u);
            ASSERT_FALSE(set.contains(entity));
            ASSERT_TRUE(set.contains(traits_type::next(entity)));
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Current) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        ASSERT_EQ(set.current(entt::tombstone), traits_type::to_version(entt::tombstone));
        ASSERT_EQ(set.current(entt::null), traits_type::to_version(entt::tombstone));

        const entity_type entity{traits_type::construct(0, 0)};
        const entity_type other{traits_type::construct(3, 3)};

        ASSERT_EQ(set.current(entity), traits_type::to_version(entt::tombstone));
        ASSERT_EQ(set.current(other), traits_type::to_version(entt::tombstone));

        set.push(entity);
        set.push(other);

        ASSERT_NE(set.current(entity), traits_type::to_version(entt::tombstone));
        ASSERT_NE(set.current(other), traits_type::to_version(entt::tombstone));

        ASSERT_EQ(set.current(traits_type::next(entity)), traits_type::to_version(entity));
        ASSERT_EQ(set.current(traits_type::next(other)), traits_type::to_version(other));
    }
}

TYPED_TEST(SparseSet, CurrentErased) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{traits_type::construct(3, 3)};

        set.push(entity);
        set.erase(entity);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.current(entity), traits_type::to_version(entt::tombstone));
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.current(entity), traits_type::to_version(entt::tombstone));
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.current(entity), traits_type::to_version(traits_type::next(entity)));
        } break;
        }
    }
}

TYPED_TEST(SparseSet, Index) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{1};
        const entity_type other{2};

        set.push(entity);
        set.push(other);

        ASSERT_EQ(set.index(entity), 0u);
        ASSERT_EQ(set.index(other), 1u);

        set.erase(entity);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 1u);
            ASSERT_FALSE(set.contains(traits_type::next(entity)));
            ASSERT_EQ(set.index(other), 0u);
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 2u);
            ASSERT_FALSE(set.contains(traits_type::next(entity)));
            ASSERT_EQ(set.index(other), 1u);
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 2u);
            ASSERT_TRUE(set.contains(traits_type::next(entity)));
            ASSERT_EQ(set.index(traits_type::next(entity)), 1u);
            ASSERT_EQ(set.index(other), 0u);
        } break;
        }
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, Index) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        const sparse_set_type set{policy};

        // index works the same in all cases, test only once
        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
            ASSERT_DEATH([[maybe_unused]] const auto pos = set.index(entity_type{1}), "");
            break;
        case entt::deletion_policy::in_place:
        case entt::deletion_policy::swap_only:
            SUCCEED();
            break;
        }
    }
}

TYPED_TEST(SparseSet, Indexing) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{1}, entity_type{2}};

        set.push(entity.begin(), entity.end());

        ASSERT_EQ(set[0u], entity[0u]);
        ASSERT_EQ(set[1u], entity[1u]);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, Indexing) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        const sparse_set_type set{policy};

        // operator[] works the same in all cases, test only once
        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
            ASSERT_DEATH([[maybe_unused]] auto value = set[0u], "");
            break;
        case entt::deletion_policy::in_place:
        case entt::deletion_policy::swap_only:
            SUCCEED();
            break;
        }
    }
}

TYPED_TEST(SparseSet, Value) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{3};

        set.push(entity);

        ASSERT_EQ(set.value(entity), nullptr);
        ASSERT_EQ(std::as_const(set).value(entity), nullptr);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, Value) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        // value works the same in all cases, test only once
        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
            ASSERT_DEATH([[maybe_unused]] auto *value = set.value(entity_type{3}), "");
            break;
        case entt::deletion_policy::in_place:
        case entt::deletion_policy::swap_only:
            SUCCEED();
            break;
        }
    }
}

TYPED_TEST(SparseSet, Push) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{1}, entity_type{3}};

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(*set.push(entity[0u]), entity[0u]);
            ASSERT_EQ(*set.push(entity[1u]), entity[1u]);
            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(entity[0u]), 0u);
            ASSERT_EQ(set.index(entity[1u]), 1u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(*set.push(entity[0u]), entity[0u]);
            ASSERT_EQ(*set.push(entity[1u]), entity[1u]);
            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(entity[0u]), 0u);
            ASSERT_EQ(set.index(entity[1u]), 1u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 0u);

            auto it = set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(*it, entity[1u]);
            ASSERT_EQ(*(++it), entity[0u]);

            ASSERT_EQ(set.index(entity[0u]), 0u);
            ASSERT_EQ(set.index(entity[1u]), 1u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.push(entity.begin(), entity.begin()), set.end());
            ASSERT_EQ(set.size(), 0u);
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(*set.push(entity[0u]), entity[0u]);
            ASSERT_EQ(*set.push(entity[1u]), entity[1u]);
            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(entity[0u]), 0u);
            ASSERT_EQ(set.index(entity[1u]), 1u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(*set.push(entity[0u]), entity[0u]);
            ASSERT_EQ(*set.push(entity[1u]), entity[1u]);
            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(entity[0u]), 1u);
            ASSERT_EQ(set.index(entity[1u]), 0u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 2u);

            auto it = set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 4u);
            ASSERT_EQ(*it, entity[1u]);
            ASSERT_EQ(*(++it), entity[0u]);

            ASSERT_EQ(set.index(entity[0u]), 2u);
            ASSERT_EQ(set.index(entity[1u]), 3u);

            set.erase(entity.begin(), entity.end());
            set.compact();

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.push(entity.begin(), entity.begin()), set.end());
            ASSERT_EQ(set.size(), 0u);
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), 0u);
            ASSERT_EQ(*set.push(entity[0u]), entity[0u]);
            ASSERT_EQ(*set.push(entity[1u]), entity[1u]);
            ASSERT_EQ(set.free_list(), 2u);
            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(entity[0u]), 0u);
            ASSERT_EQ(set.index(entity[1u]), 1u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.free_list(), 0u);
            ASSERT_EQ(*set.push(entity[0u]), entity[0u]);
            ASSERT_EQ(*set.push(entity[1u]), entity[1u]);
            ASSERT_EQ(set.free_list(), 2u);
            ASSERT_EQ(set.size(), 2u);

            ASSERT_EQ(set.index(entity[0u]), 0u);
            ASSERT_EQ(set.index(entity[1u]), 1u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.free_list(), 0u);

            auto it = set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.free_list(), 2u);
            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(*it, entity[1u]);
            ASSERT_EQ(*(++it), entity[0u]);

            ASSERT_EQ(set.index(entity[0u]), 0u);
            ASSERT_EQ(set.index(entity[1u]), 1u);

            set.erase(entity.begin(), entity.end());

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.free_list(), 0u);
            ASSERT_EQ(set.push(entity.begin(), entity.begin()), set.end());
            ASSERT_EQ(set.free_list(), 0u);
            ASSERT_EQ(set.size(), 2u);
        } break;
        }
    }
}

TYPED_TEST(SparseSet, PushOutOfBounds) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{0}, entity_type{traits_type::page_size}};

        ASSERT_EQ(*set.push(entity[0u]), entity[0u]);
        ASSERT_EQ(set.extent(), traits_type::page_size);
        ASSERT_EQ(set.index(entity[0u]), 0u);

        set.erase(entity[0u]);

        ASSERT_EQ(*set.push(entity[1u]), entity[1u]);
        ASSERT_EQ(set.extent(), 2u * traits_type::page_size);
        ASSERT_EQ(set.index(entity[1u]), 0u);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, Push) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{1}, entity_type{3}};

        set.push(entity.begin(), entity.end());

        ASSERT_DEATH(set.push(entity[0u]), "");
        ASSERT_DEATH(set.push(entity.begin(), entity.end()), "");
    }
}

TYPED_TEST(SparseSet, Bump) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{1}, entity_type{3}, traits_type::construct(2, 4)};

        set.push(entity.begin(), entity.end());

        ASSERT_EQ(set.current(entity[0u]), 0u);
        ASSERT_EQ(set.current(entity[1u]), 0u);
        ASSERT_EQ(set.current(entity[2u]), 4u);

        ASSERT_EQ(set.bump(entity[0u]), 0u);
        ASSERT_EQ(set.bump(traits_type::construct(traits_type::to_entity(entity[1u]), 1)), 1u);
        ASSERT_EQ(set.bump(traits_type::construct(traits_type::to_entity(entity[2u]), 0)), 0u);

        ASSERT_EQ(set.current(entity[0u]), 0u);
        ASSERT_EQ(set.current(entity[1u]), 1u);
        ASSERT_EQ(set.current(entity[2u]), 0u);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, Bump) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        // bump works the same in all cases, test only once
        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
            ASSERT_DEATH(set.bump(entt::null), "");
            ASSERT_DEATH(set.bump(entt::tombstone), "");
            ASSERT_DEATH(set.bump(entity_type{3}), "");
            break;
        case entt::deletion_policy::in_place:
        case entt::deletion_policy::swap_only:
            SUCCEED();
            break;
        }
    }
}

TYPED_TEST(SparseSet, Erase) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{1}, entity_type{3}, traits_type::construct(2, 4)};

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            set.push(entity.begin(), entity.end());
            set.erase(set.begin(), set.end());

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            set.push(entity.begin(), entity.end());
            set.erase(entity.begin(), entity.begin() + 2u);

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);
            ASSERT_TRUE(set.contains(entity[2u]));

            set.erase(entity[2u]);

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);
            ASSERT_FALSE(set.contains(entity[2u]));
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            set.push(entity.begin(), entity.end());
            set.erase(set.begin(), set.end());

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 0u);

            ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
            ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
            ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

            set.push(entity[0u]);
            set.push(entity.begin() + 1u, entity.end());
            set.erase(entity.begin(), entity.begin() + 2u);

            ASSERT_EQ(set.size(), 5u);
            ASSERT_EQ(set.free_list(), 3u);

            ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
            ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
            ASSERT_TRUE(set.contains(entity[2u]));

            set.erase(entity[2u]);

            ASSERT_EQ(set.size(), 5u);
            ASSERT_EQ(set.free_list(), 4u);
            ASSERT_FALSE(set.contains(entity[2u]));
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), 0u);

            set.push(entity.begin(), entity.end());
            set.erase(set.begin(), set.end());

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 0u);

            ASSERT_TRUE(set.contains(traits_type::next(entity[0u])));
            ASSERT_TRUE(set.contains(traits_type::next(entity[1u])));
            ASSERT_TRUE(set.contains(traits_type::next(entity[2u])));

            set.push(entity.begin(), entity.end());
            set.erase(entity.begin(), entity.begin() + 2u);

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 1u);

            ASSERT_TRUE(set.contains(traits_type::next(entity[0u])));
            ASSERT_TRUE(set.contains(traits_type::next(entity[1u])));
            ASSERT_TRUE(set.contains(entity[2u]));

            ASSERT_LT(set.index(entity[2u]), set.free_list());

            set.erase(entity[2u]);

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 0u);
            ASSERT_TRUE(set.contains(traits_type::next(entity[2u])));
        } break;
        }
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, Erase) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{3}, traits_type::construct(2, 4)};

        ASSERT_DEATH(set.erase(entity.begin(), entity.end()), "");
        ASSERT_DEATH(set.erase(entity.begin(), entity.begin() + 2u), "");
    }
}

TYPED_TEST(SparseSet, CrossErase) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};
        sparse_set_type other{policy};

        const std::array entity{entity_type{1}, entity_type{3}};

        set.push(entity.begin(), entity.end());
        other.push(entity[1u]);
        set.erase(other.begin(), other.end());

        ASSERT_TRUE(set.contains(entity[0u]));
        ASSERT_FALSE(set.contains(entity[1u]));
        ASSERT_EQ(set.data()[0u], entity[0u]);
    }
}

TYPED_TEST(SparseSet, Remove) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{1}, entity_type{3}, traits_type::construct(2, 4)};

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            ASSERT_EQ(set.remove(entity.begin(), entity.end()), 0u);
            ASSERT_FALSE(set.remove(entity[1u]));

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.remove(set.begin(), set.end()), 3u);

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.remove(entity.begin(), entity.begin() + 2u), 2u);

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);
            ASSERT_TRUE(set.contains(entity[2u]));

            ASSERT_TRUE(set.remove(entity[2u]));
            ASSERT_FALSE(set.remove(entity[2u]));

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);
            ASSERT_FALSE(set.contains(entity[2u]));
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            ASSERT_EQ(set.remove(entity.begin(), entity.end()), 0u);
            ASSERT_FALSE(set.remove(entity[1u]));

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), traits_type::entity_mask);

            set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.remove(set.begin(), set.end()), 3u);

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 0u);

            ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
            ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
            ASSERT_EQ(set.current(entity[2u]), traits_type::to_version(entt::tombstone));

            set.push(entity[0u]);
            set.push(entity.begin() + 1u, entity.end());

            ASSERT_EQ(set.remove(entity.begin(), entity.begin() + 2u), 2u);

            ASSERT_EQ(set.size(), 5u);
            ASSERT_EQ(set.free_list(), 3u);

            ASSERT_EQ(set.current(entity[0u]), traits_type::to_version(entt::tombstone));
            ASSERT_EQ(set.current(entity[1u]), traits_type::to_version(entt::tombstone));
            ASSERT_TRUE(set.contains(entity[2u]));

            ASSERT_TRUE(set.remove(entity[2u]));
            ASSERT_FALSE(set.remove(entity[2u]));

            ASSERT_EQ(set.size(), 5u);
            ASSERT_EQ(set.free_list(), 4u);
            ASSERT_FALSE(set.contains(entity[2u]));
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), 0u);

            ASSERT_EQ(set.remove(entity.begin(), entity.end()), 0u);
            ASSERT_FALSE(set.remove(entity[1u]));

            ASSERT_EQ(set.size(), 0u);
            ASSERT_EQ(set.free_list(), 0u);

            set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.remove(set.begin(), set.end()), 3u);

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 0u);

            ASSERT_TRUE(set.contains(traits_type::next(entity[0u])));
            ASSERT_TRUE(set.contains(traits_type::next(entity[1u])));
            ASSERT_TRUE(set.contains(traits_type::next(entity[2u])));

            set.push(entity.begin(), entity.end());

            ASSERT_EQ(set.remove(entity.begin(), entity.begin() + 2u), 2u);

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 1u);

            ASSERT_TRUE(set.contains(traits_type::next(entity[0u])));
            ASSERT_TRUE(set.contains(traits_type::next(entity[1u])));
            ASSERT_TRUE(set.contains(entity[2u]));

            ASSERT_LT(set.index(entity[2u]), set.free_list());

            ASSERT_TRUE(set.remove(entity[2u]));
            ASSERT_FALSE(set.remove(entity[2u]));

            ASSERT_EQ(set.size(), 3u);
            ASSERT_EQ(set.free_list(), 0u);
            ASSERT_TRUE(set.contains(traits_type::next(entity[2u])));

            ASSERT_TRUE(set.remove(traits_type::next(entity[2u])));

            ASSERT_TRUE(set.contains(traits_type::next(traits_type::next(entity[2u]))));
        } break;
        }
    }
}

TYPED_TEST(SparseSet, CrossRemove) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};
        sparse_set_type other{policy};

        const std::array entity{entity_type{1}, entity_type{3}};

        set.push(entity.begin(), entity.end());
        other.push(entity[1u]);
        set.remove(other.begin(), other.end());

        ASSERT_TRUE(set.contains(entity[0u]));
        ASSERT_FALSE(set.contains(entity[1u]));
        ASSERT_EQ(set.data()[0u], entity[0u]);
    }
}

TYPED_TEST(SparseSet, Compact) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{1};
        const entity_type other{2};

        set.push(entity);
        set.push(other);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(entity), 0u);
            ASSERT_EQ(set.index(other), 1u);

            set.compact();

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(entity), 0u);
            ASSERT_EQ(set.index(other), 1u);

            set.erase(entity);

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.index(other), 0u);

            set.compact();

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.index(other), 0u);
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(entity), 0u);
            ASSERT_EQ(set.index(other), 1u);

            set.compact();

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(entity), 0u);
            ASSERT_EQ(set.index(other), 1u);

            set.erase(other);

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(entity), 0u);

            set.compact();

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.index(entity), 0u);

            set.push(other);
            set.erase(entity);

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(other), 1u);

            set.compact();

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.index(other), 0u);

            set.compact();

            ASSERT_EQ(set.size(), 1u);
            ASSERT_EQ(set.index(other), 0u);
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(entity), 0u);
            ASSERT_EQ(set.index(other), 1u);

            set.compact();

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(entity), 0u);
            ASSERT_EQ(set.index(other), 1u);

            set.erase(entity);

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(other), 0u);
            ASSERT_EQ(set.index(traits_type::next(entity)), 1u);

            set.compact();

            ASSERT_EQ(set.size(), 2u);
            ASSERT_EQ(set.index(other), 0u);
            ASSERT_EQ(set.index(traits_type::next(entity)), 1u);
        } break;
        }
    }
}

TYPED_TEST(SparseSet, SwapElements) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const auto entity = traits_type::construct(1, 2);
        const auto other = traits_type::construct(3, 4);

        set.push(entity);
        set.push(other);

        ASSERT_EQ(set.index(entity), 0u);
        ASSERT_EQ(set.index(other), 1u);

        set.swap_elements(entity, other);

        ASSERT_EQ(set.index(entity), 1u);
        ASSERT_EQ(set.index(other), 0u);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, SwapElements) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const auto entity = traits_type::construct(1, 2);
        const auto other = traits_type::construct(3, 4);

        // swap_elements works the same in all cases, test only once
        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
        case entt::deletion_policy::in_place:
            SUCCEED();
            break;
        case entt::deletion_policy::swap_only:
            ASSERT_DEATH(set.swap_elements(entity, other), "");

            set.push(entity);
            set.push(other);
            set.erase(entity);

            ASSERT_DEATH(set.swap_elements(entity, other), "");
        }
    }
}

TYPED_TEST(SparseSet, Clear) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const std::array entity{entity_type{1}, entity_type{3}, entity_type{2}};

        set.push(entity.begin(), entity.end());
        set.erase(entity[1u]);
        set.clear();

        ASSERT_EQ(set.size(), 0u);
    }
}

TYPED_TEST(SparseSet, SortOrdered) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        std::array entity{entity_type{16}, entity_type{8}, entity_type{4}, entity_type{2}, entity_type{1}};

        set.push(entity.begin(), entity.end());
        set.sort(std::less{});

        ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), set.begin(), set.end()));
    }
}

TYPED_TEST(SparseSet, SortReverse) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        std::array entity{entity_type{1}, entity_type{2}, entity_type{4}, entity_type{8}, entity_type{16}};

        set.push(entity.begin(), entity.end());
        set.sort(std::less{});

        ASSERT_TRUE(std::equal(entity.begin(), entity.end(), set.begin(), set.end()));
    }
}

TYPED_TEST(SparseSet, SortUnordered) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        std::array entity{entity_type{4}, entity_type{2}, entity_type{1}, entity_type{8}, entity_type{16}};

        set.push(entity.begin(), entity.end());
        set.sort(std::less{});

        auto begin = set.begin();
        const auto end = set.end();

        ASSERT_EQ(*(begin++), entity[2u]);
        ASSERT_EQ(*(begin++), entity[1u]);
        ASSERT_EQ(*(begin++), entity[0u]);
        ASSERT_EQ(*(begin++), entity[3u]);
        ASSERT_EQ(*(begin++), entity[4u]);

        ASSERT_EQ(begin, end);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, Sort) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{1};
        const entity_type other{2};

        set.push(entity);
        set.push(other);
        set.erase(entity);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop:
        case entt::deletion_policy::swap_only: {
            SUCCEED();
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_DEATH(set.sort(std::less{}), "");
        } break;
        }
    }
}

TYPED_TEST(SparseSet, SortN) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        std::array entity{entity_type{2}, entity_type{4}, entity_type{1}, entity_type{8}, entity_type{16}};

        set.push(entity.begin(), entity.end());
        set.sort_n(0u, std::less{});

        ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), set.begin(), set.end()));

        set.sort_n(2u, std::less{});

        ASSERT_EQ(set.data()[0u], entity[1u]);
        ASSERT_EQ(set.data()[1u], entity[0u]);

        const auto length = 5u;
        set.sort_n(length, std::less{});

        auto begin = set.begin();
        auto end = set.end();

        ASSERT_EQ(*(begin++), entity[2u]);
        ASSERT_EQ(*(begin++), entity[0u]);
        ASSERT_EQ(*(begin++), entity[1u]);
        ASSERT_EQ(*(begin++), entity[3u]);
        ASSERT_EQ(*(begin++), entity[4u]);

        ASSERT_EQ(begin, end);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, SortN) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};

        const entity_type entity{1};
        const entity_type other{2};

        ASSERT_DEATH(set.sort_n(1u, std::less{}), "");

        set.push(entity);
        set.push(other);
        set.erase(entity);

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            SUCCEED();
        } break;
        case entt::deletion_policy::in_place: {
            ASSERT_EQ(set.size(), 2u);
            ASSERT_DEATH(set.sort_n(1u, std::less{}), "");
        } break;
        case entt::deletion_policy::swap_only: {
            ASSERT_EQ(set.size(), 2u);
            ASSERT_NO_THROW(set.sort_n(1u, std::less{}));
            ASSERT_DEATH(set.sort_n(2u, std::less{}), "");
        } break;
        }
    }
}

TYPED_TEST(SparseSet, SortAsDisjoint) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type lhs{policy};
        const sparse_set_type rhs{policy};

        std::array entity{entity_type{1}, entity_type{2}, entity_type{4}};

        lhs.push(entity.begin(), entity.end());

        ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.begin(), lhs.end()));

        const auto it = lhs.sort_as(rhs.begin(), rhs.end());

        ASSERT_EQ(it, lhs.begin());
        ASSERT_TRUE(std::equal(entity.rbegin(), entity.rend(), lhs.begin(), lhs.end()));
    }
}

TYPED_TEST(SparseSet, SortAsOverlap) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type lhs{policy};
        sparse_set_type rhs{policy};

        std::array lhs_entity{entity_type{1}, entity_type{2}, entity_type{4}};
        std::array rhs_entity{entity_type{2}};

        lhs.push(lhs_entity.begin(), lhs_entity.end());
        rhs.push(rhs_entity.begin(), rhs_entity.end());

        ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
        ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

        const auto it = lhs.sort_as(rhs.begin(), rhs.end());

        ASSERT_EQ(it, lhs.begin() + rhs_entity.size());

        auto begin = lhs.begin();
        const auto end = lhs.end();

        ASSERT_EQ(*(begin++), lhs_entity[1u]);
        ASSERT_EQ(*(begin++), lhs_entity[2u]);
        ASSERT_EQ(*(begin++), lhs_entity[0u]);
        ASSERT_EQ(begin, end);
    }
}

TYPED_TEST(SparseSet, SortAsOrdered) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type lhs{policy};
        sparse_set_type rhs{policy};

        std::array lhs_entity{entity_type{1}, entity_type{2}, entity_type{4}, entity_type{8}, entity_type{16}};
        std::array rhs_entity{entity_type{32}, entity_type{1}, entity_type{2}, entity_type{4}, entity_type{8}, entity_type{16}};

        lhs.push(lhs_entity.begin(), lhs_entity.end());
        rhs.push(rhs_entity.begin(), rhs_entity.end());

        ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
        ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

        const auto it = rhs.sort_as(lhs.begin(), lhs.end());

        ASSERT_EQ(it, rhs.begin() + lhs_entity.size());
        ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));
    }
}

TYPED_TEST(SparseSet, SortAsReverse) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type lhs{policy};
        sparse_set_type rhs{policy};

        std::array lhs_entity{entity_type{1}, entity_type{2}, entity_type{4}, entity_type{8}, entity_type{16}};
        std::array rhs_entity{entity_type{16}, entity_type{8}, entity_type{4}, entity_type{2}, entity_type{1}, entity_type{32}};

        lhs.push(lhs_entity.begin(), lhs_entity.end());
        rhs.push(rhs_entity.begin(), rhs_entity.end());

        ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
        ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

        const auto it = rhs.sort_as(lhs.begin(), lhs.end());

        ASSERT_EQ(it, rhs.begin() + lhs_entity.size());

        auto begin = rhs.begin();
        const auto end = rhs.end();

        ASSERT_EQ(*(begin++), rhs_entity[0u]);
        ASSERT_EQ(*(begin++), rhs_entity[1u]);
        ASSERT_EQ(*(begin++), rhs_entity[2u]);
        ASSERT_EQ(*(begin++), rhs_entity[3u]);
        ASSERT_EQ(*(begin++), rhs_entity[4u]);
        ASSERT_EQ(*(begin++), rhs_entity[5u]);
        ASSERT_EQ(begin, end);
    }
}

TYPED_TEST(SparseSet, SortAsUnordered) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type lhs{policy};
        sparse_set_type rhs{policy};

        std::array lhs_entity{entity_type{1}, entity_type{2}, entity_type{4}, entity_type{8}, entity_type{16}};
        std::array rhs_entity{entity_type{4}, entity_type{2}, entity_type{32}, entity_type{1}, entity_type{8}, entity_type{16}};

        lhs.push(lhs_entity.begin(), lhs_entity.end());
        rhs.push(rhs_entity.begin(), rhs_entity.end());

        ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
        ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

        const auto it = rhs.sort_as(lhs.begin(), lhs.end());

        ASSERT_EQ(it, rhs.begin() + lhs_entity.size());

        auto begin = rhs.begin();
        const auto end = rhs.end();

        ASSERT_EQ(*(begin++), rhs_entity[5u]);
        ASSERT_EQ(*(begin++), rhs_entity[4u]);
        ASSERT_EQ(*(begin++), rhs_entity[0u]);
        ASSERT_EQ(*(begin++), rhs_entity[1u]);
        ASSERT_EQ(*(begin++), rhs_entity[3u]);
        ASSERT_EQ(*(begin++), rhs_entity[2u]);
        ASSERT_EQ(begin, end);
    }
}

TYPED_TEST(SparseSet, SortAsInvalid) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type lhs{policy};
        sparse_set_type rhs{policy};

        std::array lhs_entity{entity_type{1}, entity_type{2}, traits_type::construct(3, 1)};
        std::array rhs_entity{entity_type{2}, entity_type{1}, traits_type::construct(3, 2)};

        lhs.push(lhs_entity.begin(), lhs_entity.end());
        rhs.push(rhs_entity.begin(), rhs_entity.end());

        ASSERT_TRUE(std::equal(lhs_entity.rbegin(), lhs_entity.rend(), lhs.begin(), lhs.end()));
        ASSERT_TRUE(std::equal(rhs_entity.rbegin(), rhs_entity.rend(), rhs.begin(), rhs.end()));

        const auto it = rhs.sort_as(lhs.begin(), lhs.end());

        ASSERT_EQ(it, rhs.begin() + lhs_entity.size() - 1u);

        auto begin = rhs.begin();
        const auto end = rhs.end();

        ASSERT_EQ(*(begin++), rhs_entity[0u]);
        ASSERT_EQ(*(begin++), rhs_entity[1u]);
        ASSERT_EQ(*(begin++), rhs_entity[2u]);
        ASSERT_EQ(rhs.current(rhs_entity[0u]), 0u);
        ASSERT_EQ(rhs.current(rhs_entity[1u]), 0u);
        ASSERT_EQ(rhs.current(rhs_entity[2u]), 2u);
        ASSERT_EQ(begin, end);
    }
}

ENTT_DEBUG_TYPED_TEST(SparseSetDeathTest, SortAs) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type lhs{policy};
        sparse_set_type rhs{policy};

        switch(policy) {
        case entt::deletion_policy::swap_and_pop: {
            SUCCEED();
        } break;
        case entt::deletion_policy::in_place: {
            const entity_type entity{4};

            lhs.push(entity);
            lhs.erase(entity);

            ASSERT_DEATH(lhs.sort_as(rhs.begin(), rhs.end()), "");
        } break;
        case entt::deletion_policy::swap_only: {
            const std::array entity{entity_type{1}, entity_type{4}, entity_type{2}};

            lhs.push(entity.begin(), entity.end());
            rhs.push(entity.rbegin(), entity.rend());
            lhs.erase(entity[0u]);
            lhs.bump(entity[0u]);

            ASSERT_DEATH(lhs.sort_as(rhs.begin(), rhs.end()), "");
        } break;
        }
    }
}

TYPED_TEST(SparseSet, CanModifyDuringIteration) {
    using entity_type = typename TestFixture::type;
    using sparse_set_type = entt::basic_sparse_set<entity_type>;

    for(const auto policy: this->deletion_policy) {
        sparse_set_type set{policy};
        set.push(entity_type{0});

        ASSERT_EQ(set.capacity(), 1u);

        const auto it = set.begin();
        set.reserve(2u);

        ASSERT_EQ(set.capacity(), 2u);

        // this should crash with asan enabled if we break the constraint
        [[maybe_unused]] const auto entity = *it;
    }
}

TYPED_TEST(SparseSet, CustomAllocator) {
    using entity_type = typename TestFixture::type;

    for(const auto policy: this->deletion_policy) {
        const test::throwing_allocator<entity_type> allocator{};
        entt::basic_sparse_set<entity_type, test::throwing_allocator<entity_type>> set{policy, allocator};

        ASSERT_EQ(set.get_allocator(), allocator);

        set.reserve(1u);

        ASSERT_EQ(set.capacity(), 1u);

        set.push(entity_type{0});
        set.push(entity_type{1});

        entt::basic_sparse_set<entity_type, test::throwing_allocator<entity_type>> other{std::move(set), allocator};

        test::is_initialized(set);

        ASSERT_TRUE(set.empty());
        ASSERT_FALSE(other.empty());
        ASSERT_EQ(other.capacity(), 2u);
        ASSERT_EQ(other.size(), 2u);

        set = std::move(other);
        test::is_initialized(other);

        ASSERT_FALSE(set.empty());
        ASSERT_TRUE(other.empty());
        ASSERT_EQ(set.capacity(), 2u);
        ASSERT_EQ(set.size(), 2u);

        other = {};
        set.swap(other);
        set = std::move(other);
        test::is_initialized(other);

        ASSERT_FALSE(set.empty());
        ASSERT_TRUE(other.empty());
        ASSERT_EQ(set.capacity(), 2u);
        ASSERT_EQ(set.size(), 2u);

        set.clear();

        ASSERT_EQ(set.capacity(), 2u);
        ASSERT_EQ(set.size(), 0u);

        set.shrink_to_fit();

        ASSERT_EQ(set.capacity(), 0u);
    }
}

TYPED_TEST(SparseSet, ThrowingAllocator) {
    using entity_type = typename TestFixture::type;
    using traits_type = entt::entt_traits<entity_type>;

    for(const auto policy: this->deletion_policy) {
        entt::basic_sparse_set<entity_type, test::throwing_allocator<entity_type>> set{policy};

        set.get_allocator().template throw_counter<entity_type>(0u);

        ASSERT_THROW(set.reserve(1u), test::throwing_allocator_exception);
        ASSERT_EQ(set.capacity(), 0u);
        ASSERT_EQ(set.extent(), 0u);

        set.get_allocator().template throw_counter<entity_type>(0u);

        ASSERT_THROW(set.push(entity_type{0}), test::throwing_allocator_exception);
        ASSERT_EQ(set.extent(), traits_type::page_size);
        ASSERT_EQ(set.capacity(), 0u);

        set.push(entity_type{0});
        set.get_allocator().template throw_counter<entity_type>(0u);

        ASSERT_THROW(set.reserve(2u), test::throwing_allocator_exception);
        ASSERT_EQ(set.extent(), traits_type::page_size);
        ASSERT_TRUE(set.contains(entity_type{0}));
        ASSERT_EQ(set.capacity(), 1u);

        set.get_allocator().template throw_counter<entity_type>(0u);

        ASSERT_THROW(set.push(entity_type{1}), test::throwing_allocator_exception);
        ASSERT_EQ(set.extent(), traits_type::page_size);
        ASSERT_TRUE(set.contains(entity_type{0}));
        ASSERT_FALSE(set.contains(entity_type{1}));
        ASSERT_EQ(set.capacity(), 1u);

        const std::array entity{entity_type{1}, entity_type{traits_type::page_size}};
        set.get_allocator().template throw_counter<entity_type>(1u);

        ASSERT_THROW(set.push(entity.begin(), entity.end()), test::throwing_allocator_exception);
        ASSERT_EQ(set.extent(), 2 * traits_type::page_size);
        ASSERT_TRUE(set.contains(entity_type{0}));
        ASSERT_TRUE(set.contains(entity_type{1}));
        ASSERT_FALSE(set.contains(entity_type{traits_type::page_size}));
        ASSERT_EQ(set.capacity(), 2u);
        ASSERT_EQ(set.size(), 2u);

        set.push(entity[1u]);

        ASSERT_TRUE(set.contains(entity_type{traits_type::page_size}));
    }
}
