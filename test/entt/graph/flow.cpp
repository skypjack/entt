#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/graph/flow.hpp>
#include "../../common/config.h"
#include "../../common/linter.hpp"
#include "../../common/throwing_allocator.hpp"

TEST(Flow, Constructors) {
    entt::flow flow{};

    ASSERT_TRUE(flow.empty());
    ASSERT_EQ(flow.size(), 0u);

    flow = entt::flow{std::allocator<entt::id_type>{}};

    ASSERT_TRUE(flow.empty());
    ASSERT_EQ(flow.size(), 0u);

    flow.bind(2);
    flow.bind(4);
    flow.bind(8);

    ASSERT_FALSE(flow.empty());
    ASSERT_EQ(flow.size(), 3u);

    const entt::flow temp{flow, flow.get_allocator()};
    const entt::flow other{std::move(flow), flow.get_allocator()};

    test::is_initialized(flow);

    ASSERT_TRUE(flow.empty());
    ASSERT_EQ(other.size(), 3u);

    ASSERT_EQ(other[0u], 2);
    ASSERT_EQ(other[1u], 4);
    ASSERT_EQ(other[2u], 8);
}

TEST(Flow, Copy) {
    entt::flow flow{};

    flow.bind(2);
    flow.bind(4);
    flow.bind(8);

    entt::flow other{flow};

    ASSERT_EQ(flow.size(), 3u);
    ASSERT_EQ(other.size(), 3u);

    ASSERT_EQ(other[0u], 2);
    ASSERT_EQ(other[1u], 4);
    ASSERT_EQ(other[2u], 8);

    flow.bind(1);
    other.bind(3);

    other = flow;

    ASSERT_EQ(other.size(), 4u);
    ASSERT_EQ(flow.size(), 4u);

    ASSERT_EQ(other[0u], 2);
    ASSERT_EQ(other[1u], 4);
    ASSERT_EQ(other[2u], 8);
    ASSERT_EQ(other[3u], 1);
}

TEST(Flow, Move) {
    entt::flow flow{};

    flow.bind(2);
    flow.bind(4);
    flow.bind(8);

    entt::flow other{std::move(flow)};

    test::is_initialized(flow);

    ASSERT_TRUE(flow.empty());
    ASSERT_EQ(other.size(), 3u);

    ASSERT_EQ(other[0u], 2);
    ASSERT_EQ(other[1u], 4);
    ASSERT_EQ(other[2u], 8);

    flow = {};
    flow.bind(1);
    other.bind(3);

    other = std::move(flow);
    test::is_initialized(flow);

    ASSERT_TRUE(flow.empty());
    ASSERT_EQ(other.size(), 1u);

    ASSERT_EQ(other[0u], 1);
}

TEST(Flow, Swap) {
    entt::flow flow{};
    entt::flow other{};

    flow.bind(8);

    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(flow.size(), 1u);
    ASSERT_EQ(flow[0u], 8);

    flow.swap(other);

    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(flow.size(), 0u);
    ASSERT_EQ(other[0u], 8);
}

TEST(Flow, Clear) {
    entt::flow flow{};

    flow.bind(0);
    flow.bind(4);

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow[0u], 0);
    ASSERT_EQ(flow[1u], 4);

    flow.clear();

    ASSERT_EQ(flow.size(), 0u);
}

TEST(Flow, Set) {
    entt::flow flow{};
    flow.bind(0).set(2, true).bind(1).set(2, true).set(3, false);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_NE(graph.edges().cbegin(), graph.edges().cend());

    ASSERT_TRUE(graph.contains(0u, 1u));
    ASSERT_FALSE(graph.contains(1u, 0u));
}

TEST(Flow, RO) {
    entt::flow flow{};
    flow.bind(0).ro(2).bind(1).ro(2).ro(3);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_EQ(graph.edges().cbegin(), graph.edges().cend());
}

TEST(Flow, RangeRO) {
    entt::flow flow{};
    const std::array<entt::id_type, 2u> res{10, 11};
    flow.bind(0).ro(res.begin(), res.begin() + 1u).bind(1).ro(res.begin(), res.end());
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_EQ(graph.edges().cbegin(), graph.edges().cend());
}

TEST(Flow, RW) {
    entt::flow flow{};
    flow.bind(0).rw(2).bind(1).rw(2).rw(3);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_NE(graph.edges().cbegin(), graph.edges().cend());

    ASSERT_TRUE(graph.contains(0u, 1u));
    ASSERT_FALSE(graph.contains(1u, 0u));
}

