#include <cstdint>
#include <memory>
#include <gtest/gtest.h>
#include <entt/process/process.hpp>
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
    auto process = entt::basic_process<int>::allocate<test_process<int>>(std::allocator<void>{});

    ASSERT_FALSE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_FALSE(process->rejected());

    process->succeed();
    process->fail();
    process->abort();
    process->pause();
    process->unpause();

    ASSERT_FALSE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_FALSE(process->rejected());

    process->tick(0);

    ASSERT_TRUE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_FALSE(process->rejected());

    process->pause();

    ASSERT_TRUE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_TRUE(process->paused());
    ASSERT_FALSE(process->rejected());

    process->unpause();

    ASSERT_TRUE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_FALSE(process->rejected());

    process->fail();

    ASSERT_FALSE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_FALSE(process->rejected());

    process->tick(0);

    ASSERT_FALSE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_TRUE(process->rejected());
}

TEST(Process, Succeeded) {
    auto process = entt::basic_process<test::empty>::allocate<test_process<test::empty>>(std::allocator<void>{});

    process->tick({});
    process->tick({});
    process->succeed();
    process->tick({});

    ASSERT_FALSE(process->alive());
    ASSERT_TRUE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_FALSE(process->rejected());

    ASSERT_TRUE(process->update_invoked);
    ASSERT_TRUE(process->succeeded_invoked);
    ASSERT_FALSE(process->failed_invoked);
    ASSERT_FALSE(process->aborted_invoked);
}

TEST(Process, Fail) {
    auto process = entt::basic_process<int>::allocate<test_process<int>>(std::allocator<void>{});

    process->tick(0);
    process->tick(0);
    process->fail();
    process->tick(0);

    ASSERT_FALSE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_TRUE(process->rejected());

    ASSERT_TRUE(process->update_invoked);
    ASSERT_FALSE(process->succeeded_invoked);
    ASSERT_TRUE(process->failed_invoked);
    ASSERT_FALSE(process->aborted_invoked);
}

TEST(Process, Data) {
    auto process = entt::basic_process<test::empty>::allocate<test_process<test::empty>>(std::allocator<void>{});
    int value = 0;

    process->tick({});
    process->tick({}, &value);
    process->succeed();
    process->tick({}, &value);

    ASSERT_FALSE(process->alive());
    ASSERT_TRUE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_FALSE(process->rejected());

    ASSERT_EQ(value, 1);
    ASSERT_TRUE(process->update_invoked);
    ASSERT_TRUE(process->succeeded_invoked);
    ASSERT_FALSE(process->failed_invoked);
    ASSERT_FALSE(process->aborted_invoked);
}

TEST(Process, AbortNextTick) {
    auto process = entt::basic_process<int>::allocate<test_process<int>>(std::allocator<void>{});

    process->tick(0);
    process->abort();
    process->tick(0);

    ASSERT_FALSE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_TRUE(process->rejected());

    ASSERT_TRUE(process->update_invoked);
    ASSERT_FALSE(process->succeeded_invoked);
    ASSERT_FALSE(process->failed_invoked);
    ASSERT_TRUE(process->aborted_invoked);
}

TEST(Process, AbortImmediately) {
    auto process = entt::basic_process<test::empty>::allocate<test_process<test::empty>>(std::allocator<void>{});

    process->tick({});
    process->abort(true);

    ASSERT_FALSE(process->alive());
    ASSERT_FALSE(process->finished());
    ASSERT_FALSE(process->paused());
    ASSERT_TRUE(process->rejected());

    ASSERT_TRUE(process->update_invoked);
    ASSERT_FALSE(process->succeeded_invoked);
    ASSERT_FALSE(process->failed_invoked);
    ASSERT_TRUE(process->aborted_invoked);
}

TEST(ProcessAdaptor, Resolved) {
    bool updated = false;
    auto lambda = [&updated](std::uint32_t, void *, auto resolve, auto) {
        ASSERT_FALSE(updated);
        updated = true;
        resolve();
    };

    auto process = entt::basic_process<std::uint32_t>::allocate<entt::process_adaptor<decltype(lambda)>>(std::allocator<void>{}, lambda);

    process->tick(0u);
    process->tick(0u);

    ASSERT_TRUE(process->finished());
    ASSERT_TRUE(updated);
}

TEST(ProcessAdaptor, Rejected) {
    bool updated = false;
    auto lambda = [&updated](std::uint32_t, void *, auto, auto rejected) {
        ASSERT_FALSE(updated);
        updated = true;
        rejected();
    };

    auto process = entt::basic_process<std::uint32_t>::allocate<entt::process_adaptor<decltype(lambda)>>(std::allocator<void>{}, lambda);

    process->tick(0u);
    process->tick(0u);

    ASSERT_TRUE(process->rejected());
    ASSERT_TRUE(updated);
}

TEST(ProcessAdaptor, Data) {
    int value = 0;
    auto lambda = [](std::uint32_t, void *data, auto resolve, auto) {
        *static_cast<int *>(data) = 2;
        resolve();
    };

    auto process = entt::basic_process<std::uint32_t>::allocate<entt::process_adaptor<decltype(lambda)>>(std::allocator<void>{}, lambda);

    process->tick(0u, &value);

    ASSERT_TRUE(process->finished());
    ASSERT_EQ(value, 2);
}
