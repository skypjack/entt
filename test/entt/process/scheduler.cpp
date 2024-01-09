#include <functional>
#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/process/process.hpp>
#include <entt/process/scheduler.hpp>

struct foo_process: entt::process<foo_process, entt::scheduler::delta_type> {
    foo_process(std::function<void()> upd, std::function<void()> abort)
        : on_update{std::move(upd)}, on_aborted{std::move(abort)} {}

    void update(delta_type, void *) {
        on_update();
    }

    void aborted() {
        on_aborted();
    }

    std::function<void()> on_update;
    std::function<void()> on_aborted;
};

struct succeeded_process: entt::process<succeeded_process, entt::scheduler::delta_type> {
    void update(delta_type, void *) {
        ++invoked;
        succeed();
    }

    inline static unsigned int invoked; // NOLINT
};

struct failed_process: entt::process<failed_process, entt::scheduler::delta_type> {
    void update(delta_type, void *) {
        ++invoked;
        fail();
    }

    inline static unsigned int invoked; // NOLINT
};

struct Scheduler: ::testing::Test {
    void SetUp() override {
        succeeded_process::invoked = 0u;
        failed_process::invoked = 0u;
    }
};

TEST_F(Scheduler, Functionalities) {
    entt::scheduler scheduler{};
    entt::scheduler other{std::move(scheduler)};

    scheduler = std::move(other);

    bool updated = false;
    bool aborted = false;

    ASSERT_EQ(scheduler.size(), 0u);
    ASSERT_TRUE(scheduler.empty());

    scheduler.attach<foo_process>(
        [&updated]() { updated = true; },
        [&aborted]() { aborted = true; });

    ASSERT_NE(scheduler.size(), 0u);
    ASSERT_FALSE(scheduler.empty());

    scheduler.update(0);
    scheduler.abort(true);

    ASSERT_TRUE(updated);
    ASSERT_TRUE(aborted);

    ASSERT_NE(scheduler.size(), 0u);
    ASSERT_FALSE(scheduler.empty());

    scheduler.clear();

    ASSERT_EQ(scheduler.size(), 0u);
    ASSERT_TRUE(scheduler.empty());
}

TEST_F(Scheduler, Swap) {
    entt::scheduler scheduler{};
    entt::scheduler other{};
    int counter{};

    scheduler.attach([&counter](auto &&...) { ++counter; });

    ASSERT_EQ(scheduler.size(), 1u);
    ASSERT_EQ(other.size(), 0u);
    ASSERT_EQ(counter, 0);

    scheduler.update({});

    ASSERT_EQ(counter, 1);

    scheduler.swap(other);
    scheduler.update({});

    ASSERT_EQ(scheduler.size(), 0u);
    ASSERT_EQ(other.size(), 1u);
    ASSERT_EQ(counter, 1);

    other.update({});

    ASSERT_EQ(counter, 2);
}

TEST_F(Scheduler, Then) {
    entt::scheduler scheduler{};

    scheduler
        // failing process with successor
        .attach<succeeded_process>()
        .then<succeeded_process>()
        .then<failed_process>()
        .then<succeeded_process>()
        // failing process without successor
        .attach<succeeded_process>()
        .then<succeeded_process>()
        .then<failed_process>()
        // non-failing process
        .attach<succeeded_process>()
        .then<succeeded_process>();

    ASSERT_EQ(succeeded_process::invoked, 0u);
    ASSERT_EQ(failed_process::invoked, 0u);

    while(!scheduler.empty()) {
        scheduler.update(0);
    }

    ASSERT_EQ(succeeded_process::invoked, 6u);
    ASSERT_EQ(failed_process::invoked, 2u);
}

TEST_F(Scheduler, Functor) {
    entt::scheduler scheduler{};

    bool first_functor = false;
    bool second_functor = false;

    auto attach = [&first_functor](auto, void *, auto resolve, auto) {
        ASSERT_FALSE(first_functor);
        first_functor = true;
        resolve();
    };

    auto then = [&second_functor](auto, void *, auto, auto reject) {
        ASSERT_FALSE(second_functor);
        second_functor = true;
        reject();
    };

    scheduler.attach(std::move(attach)).then(std::move(then)).then([](auto...) { FAIL(); });

    while(!scheduler.empty()) {
        scheduler.update(0);
    }

    ASSERT_TRUE(first_functor);
    ASSERT_TRUE(second_functor);
    ASSERT_TRUE(scheduler.empty());
}

TEST_F(Scheduler, SpawningProcess) {
    entt::scheduler scheduler{};

    scheduler.attach([&scheduler](auto, void *, auto resolve, auto) {
        scheduler.attach<succeeded_process>().then<failed_process>();
        resolve();
    });

    ASSERT_EQ(succeeded_process::invoked, 0u);
    ASSERT_EQ(failed_process::invoked, 0u);

    while(!scheduler.empty()) {
        scheduler.update(0);
    }

    ASSERT_EQ(succeeded_process::invoked, 1u);
    ASSERT_EQ(failed_process::invoked, 1u);
}

TEST_F(Scheduler, CustomAllocator) {
    const std::allocator<void> allocator{};
    entt::scheduler scheduler{allocator};

    ASSERT_EQ(scheduler.get_allocator(), allocator);
    ASSERT_FALSE(scheduler.get_allocator() != allocator);

    scheduler.attach([](auto &&...) {});
    const decltype(scheduler) other{std::move(scheduler), allocator};

    ASSERT_EQ(other.size(), 1u);
}