TEST(Flow, RangeRW) {
    entt::flow flow{};
    const std::array<entt::id_type, 2u> res{10, 11};
    flow.bind(0).rw(res.begin(), res.begin() + 1u).bind(1).rw(res.begin(), res.end());
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

    ASSERT_EQ(flow[0u], "task_0"_hs);
    ASSERT_EQ(flow[1u], "task_1"_hs);
    ASSERT_EQ(flow[2u], "task_2"_hs);
    ASSERT_EQ(flow[3u], "task_3"_hs);
    ASSERT_EQ(flow[4u], "task_4"_hs);

    auto it = graph.edges().cbegin();
    const auto last = graph.edges().cend();

    ASSERT_NE(it, last);
    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{2u}));
    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{4u}));
    ASSERT_EQ(*it++, std::make_pair(std::size_t{1u}, std::size_t{3u}));
    ASSERT_EQ(*it++, std::make_pair(std::size_t{1u}, std::size_t{4u}));
    ASSERT_EQ(*it++, std::make_pair(std::size_t{2u}, std::size_t{3u}));
    ASSERT_EQ(it, last);
}

TEST(Flow, Sync) {
    using namespace entt::literals;

    entt::flow flow{};

    flow.bind("task_0"_hs)
        .ro("resource_0"_hs);

    flow.bind("task_1"_hs)
        .rw("resource_1"_hs);

    flow.bind("task_2"_hs)
        .sync();

    flow.bind("task_3"_hs)
        .ro("resource_0"_hs)
        .rw("resource_2"_hs);

    flow.bind("task_4"_hs)
        .ro("resource_2"_hs);

    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 5u);
    ASSERT_EQ(flow.size(), graph.size());

    ASSERT_EQ(flow[0u], "task_0"_hs);
    ASSERT_EQ(flow[1u], "task_1"_hs);
    ASSERT_EQ(flow[2u], "task_2"_hs);
    ASSERT_EQ(flow[3u], "task_3"_hs);
    ASSERT_EQ(flow[4u], "task_4"_hs);

    auto it = graph.edges().cbegin();
    const auto last = graph.edges().cend();

    ASSERT_NE(it, last);
    ASSERT_EQ(*it++, std::make_pair(std::size_t{0u}, std::size_t{2u}));
    ASSERT_EQ(*it++, std::make_pair(std::size_t{1u}, std::size_t{2u}));
    ASSERT_EQ(*it++, std::make_pair(std::size_t{2u}, std::size_t{3u}));
    ASSERT_EQ(*it++, std::make_pair(std::size_t{3u}, std::size_t{4u}));
    ASSERT_EQ(it, last);
}

ENTT_DEBUG_TEST(FlowDeathTest, NoBind) {
    entt::flow flow{};

    ASSERT_DEATH(flow.ro(4), "");
    ASSERT_DEATH(flow.rw(4), "");

    flow.bind(0);

    ASSERT_NO_THROW(flow.ro(1));
    ASSERT_NO_THROW(flow.rw(2));
}

TEST(Flow, DirectRebind) {
    entt::flow flow{};
    flow.bind(0).ro(2).rw(2).bind(1).ro(2);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_NE(graph.edges().cbegin(), graph.edges().cend());

    ASSERT_TRUE(graph.contains(0u, 1u));
    ASSERT_FALSE(graph.contains(1u, 0u));
}

TEST(Flow, DeferredRebind) {
    entt::flow flow{};
    flow.bind(0).ro(2).bind(1).ro(2).bind(0).rw(2);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_NE(graph.edges().cbegin(), graph.edges().cend());

    ASSERT_FALSE(graph.contains(0u, 1u));
    ASSERT_TRUE(graph.contains(1u, 0u));
}

TEST(Flow, Loop) {
    entt::flow flow{};
    flow.bind(0).rw(2).bind(1).ro(2).bind(0).rw(2);
    auto graph = flow.graph();

    ASSERT_EQ(flow.size(), 2u);
    ASSERT_EQ(flow.size(), graph.size());
    ASSERT_NE(graph.edges().cbegin(), graph.edges().cend());

    ASSERT_TRUE(graph.contains(0u, 1u));
    ASSERT_TRUE(graph.contains(1u, 0u));
}

TEST(Flow, ThrowingAllocator) {
    entt::basic_flow<test::throwing_allocator<entt::id_type>> flow{};

    flow.get_allocator().throw_counter<std::pair<std::size_t, entt::id_type>>(0u);

    ASSERT_EQ(flow.size(), 0u);
    ASSERT_THROW(flow.bind(1), test::throwing_allocator_exception);
    ASSERT_EQ(flow.size(), 0u);

    flow.bind(1);

    ASSERT_EQ(flow.size(), 1u);
}
