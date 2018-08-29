#include <unordered_set>
#include <gtest/gtest.h>
#include <entt/entity/attachee.hpp>

TEST(AttacheeNoType, Functionalities) {
    entt::Attachee<std::uint64_t> attachee;

    attachee.construct(42u);

    ASSERT_EQ(attachee.get(), 42u);

    attachee.destroy();

    ASSERT_NE(attachee.get(), 42u);

    (void)entt::Attachee<std::uint64_t>{std::move(attachee)};
    entt::Attachee<std::uint64_t> other;
    other = std::move(attachee);
}

TEST(AttacheeWithType, Functionalities) {
    entt::Attachee<std::uint64_t, int> attachee;
    const auto &cattachee = attachee;

    attachee.construct(42u, 3);

    ASSERT_EQ(attachee.get(), 3);
    ASSERT_EQ(cattachee.get(), 3);
    ASSERT_EQ(attachee.Attachee<std::uint64_t>::get(), 42u);

    attachee.move(0u);

    ASSERT_EQ(attachee.get(), 3);
    ASSERT_EQ(cattachee.get(), 3);
    ASSERT_EQ(attachee.Attachee<std::uint64_t>::get(), 0u);

    attachee.destroy();

    ASSERT_NE(attachee.Attachee<std::uint64_t>::get(), 0u);
    ASSERT_NE(attachee.Attachee<std::uint64_t>::get(), 42u);
}

TEST(AttacheeWithType, AggregatesMustWork) {
    struct AggregateType { int value; };
    // the goal of this test is to enforce the requirements for aggregate types
    entt::Attachee<std::uint64_t, AggregateType>{}.construct(0, 42);
}

TEST(AttacheeWithType, TypesFromStandardTemplateLibraryMustWork) {
    // see #37 - this test shouldn't crash, that's all
    entt::Attachee<std::uint64_t, std::unordered_set<int>> attachee;
    attachee.construct(0).insert(42);
    attachee.destroy();
}

TEST(AttacheeWithType, MoveOnlyComponent) {
    struct MoveOnlyComponent {
        MoveOnlyComponent() = default;
        ~MoveOnlyComponent() = default;
        MoveOnlyComponent(const MoveOnlyComponent &) = delete;
        MoveOnlyComponent(MoveOnlyComponent &&) = default;
        MoveOnlyComponent & operator=(const MoveOnlyComponent &) = delete;
        MoveOnlyComponent & operator=(MoveOnlyComponent &&) = default;
    };

    // it's purpose is to ensure that move only components are always accepted
    entt::Attachee<std::uint64_t, MoveOnlyComponent> attachee;
    (void)attachee;
}
