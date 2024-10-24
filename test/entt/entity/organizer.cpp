#include <array>
#include <cstddef>
#include <utility>
#include <gtest/gtest.h>
#include <entt/core/type_info.hpp>
#include <entt/entity/organizer.hpp>
#include <entt/entity/registry.hpp>

void ro_int_rw_char_double(entt::view<entt::get_t<const int, char>>, double &) {}
void ro_char_rw_int(entt::view<entt::get_t<int, const char>>) {}
void ro_char_rw_double(entt::view<entt::get_t<const char>>, double &) {}
void ro_int_double(entt::view<entt::get_t<const int>>, const double &) {}
void sync_point(entt::registry &, entt::view<entt::get_t<const int>>) {}

struct clazz {
    void ro_int_char_double(entt::view<entt::get_t<const int, const char>>, const double &) {}
    void rw_int(entt::view<entt::get_t<int>>) {}
    void rw_int_char(entt::view<entt::get_t<int, char>>) {}
    void rw_int_char_double(entt::view<entt::get_t<int, char>>, double &) {}

    static void ro_int_with_payload(const clazz &, entt::view<entt::get_t<const int>>) {}
    static void ro_char_with_payload(const clazz &, entt::view<entt::get_t<const char>>) {}
    static void ro_int_char_with_payload(clazz &, entt::view<entt::get_t<const int, const char>>) {}
};

void to_args_integrity(entt::view<entt::get_t<int>> view, std::size_t &value, entt::registry &) {
    value = view.size();
}

TEST(Organizer, EmplaceFreeFunction) {
    entt::organizer organizer;
    entt::registry registry;

    organizer.emplace<&ro_int_rw_char_double>("t1");
    organizer.emplace<&ro_char_rw_int>("t2");
    organizer.emplace<&ro_char_rw_double>("t3");
    organizer.emplace<&ro_int_double>("t4");

    const auto graph = organizer.graph();

    ASSERT_EQ(graph.size(), 4u);

    ASSERT_STREQ(graph[0u].name(), "t1");
    ASSERT_STREQ(graph[1u].name(), "t2");
    ASSERT_STREQ(graph[2u].name(), "t3");
    ASSERT_STREQ(graph[3u].name(), "t4");

    ASSERT_EQ(graph[0u].ro_count(), 1u);
    ASSERT_EQ(graph[1u].ro_count(), 1u);
    ASSERT_EQ(graph[2u].ro_count(), 1u);
    ASSERT_EQ(graph[3u].ro_count(), 2u);

    ASSERT_EQ(graph[0u].rw_count(), 2u);
    ASSERT_EQ(graph[1u].rw_count(), 1u);
    ASSERT_EQ(graph[2u].rw_count(), 1u);
    ASSERT_EQ(graph[3u].rw_count(), 0u);

    ASSERT_NE(graph[0u].info(), graph[1u].info());
    ASSERT_NE(graph[1u].info(), graph[2u].info());
    ASSERT_NE(graph[2u].info(), graph[3u].info());

    ASSERT_TRUE(graph[0u].top_level());
    ASSERT_FALSE(graph[1u].top_level());
    ASSERT_FALSE(graph[2u].top_level());
    ASSERT_FALSE(graph[3u].top_level());

    ASSERT_EQ(graph[0u].in_edges().size(), 0u);
    ASSERT_EQ(graph[1u].in_edges().size(), 1u);
    ASSERT_EQ(graph[2u].in_edges().size(), 1u);
    ASSERT_EQ(graph[3u].in_edges().size(), 2u);

    ASSERT_EQ(graph[1u].in_edges()[0u], 0u);
    ASSERT_EQ(graph[2u].in_edges()[0u], 0u);
    ASSERT_EQ(graph[3u].in_edges()[0u], 1u);
    ASSERT_EQ(graph[3u].in_edges()[1u], 2u);

    ASSERT_EQ(graph[0u].out_edges().size(), 2u);
    ASSERT_EQ(graph[1u].out_edges().size(), 1u);
    ASSERT_EQ(graph[2u].out_edges().size(), 1u);
    ASSERT_EQ(graph[3u].out_edges().size(), 0u);

    ASSERT_EQ(graph[0u].out_edges()[0u], 1u);
    ASSERT_EQ(graph[0u].out_edges()[1u], 2u);
    ASSERT_EQ(graph[1u].out_edges()[0u], 3u);
    ASSERT_EQ(graph[2u].out_edges()[0u], 3u);

    for(auto &&vertex: graph) {
        typename entt::organizer::function_type *cb = vertex.callback();
        ASSERT_NO_THROW(cb(vertex.data(), registry));
    }

    organizer.clear();

    ASSERT_EQ(organizer.graph().size(), 0u);
}

