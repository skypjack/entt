#include <functional>
#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/process/process.hpp>
#include <entt/process/scheduler.hpp>
#include "../../common/config.h"

class foo_process: public entt::process {
    void update(const delta_type, void *) override {
        on_update();
    }

    void aborted() override {
        on_aborted();
    }

public:
    using allocator_type = typename entt::process::allocator_type;

    foo_process(const allocator_type &allocator, std::function<void()> upd, std::function<void()> abort)
        : entt::process{allocator},
          on_update{std::move(upd)},
          on_aborted{std::move(abort)} {}

private:
    std::function<void()> on_update;
    std::function<void()> on_aborted;
};

class succeeded_process: public entt::process {
    void update(const delta_type, void *data) override {
        ++static_cast<std::pair<int, int> *>(data)->first;
        succeed();
    }

public:
    using entt::process::process;
};

class failed_process: public entt::process {
    void update(const delta_type, void *data) override {
        ++static_cast<std::pair<int, int> *>(data)->second;
        fail();
    }

public:
    using entt::process::process;
};

TEST(Scheduler, Functionalities) {
    entt::scheduler scheduler{};
    entt::scheduler other{std::move(scheduler)};

    scheduler = std::move(other);

    bool updated = false;
    bool aborted = false;

    ASSERT_EQ(scheduler.size(), 0u);
    ASSERT_TRUE(scheduler.empty());

    scheduler.attach<foo_process>([&updated]() { updated = true; }, [&aborted]() { aborted = true; });

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

TEST(Scheduler, Swap) {
    entt::scheduler scheduler{};
    entt::scheduler other{};
    int counter{};

    scheduler.attach([&counter](entt::process &, std::uint32_t, void *) { ++counter; });

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

TEST(Scheduler, SharedFromThis) {
    entt::scheduler scheduler{};
    auto &process = scheduler.attach<succeeded_process>();
    const auto &then = process.then<failed_process>();
    auto other = process.shared_from_this();

    ASSERT_TRUE(other);
    ASSERT_NE(&process, &then);
    ASSERT_EQ(&process, other.get());
    ASSERT_EQ(process.get_allocator(), scheduler.get_allocator());
    ASSERT_EQ(process.get_allocator(), other->get_allocator());
    ASSERT_EQ(then.get_allocator(), scheduler.get_allocator());
}

TEST(Scheduler, AttachThen) {
    entt::scheduler scheduler{};
    std::pair<int, int> counter{};

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

    while(!scheduler.empty()) {
        scheduler.update(0, &counter);
    }

    ASSERT_EQ(counter.first, 6u);
    ASSERT_EQ(counter.second, 2u);
}

TEST(Scheduler, Functor) {
    entt::scheduler scheduler{};

    bool first_functor = false;
    bool second_functor = false;

    scheduler
        .attach([&first_functor](entt::process &proc, std::uint32_t, void *) {
            ASSERT_FALSE(first_functor);
            first_functor = true;
            proc.succeed();
        })
        .then([&second_functor](entt::process &proc, std::uint32_t, void *) {
            ASSERT_FALSE(second_functor);
            second_functor = true;
            proc.fail();
        })
        .then([](entt::process &, std::uint32_t, void *) { FAIL(); });

    while(!scheduler.empty()) {
        scheduler.update(0);
    }

    ASSERT_TRUE(first_functor);
    ASSERT_TRUE(second_functor);
    ASSERT_TRUE(scheduler.empty());
}

TEST(Scheduler, SpawningProcess) {
    entt::scheduler scheduler{};
    std::pair<int, int> counter{};

    scheduler.attach([&scheduler](entt::process &proc, std::uint32_t, void *) {
        scheduler.attach<succeeded_process>().then<failed_process>();
        proc.succeed();
    });

    while(!scheduler.empty()) {
        scheduler.update(0, &counter);
    }

    ASSERT_EQ(counter.first, 1u);
    ASSERT_EQ(counter.second, 1u);
}

TEST(Scheduler, CustomAllocator) {
    const std::allocator<void> allocator{};
    entt::scheduler scheduler{allocator};

    ASSERT_EQ(scheduler.get_allocator(), allocator);
    ASSERT_FALSE(scheduler.get_allocator() != allocator);

    scheduler.attach([](entt::process &, std::uint32_t, void *) {});
    const decltype(scheduler) other{std::move(scheduler), allocator};

    ASSERT_EQ(other.size(), 1u);
}
