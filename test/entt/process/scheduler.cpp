#include <functional>
#include <gtest/gtest.h>
#include <entt/process/scheduler.hpp>
#include <entt/process/process.hpp>

struct FooProcess: entt::Process<FooProcess, int> {
    FooProcess(std::function<void()> onUpdate, std::function<void()> onAborted)
        : onUpdate{onUpdate}, onAborted{onAborted}
    {}

    void update(delta_type, void *) { onUpdate(); }
    void aborted() { onAborted(); }

    std::function<void()> onUpdate;
    std::function<void()> onAborted;
};

struct SucceededProcess: entt::Process<SucceededProcess, int> {
    void update(delta_type, void *) {
        ASSERT_FALSE(updated);
        updated = true;
        ++invoked;
        succeed();
    }

    static unsigned int invoked;
    bool updated = false;
};

unsigned int SucceededProcess::invoked = 0;

struct FailedProcess: entt::Process<FailedProcess, int> {
    void update(delta_type, void *) {
        ASSERT_FALSE(updated);
        updated = true;
        fail();
    }

    bool updated = false;
};

TEST(Scheduler, Functionalities) {
    entt::Scheduler<int> scheduler{};

    bool updated = false;
    bool aborted = false;

    ASSERT_EQ(scheduler.size(), entt::Scheduler<int>::size_type{});
    ASSERT_TRUE(scheduler.empty());

    scheduler.attach<FooProcess>(
                [&updated](){ updated = true; },
                [&aborted](){ aborted = true; }
    );

    ASSERT_NE(scheduler.size(), entt::Scheduler<int>::size_type{});
    ASSERT_FALSE(scheduler.empty());

    scheduler.update(0);
    scheduler.abort(true);

    ASSERT_TRUE(updated);
    ASSERT_TRUE(aborted);

    ASSERT_NE(scheduler.size(), entt::Scheduler<int>::size_type{});
    ASSERT_FALSE(scheduler.empty());

    scheduler.clear();

    ASSERT_EQ(scheduler.size(), entt::Scheduler<int>::size_type{});
    ASSERT_TRUE(scheduler.empty());
}

TEST(Scheduler, Then) {
    entt::Scheduler<int> scheduler;

    scheduler.attach<SucceededProcess>()
            .then<SucceededProcess>()
            .then<FailedProcess>()
            .then<SucceededProcess>();

    for(auto i = 0; i < 8; ++i) {
        scheduler.update(0);
    }

    ASSERT_EQ(SucceededProcess::invoked, 2u);
}

TEST(Scheduler, Functor) {
    entt::Scheduler<int> scheduler;

    bool firstFunctor = false;
    bool secondFunctor = false;

    scheduler.attach([&firstFunctor](auto, void *, auto resolve, auto){
        ASSERT_FALSE(firstFunctor);
        firstFunctor = true;
        resolve();
    }).then([&secondFunctor](auto, void *, auto, auto reject){
        ASSERT_FALSE(secondFunctor);
        secondFunctor = true;
        reject();
    }).then([](auto...){
        FAIL();
    });

    for(auto i = 0; i < 8; ++i) {
        scheduler.update(0);
    }

    ASSERT_TRUE(firstFunctor);
    ASSERT_TRUE(secondFunctor);
}