TEST(Organizer, EmplaceMemberFunction) {
    entt::organizer organizer;
    entt::registry registry;
    clazz instance;

    organizer.emplace<&clazz::ro_int_char_double>(instance, "t1");
    organizer.emplace<&clazz::rw_int>(instance, "t2");
    organizer.emplace<&clazz::rw_int_char>(instance, "t3");
    organizer.emplace<&clazz::rw_int_char_double>(instance, "t4");

    const auto graph = organizer.graph();

    ASSERT_EQ(graph.size(), 4u);

    ASSERT_STREQ(graph[0u].name(), "t1");
    ASSERT_STREQ(graph[1u].name(), "t2");
    ASSERT_STREQ(graph[2u].name(), "t3");
    ASSERT_STREQ(graph[3u].name(), "t4");

    ASSERT_EQ(graph[0u].ro_count(), 3u);
    ASSERT_EQ(graph[1u].ro_count(), 0u);
    ASSERT_EQ(graph[2u].ro_count(), 0u);
    ASSERT_EQ(graph[3u].ro_count(), 0u);

    ASSERT_EQ(graph[0u].rw_count(), 0u);
    ASSERT_EQ(graph[1u].rw_count(), 1u);
    ASSERT_EQ(graph[2u].rw_count(), 2u);
    ASSERT_EQ(graph[3u].rw_count(), 3u);

    ASSERT_NE(graph[0u].info(), graph[1u].info());
    ASSERT_NE(graph[1u].info(), graph[2u].info());
    ASSERT_NE(graph[2u].info(), graph[3u].info());

    ASSERT_TRUE(graph[0u].top_level());
    ASSERT_FALSE(graph[1u].top_level());
    ASSERT_FALSE(graph[2u].top_level());
    ASSERT_FALSE(graph[3u].top_level());

    ASSERT_EQ(graph[0u].in_edges().size(), 0u);
    ASSERT_EQ(graph[1u].in_edges().size(), 1u);
    ASSERT_EQ(graph[2u].in_edges().size(), 1u);
    ASSERT_EQ(graph[3u].in_edges().size(), 1u);

    ASSERT_EQ(graph[1u].in_edges()[0u], 0u);
    ASSERT_EQ(graph[2u].in_edges()[0u], 1u);
    ASSERT_EQ(graph[3u].in_edges()[0u], 2u);

    ASSERT_EQ(graph[0u].out_edges().size(), 1u);
    ASSERT_EQ(graph[1u].out_edges().size(), 1u);
    ASSERT_EQ(graph[2u].out_edges().size(), 1u);
    ASSERT_EQ(graph[3u].out_edges().size(), 0u);

    ASSERT_EQ(graph[0u].out_edges()[0u], 1u);
    ASSERT_EQ(graph[1u].out_edges()[0u], 2u);
    ASSERT_EQ(graph[2u].out_edges()[0u], 3u);

    for(auto &&vertex: graph) {
        typename entt::organizer::function_type *cb = vertex.callback();
        ASSERT_NO_THROW(cb(vertex.data(), registry));
    }

    organizer.clear();

    ASSERT_EQ(organizer.graph().size(), 0u);
}

