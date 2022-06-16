#include <gtest/gtest.h>
#include <entt/core/fwd.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/graph/flow.hpp>
#include "../common/throwing_allocator.hpp"

TEST(Flow, Constructors) {
    entt::flow flow{};

    ASSERT_EQ(flow.size(), 0u);

    flow = entt::flow{std::allocator<entt::id_type>{}};

    ASSERT_EQ(flow.size(), 0u);

    flow.bind(0);
    flow.bind(3);
    flow.bind(99);

    ASSERT_EQ(flow.size(), 3u);

    entt::flow temp{flow, flow.get_allocator()};
    entt::flow other{std::move(flow), flow.get_allocator()};

    ASSERT_EQ(flow.size(), 0u);
    ASSERT_EQ(other.size(), 3u);

    ASSERT_EQ(flow.tasks().cbegin(), flow.tasks().cend());
    ASSERT_EQ(*++other.tasks().cbegin(), 3);
}

TEST(Flow, Copy) {
    entt::flow flow{};

    flow.bind(0);
    flow.bind(3);
    flow.bind(99);

    entt::flow other{flow};

    ASSERT_EQ(flow.size(), 3u);
    ASSERT_EQ(other.size(), 3u);

    ASSERT_EQ(*flow.tasks().cbegin(), 0);
    ASSERT_EQ(*++other.tasks().cbegin(), 3);

    flow.bind(1);
    other.bind(2);

    other = flow;

    ASSERT_EQ(other.size(), 4u);
    ASSERT_EQ(flow.size(), 4u);

    ASSERT_EQ(*--flow.tasks().cend(), 1);
    ASSERT_EQ(*--other.tasks().cend(), 1);
}

TEST(Flow, Move) {
    entt::flow flow{};

    flow.bind(0);
    flow.bind(3);
    flow.bind(99);

    entt::flow other{std::move(flow)};

    ASSERT_EQ(flow.size(), 0u);
    ASSERT_EQ(other.size(), 3u);

    ASSERT_EQ(flow.tasks().cbegin(), flow.tasks().cend());
    ASSERT_EQ(*++other.tasks().cbegin(), 3);

    flow = {};
    flow.bind(1);
    other.bind(2);

    other = std::move(flow);

    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(flow.size(), 0u);

    ASSERT_EQ(flow.tasks().cbegin(), flow.tasks().cend());
    ASSERT_EQ(*other.tasks().cbegin(), 1);
}

TEST(Flow, Swap) {
    entt::flow flow{};
    entt::flow other{};

    flow.bind(7);

    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(flow.size(), 1u);
    ASSERT_EQ(*flow.tasks().cbegin(), 7);
    ASSERT_EQ(other.tasks().cbegin(), other.tasks().cend());

    flow.swap(other);

    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(flow.size(), 0u);
    ASSERT_EQ(flow.tasks().cbegin(), flow.tasks().cend());
    ASSERT_EQ(*other.tasks().cbegin(), 7);
}

TEST(Flow, Clear) {
    entt::flow flow{};

    flow.bind(0);
    flow.bind(99);

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(*flow.tasks().cbegin(), 0);
    ASSERT_EQ(*--flow.tasks().cend(), 99);
    ASSERT_EQ(++flow.tasks().cbegin(), --flow.tasks().cend());

    flow.clear();

    ASSERT_EQ(flow.size(), 0u);
    ASSERT_EQ(flow.tasks().cbegin(), flow.tasks().cend());
}

TEST(Flow, RO) {
    entt::flow flow{};
    flow.bind(0).ro(10).bind(1).ro(10).ro(11);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_EQ(graph.edges().cbegin(), graph.edges().cend());
}

TEST(Flow, RangeRO) {
    entt::flow flow{};
    const entt::id_type res[2u]{10, 11};
    flow.bind(0).ro(res, res + 1).bind(1).ro(res, res + 2);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_EQ(graph.edges().cbegin(), graph.edges().cend());
}

TEST(Flow, RW) {
    entt::flow flow{};
    flow.bind(0).rw(10).bind(1).rw(10).rw(11);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_NE(graph.edges().cbegin(), graph.edges().cend());

    ASSERT_TRUE(graph.contains(0u, 1u));
    ASSERT_FALSE(graph.contains(1u, 0u));
}

TEST(Flow, RangeRW) {
    entt::flow flow{};
    const entt::id_type res[2u]{10, 11};
    flow.bind(0).rw(res, res + 1).bind(1).rw(res, res + 2);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_NE(graph.edges().cbegin(), graph.edges().cend());

    ASSERT_TRUE(graph.contains(0u, 1u));
    ASSERT_FALSE(graph.contains(1u, 0u));
}

TEST(Flow, Graph) {
    using namespace entt::literals;

    entt::flow flow{};

    flow.bind("task_0"_hs)
        .ro("resource_0"_hs)
        .rw("resource_1"_hs);

    flow.bind("task_1"_hs)
        .ro("resource_0"_hs)
        .rw("resource_2"_hs);

    flow.bind("task_2"_hs)
        .ro("resource_1"_hs)
        .rw("resource_3"_hs);

    flow.bind("task_3"_hs)
        .rw("resource_1"_hs)
        .ro("resource_2"_hs);

    flow.bind("task_4"_hs)
        .rw("resource_0"_hs);

    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 5u);
    ASSERT_EQ(flow.size(), graph.size());

    auto from = flow.tasks().cbegin();
    const auto to = flow.tasks().cend();

    ASSERT_NE(from, to);
    ASSERT_EQ(from[0u], "task_0"_hs);
    ASSERT_EQ(from[1u], "task_1"_hs);
    ASSERT_EQ(from[2u], "task_2"_hs);
    ASSERT_EQ(from[3u], "task_3"_hs);
    ASSERT_EQ(from[4u], "task_4"_hs);
    ASSERT_EQ(from + 5u, to);

    auto first = graph.edges().cbegin();
    const auto last = graph.edges().cend();

    ASSERT_NE(first, last);
    ASSERT_EQ(*first++, std::make_pair(std::size_t{0u}, std::size_t{2u}));
    ASSERT_EQ(*first++, std::make_pair(std::size_t{0u}, std::size_t{4u}));
    ASSERT_EQ(*first++, std::make_pair(std::size_t{1u}, std::size_t{3u}));
    ASSERT_EQ(*first++, std::make_pair(std::size_t{1u}, std::size_t{4u}));
    ASSERT_EQ(*first++, std::make_pair(std::size_t{2u}, std::size_t{3u}));
    ASSERT_EQ(first, last);
}

TEST(Flow, ThrowingAllocator) {
    using allocator = test::throwing_allocator<entt::id_type>;
    using task_allocator = test::throwing_allocator<std::pair<std::size_t, entt::id_type>>;
    using task_exception = typename task_allocator::exception_type;

    entt::basic_flow<allocator> flow{};

    task_allocator::trigger_on_allocate = true;

    ASSERT_EQ(flow.size(), 0u);
    ASSERT_THROW(flow.bind(1), task_exception);
    ASSERT_EQ(flow.size(), 0u);

    flow.bind(1);

    ASSERT_EQ(flow.size(), 1u);
}
