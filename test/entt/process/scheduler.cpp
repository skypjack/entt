#include <functional>
#include <gtest/gtest.h>
#include <entt/process/scheduler.hpp>
#include <entt/process/process.hpp>

struct foo_process: entt::process<foo_process, int> {
    foo_process(std::function<void()> upd, std::function<void()> abort)
        : on_update{upd}, on_aborted{abort}
    {}

    void update(delta_type, void *) { on_update(); }
    void aborted() { on_aborted(); }

    std::function<void()> on_update;
    std::function<void()> on_aborted;
};

struct succeeded_process: entt::process<succeeded_process, int> {
    void update(delta_type, void *) {
        ASSERT_FALSE(updated);
        updated = true;
        ++invoked;
        succeed();
    }

    static unsigned int invoked;
    bool updated = false;
};

unsigned int succeeded_process::invoked = 0;

struct failed_process: entt::process<failed_process, int> {
    void update(delta_type, void *) {
        ASSERT_FALSE(updated);
        updated = true;
        fail();
    }

    bool updated = false;
};

TEST(Scheduler, Functionalities) {
    entt::scheduler<int> scheduler{};

    bool updated = false;
    bool aborted = false;

    ASSERT_EQ(scheduler.size(), 0u);
    ASSERT_TRUE(scheduler.empty());

    scheduler.attach<foo_process>(
        [&updated](){ updated = true; },
        [&aborted](){ aborted = true; }
    );

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

TEST(Scheduler, Then) {
    entt::scheduler<int> scheduler;

    // failing process with successor
    scheduler.attach<succeeded_process>()
        .then<succeeded_process>()
        .then<failed_process>()
        .then<succeeded_process>();

    // failing process without successor
    scheduler.attach<succeeded_process>()
        .then<succeeded_process>()
        .then<failed_process>();

    // non-failing process
    scheduler.attach<succeeded_process>()
        .then<succeeded_process>();

    for(auto i = 0; i < 8; ++i) {
        scheduler.update(0);
    }

    ASSERT_EQ(succeeded_process::invoked, 6u);
    ASSERT_TRUE(scheduler.empty());
}

TEST(Scheduler, Functor) {
    entt::scheduler<int> scheduler;

    bool first_functor = false;
    bool second_functor = false;

    scheduler.attach([&first_functor](auto, void *, auto resolve, auto){
        ASSERT_FALSE(first_functor);
        first_functor = true;
        resolve();
    }).then([&second_functor](auto, void *, auto, auto reject){
        ASSERT_FALSE(second_functor);
        second_functor = true;
        reject();
    }).then([](auto...){ FAIL(); });

    for(auto i = 0; i < 8; ++i) {
        scheduler.update(0);
    }

    ASSERT_TRUE(first_functor);
    ASSERT_TRUE(second_functor);
    ASSERT_TRUE(scheduler.empty());
}