TEST(Organizer, EmplaceFreeFunctionWithPayload) {
    entt::organizer organizer;
    entt::registry registry;
    clazz instance;

    organizer.emplace<&clazz::ro_int_char_double>(instance, "t1");
    organizer.emplace<&clazz::ro_int_with_payload>(instance, "t2");
    organizer.emplace<&clazz::ro_char_with_payload, const clazz>(instance, "t3");
    organizer.emplace<&clazz::ro_int_char_with_payload, clazz>(instance, "t4");
    organizer.emplace<&clazz::rw_int_char>(instance, "t5");

    const auto graph = organizer.graph();

    ASSERT_EQ(graph.size(), 5u);

    ASSERT_STREQ(graph[0u].name(), "t1");
    ASSERT_STREQ(graph[1u].name(), "t2");
    ASSERT_STREQ(graph[2u].name(), "t3");
    ASSERT_STREQ(graph[3u].name(), "t4");
    ASSERT_STREQ(graph[4u].name(), "t5");

    ASSERT_EQ(graph[0u].ro_count(), 3u);
    ASSERT_EQ(graph[1u].ro_count(), 1u);
    ASSERT_EQ(graph[2u].ro_count(), 2u);
    ASSERT_EQ(graph[3u].ro_count(), 2u);
    ASSERT_EQ(graph[4u].ro_count(), 0u);

    ASSERT_EQ(graph[0u].rw_count(), 0u);
    ASSERT_EQ(graph[1u].rw_count(), 0u);
    ASSERT_EQ(graph[2u].rw_count(), 0u);
    ASSERT_EQ(graph[3u].rw_count(), 1u);
    ASSERT_EQ(graph[4u].rw_count(), 2u);

    ASSERT_NE(graph[0u].info(), graph[1u].info());
    ASSERT_NE(graph[1u].info(), graph[2u].info());
    ASSERT_NE(graph[2u].info(), graph[3u].info());
    ASSERT_NE(graph[3u].info(), graph[4u].info());

    ASSERT_TRUE(graph[0u].top_level());
    ASSERT_TRUE(graph[1u].top_level());
    ASSERT_TRUE(graph[2u].top_level());
    ASSERT_FALSE(graph[3u].top_level());
    ASSERT_FALSE(graph[4u].top_level());

    ASSERT_EQ(graph[3u].in_edges().size(), 1u);
    ASSERT_EQ(graph[4u].in_edges().size(), 3u);

    ASSERT_EQ(graph[3u].in_edges()[0u], 2u);
    ASSERT_EQ(graph[4u].in_edges()[0u], 0u);
    ASSERT_EQ(graph[4u].in_edges()[1u], 1u);
    ASSERT_EQ(graph[4u].in_edges()[2u], 3u);

    ASSERT_EQ(graph[0u].out_edges().size(), 1u);
    ASSERT_EQ(graph[1u].out_edges().size(), 1u);
    ASSERT_EQ(graph[2u].out_edges().size(), 1u);
    ASSERT_EQ(graph[3u].out_edges().size(), 1u);
    ASSERT_EQ(graph[4u].out_edges().size(), 0u);

    ASSERT_EQ(graph[0u].out_edges()[0u], 4u);
    ASSERT_EQ(graph[1u].out_edges()[0u], 4u);
    ASSERT_EQ(graph[2u].out_edges()[0u], 3u);
    ASSERT_EQ(graph[3u].out_edges()[0u], 4u);

    for(auto &&vertex: graph) {
        typename entt::organizer::function_type *cb = vertex.callback();
        ASSERT_NO_THROW(cb(vertex.data(), registry));
    }

    organizer.clear();

    ASSERT_EQ(organizer.graph().size(), 0u);
}

