#include <cstdint>
#include <memory>
#include <gtest/gtest.h>
#include <entt/process/process.hpp>
#include "../../common/config.h"
#include "../../common/empty.h"

template<typename Delta>
class test_process: public entt::basic_process<Delta> {
    using entt::basic_process<Delta>::basic_process;

    void succeeded() override {
        succeeded_invoked = true;
    }

    void failed() override {
        failed_invoked = true;
    }

    void aborted() override {
        aborted_invoked = true;
    }

    void update(const Delta, void *data) override {
        if(data != nullptr) {
            (*static_cast<int *>(data))++;
        }

        update_invoked = true;
    }

public:
    bool update_invoked{};
    bool succeeded_invoked{};
    bool failed_invoked{};
    bool aborted_invoked{};
};

TEST(Process, Basics) {
    test_process<int> process{};

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    process.succeed();
    process.fail();
    process.abort();
    process.pause();
    process.unpause();

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    process.tick(0);

    ASSERT_TRUE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    process.pause();

    ASSERT_TRUE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_TRUE(process.paused());
    ASSERT_FALSE(process.rejected());

    process.unpause();

    ASSERT_TRUE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    process.fail();

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    process.tick(0);

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_TRUE(process.rejected());
}

TEST(Process, Succeeded) {
    test_process<test::empty> process{};

    process.tick({});
    process.tick({});
    process.succeed();
    process.tick({});

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    ASSERT_TRUE(process.update_invoked);
    ASSERT_TRUE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, Fail) {
    test_process<int> process{};

    process.tick(0);
    process.tick(0);
    process.fail();
    process.tick(0);

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_TRUE(process.rejected());

    ASSERT_TRUE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_TRUE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, Data) {
    test_process<test::empty> process{};
    int value = 0;

    process.tick({});
    process.tick({}, &value);
    process.succeed();
    process.tick({}, &value);

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    ASSERT_EQ(value, 1);
    ASSERT_TRUE(process.update_invoked);
    ASSERT_TRUE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, AbortNextTick) {
    test_process<int> process{};

    process.tick(0);
    process.abort();
    process.tick(0);

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_TRUE(process.rejected());

    ASSERT_TRUE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_TRUE(process.aborted_invoked);
}

TEST(Process, AbortImmediately) {
    test_process<test::empty> process{};

    process.tick({});
    process.abort();
    process.tick({});

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_TRUE(process.rejected());

    ASSERT_TRUE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_TRUE(process.aborted_invoked);
}

TEST(Process, ThenPeek) {
    test_process<int> process{};

    ASSERT_FALSE(process.peek());

    process.then<test_process<int>>().then<test_process<int>>();

    ASSERT_TRUE(process.peek());
    ASSERT_TRUE(process.peek()->peek());
    ASSERT_FALSE(process.peek()->peek()->peek());
    // peek does not release ownership
    ASSERT_TRUE(process.peek());
}

TEST(Process, CustomAllocator) {
    const std::allocator<void> allocator{};
    entt::process process{allocator};

    ASSERT_EQ(process.get_allocator(), allocator);
    ASSERT_FALSE(process.get_allocator() != allocator);

    entt::process &other = process.then([](entt::process &, std::uint32_t, void *) {});

    ASSERT_NE(&process, &other);
    ASSERT_EQ(other.get_allocator(), allocator);
    ASSERT_FALSE(other.get_allocator() != allocator);
}