TEST(Organizer, EmplaceDirectFunction) {
    entt::organizer organizer;
    entt::registry registry;
    clazz instance;

    // no aggressive comdat
    auto t1 = +[](const void *, entt::registry &reg) { reg.clear<int>(); };
    auto t2 = +[](const void *, entt::registry &reg) { reg.clear<char>(); };
    auto t3 = +[](const void *, entt::registry &reg) { reg.clear<double>(); };
    auto t4 = +[](const void *, entt::registry &reg) { reg.clear(); };

    organizer.emplace<int>(t1, nullptr, "t1");
    organizer.emplace<const int>(t2, &instance, "t2");
    organizer.emplace<const int, char>(t3, nullptr, "t3");
    organizer.emplace<int, char, double>(t4, &instance, "t4");

    const auto graph = organizer.graph();

    ASSERT_EQ(graph.size(), 4u);

    ASSERT_STREQ(graph[0u].name(), "t1");
    ASSERT_STREQ(graph[1u].name(), "t2");
    ASSERT_STREQ(graph[2u].name(), "t3");
    ASSERT_STREQ(graph[3u].name(), "t4");

    ASSERT_EQ(graph[0u].ro_count(), 0u);
    ASSERT_EQ(graph[1u].ro_count(), 1u);
    ASSERT_EQ(graph[2u].ro_count(), 1u);
    ASSERT_EQ(graph[3u].ro_count(), 0u);

    ASSERT_EQ(graph[0u].rw_count(), 1u);
    ASSERT_EQ(graph[1u].rw_count(), 0u);
    ASSERT_EQ(graph[2u].rw_count(), 1u);
    ASSERT_EQ(graph[3u].rw_count(), 3u);

    ASSERT_TRUE(graph[0u].callback() == t1);
    ASSERT_TRUE(graph[1u].callback() == t2);
    ASSERT_TRUE(graph[2u].callback() == t3);
    ASSERT_TRUE(graph[3u].callback() == t4);

    ASSERT_EQ(graph[0u].data(), nullptr);
    ASSERT_EQ(graph[1u].data(), &instance);
    ASSERT_EQ(graph[2u].data(), nullptr);
    ASSERT_EQ(graph[3u].data(), &instance);

    ASSERT_EQ(graph[0u].info(), entt::type_id<void>());
    ASSERT_EQ(graph[1u].info(), entt::type_id<void>());
    ASSERT_EQ(graph[2u].info(), entt::type_id<void>());
    ASSERT_EQ(graph[3u].info(), entt::type_id<void>());

    ASSERT_TRUE(graph[0u].top_level());
    ASSERT_FALSE(graph[1u].top_level());
    ASSERT_FALSE(graph[2u].top_level());
    ASSERT_FALSE(graph[3u].top_level());

    ASSERT_EQ(graph[1u].in_edges().size(), 1u);
    ASSERT_EQ(graph[2u].in_edges().size(), 1u);
    ASSERT_EQ(graph[3u].in_edges().size(), 1u);

    ASSERT_EQ(graph[1u].in_edges()[0u], 0u);
    ASSERT_EQ(graph[2u].in_edges()[0u], 1u);
    ASSERT_EQ(graph[3u].in_edges()[0u], 2u);

    ASSERT_EQ(graph[0u].out_edges().size(), 1u);
    ASSERT_EQ(graph[1u].out_edges().size(), 1u);
    ASSERT_EQ(graph[2u].out_edges().size(), 1u);
    ASSERT_EQ(graph[3u].out_edges().size(), 0u);

    ASSERT_EQ(graph[0u].out_edges()[0u], 1u);
    ASSERT_EQ(graph[1u].out_edges()[0u], 2u);
    ASSERT_EQ(graph[2u].out_edges()[0u], 3u);

    for(auto &&vertex: graph) {
        typename entt::organizer::function_type *cb = vertex.callback();
        ASSERT_NO_THROW(cb(vertex.data(), registry));
    }

    organizer.clear();

    ASSERT_EQ(organizer.graph().size(), 0u);
}

TEST(Organizer, SyncPoint) {
    entt::organizer organizer;
    entt::registry registry;
    clazz instance;

    organizer.emplace<&ro_int_double>("before");
    organizer.emplace<&sync_point>("sync_1");
    organizer.emplace<&clazz::ro_int_char_double>(instance, "mid_1");
    organizer.emplace<&ro_int_double>("mid_2");
    organizer.emplace<&sync_point>("sync_2");
    organizer.emplace<&ro_int_double>("after");

    const auto graph = organizer.graph();

    ASSERT_EQ(graph.size(), 6u);

    ASSERT_STREQ(graph[0u].name(), "before");
    ASSERT_STREQ(graph[1u].name(), "sync_1");
    ASSERT_STREQ(graph[2u].name(), "mid_1");
    ASSERT_STREQ(graph[3u].name(), "mid_2");
    ASSERT_STREQ(graph[4u].name(), "sync_2");
    ASSERT_STREQ(graph[5u].name(), "after");

    ASSERT_TRUE(graph[0u].top_level());
    ASSERT_FALSE(graph[1u].top_level());
    ASSERT_FALSE(graph[2u].top_level());
    ASSERT_FALSE(graph[3u].top_level());
    ASSERT_FALSE(graph[4u].top_level());
    ASSERT_FALSE(graph[5u].top_level());

    ASSERT_EQ(graph[1u].in_edges().size(), 1u);
    ASSERT_EQ(graph[2u].in_edges().size(), 1u);
    ASSERT_EQ(graph[3u].in_edges().size(), 1u);
    ASSERT_EQ(graph[4u].in_edges().size(), 2u);
    ASSERT_EQ(graph[5u].in_edges().size(), 1u);

    ASSERT_EQ(graph[1u].in_edges()[0u], 0u);
    ASSERT_EQ(graph[2u].in_edges()[0u], 1u);
    ASSERT_EQ(graph[3u].in_edges()[0u], 1u);
    ASSERT_EQ(graph[4u].in_edges()[0u], 2u);
    ASSERT_EQ(graph[4u].in_edges()[1u], 3u);
    ASSERT_EQ(graph[5u].in_edges()[0u], 4u);

    ASSERT_EQ(graph[0u].out_edges().size(), 1u);
    ASSERT_EQ(graph[1u].out_edges().size(), 2u);
    ASSERT_EQ(graph[2u].out_edges().size(), 1u);
    ASSERT_EQ(graph[3u].out_edges().size(), 1u);
    ASSERT_EQ(graph[4u].out_edges().size(), 1u);
    ASSERT_EQ(graph[5u].out_edges().size(), 0u);

    ASSERT_EQ(graph[0u].out_edges()[0u], 1u);
    ASSERT_EQ(graph[1u].out_edges()[0u], 2u);
    ASSERT_EQ(graph[1u].out_edges()[1u], 3u);
    ASSERT_EQ(graph[2u].out_edges()[0u], 4u);
    ASSERT_EQ(graph[3u].out_edges()[0u], 4u);
    ASSERT_EQ(graph[4u].out_edges()[0u], 5u);

    for(auto &&vertex: graph) {
        typename entt::organizer::function_type *cb = vertex.callback();
        ASSERT_NO_THROW(cb(vertex.data(), registry));
    }
}

TEST(Organizer, Override) {
    entt::organizer organizer;

    organizer.emplace<&ro_int_rw_char_double, const char, const double>("t1");
    organizer.emplace<&ro_char_rw_double, const double>("t2");
    organizer.emplace<&ro_int_double, double>("t3");

    const auto graph = organizer.graph();

    ASSERT_EQ(graph.size(), 3u);

    ASSERT_STREQ(graph[0u].name(), "t1");
    ASSERT_STREQ(graph[1u].name(), "t2");
    ASSERT_STREQ(graph[2u].name(), "t3");

    ASSERT_TRUE(graph[0u].top_level());
    ASSERT_TRUE(graph[1u].top_level());
    ASSERT_FALSE(graph[2u].top_level());

    ASSERT_EQ(graph[2u].in_edges().size(), 2u);

    ASSERT_EQ(graph[2u].in_edges()[0u], 0u);
    ASSERT_EQ(graph[2u].in_edges()[1u], 1u);

    ASSERT_EQ(graph[0u].out_edges().size(), 1u);
    ASSERT_EQ(graph[1u].out_edges().size(), 1u);
    ASSERT_EQ(graph[2u].out_edges().size(), 0u);

    ASSERT_EQ(graph[0u].out_edges()[0u], 2u);
    ASSERT_EQ(graph[1u].out_edges()[0u], 2u);
}

TEST(Organizer, Prepare) {
    entt::organizer organizer;
    entt::registry registry;
    clazz instance;

    organizer.emplace<&ro_int_double>();
    organizer.emplace<&clazz::rw_int_char>(instance);

    const auto graph = organizer.graph();

    ASSERT_FALSE(registry.ctx().contains<int>());
    ASSERT_FALSE(registry.ctx().contains<char>());
    ASSERT_FALSE(registry.ctx().contains<double>());

    ASSERT_EQ(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_EQ(std::as_const(registry).storage<char>(), nullptr);
    ASSERT_EQ(std::as_const(registry).storage<double>(), nullptr);

    for(auto &&vertex: graph) {
        vertex.prepare(registry);
    }

    ASSERT_FALSE(registry.ctx().contains<int>());
    ASSERT_FALSE(registry.ctx().contains<char>());
    ASSERT_TRUE(registry.ctx().contains<double>());

    ASSERT_NE(std::as_const(registry).storage<int>(), nullptr);
    ASSERT_NE(std::as_const(registry).storage<char>(), nullptr);
    ASSERT_EQ(std::as_const(registry).storage<double>(), nullptr);
}

TEST(Organizer, Dependencies) {
    entt::organizer organizer;
    clazz instance;

    organizer.emplace<&ro_int_double>();
    organizer.emplace<&clazz::rw_int_char>(instance);
    organizer.emplace<char, const double>(+[](const void *, entt::registry &) {});

    const auto graph = organizer.graph();
    constexpr auto number_of_elements = 5u;
    std::array<const entt::type_info *, number_of_elements> buffer{};

    ASSERT_EQ(graph.size(), 3u);

    ASSERT_EQ(graph[0u].ro_count(), 2u);
    ASSERT_EQ(graph[0u].rw_count(), 0u);

    ASSERT_EQ(graph[0u].ro_dependency(buffer.data(), 0u), 0u);
    ASSERT_EQ(graph[0u].rw_dependency(buffer.data(), 2u), 0u);

    ASSERT_EQ(graph[0u].ro_dependency(buffer.data(), 5u), 2u);
    ASSERT_EQ(*buffer[0u], entt::type_id<int>());
    ASSERT_EQ(*buffer[1u], entt::type_id<double>());

    ASSERT_EQ(graph[1u].ro_count(), 0u);
    ASSERT_EQ(graph[1u].rw_count(), 2u);

    ASSERT_EQ(graph[1u].ro_dependency(buffer.data(), 2u), 0u);
    ASSERT_EQ(graph[1u].rw_dependency(buffer.data(), 0u), 0u);

    ASSERT_EQ(graph[1u].rw_dependency(buffer.data(), 5u), 2u);
    ASSERT_EQ(*buffer[0u], entt::type_id<int>());
    ASSERT_EQ(*buffer[1u], entt::type_id<char>());

    ASSERT_EQ(graph[2u].ro_count(), 1u);
    ASSERT_EQ(graph[2u].rw_count(), 1u);

    ASSERT_EQ(graph[2u].ro_dependency(buffer.data(), 2u), 1u);
    ASSERT_EQ(graph[2u].rw_dependency(buffer.data(), 0u), 0u);

    ASSERT_EQ(graph[2u].ro_dependency(buffer.data(), 5u), 1u);
    ASSERT_EQ(*buffer[0u], entt::type_id<double>());

    ASSERT_EQ(graph[2u].rw_dependency(buffer.data(), 5u), 1u);
    ASSERT_EQ(*buffer[0u], entt::type_id<char>());
}

TEST(Organizer, ToArgsIntegrity) {
    entt::organizer organizer;
    entt::registry registry;

    organizer.emplace<&to_args_integrity>();
    registry.ctx().emplace<std::size_t>(2u);

    auto graph = organizer.graph();
    graph[0u].callback()(graph[0u].data(), registry);

    ASSERT_EQ(registry.ctx().get<std::size_t>(), 0u);
}
