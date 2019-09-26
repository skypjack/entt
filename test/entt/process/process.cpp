#include <gtest/gtest.h>
#include <cstdint>
#include <entt/process/process.hpp>

struct time_info {
    uint64_t timestamp;
    uint64_t prev_timestamp;
    double timescale;
    double dt() { 
        return (timestamp - prev_timestamp) * timescale;
    }
};

using delta_type = time_info;

struct fake_process: entt::process<fake_process, delta_type> {
    using process_type = entt::process<fake_process, delta_type>;

    void succeed() noexcept { process_type::succeed(); }
    void fail() noexcept { process_type::fail(); }
    void pause() noexcept { process_type::pause(); }
    void unpause() noexcept { process_type::unpause(); }

    void init() { init_invoked = true; }
    void succeeded() { succeeded_invoked = true; }
    void failed() { failed_invoked = true; }
    void aborted() { aborted_invoked = true; }

    void update(delta_type, void *data) {
        if(data) {
            (*static_cast<int *>(data))++;
        }

        update_invoked = true;
    }

    bool init_invoked{false};
    bool update_invoked{false};
    bool succeeded_invoked{false};
    bool failed_invoked{false};
    bool aborted_invoked{false};
};

TEST(Process, Basics) {
    fake_process process;

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.dead());
    ASSERT_FALSE(process.paused());

    process.succeed();
    process.fail();
    process.abort();
    process.pause();
    process.unpause();

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.dead());
    ASSERT_FALSE(process.paused());

    process.tick({});

    ASSERT_TRUE(process.alive());
    ASSERT_FALSE(process.dead());
    ASSERT_FALSE(process.paused());

    process.pause();

    ASSERT_TRUE(process.alive());
    ASSERT_FALSE(process.dead());
    ASSERT_TRUE(process.paused());

    process.unpause();

    ASSERT_TRUE(process.alive());
    ASSERT_FALSE(process.dead());
    ASSERT_FALSE(process.paused());
}

TEST(Process, Succeeded) {
    fake_process process;

    process.tick({});
    process.tick({});
    process.succeed();
    process.tick({});

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.dead());
    ASSERT_FALSE(process.paused());

    ASSERT_TRUE(process.init_invoked);
    ASSERT_TRUE(process.update_invoked);
    ASSERT_TRUE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, Fail) {
    fake_process process;

    process.tick({});
    process.tick({});
    process.fail();
    process.tick({});

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.dead());
    ASSERT_FALSE(process.paused());

    ASSERT_TRUE(process.init_invoked);
    ASSERT_TRUE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_TRUE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, Data) {
    fake_process process;
    int value = 0;

    process.tick({});
    process.tick({}, &value);
    process.succeed();
    process.tick({}, &value);

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.dead());
    ASSERT_FALSE(process.paused());

    ASSERT_EQ(value, 1);
    ASSERT_TRUE(process.init_invoked);
    ASSERT_TRUE(process.update_invoked);
    ASSERT_TRUE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, AbortNextTick) {
    fake_process process;

    process.tick({});
    process.abort();
    process.tick({});

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.dead());
    ASSERT_FALSE(process.paused());

    ASSERT_TRUE(process.init_invoked);
    ASSERT_FALSE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_TRUE(process.aborted_invoked);
}

TEST(Process, AbortImmediately) {
    fake_process process;

    process.tick({});
    process.abort(true);

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.dead());
    ASSERT_FALSE(process.paused());

    ASSERT_TRUE(process.init_invoked);
    ASSERT_FALSE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_TRUE(process.aborted_invoked);
}

TEST(ProcessAdaptor, Resolved) {
    bool updated = false;
    auto lambda = [&updated](std::uint64_t, void *, auto resolve, auto) {
        ASSERT_FALSE(updated);
        updated = true;
        resolve();
    };

    auto process = entt::process_adaptor<decltype(lambda), std::uint64_t>{lambda};

    process.tick({});
    process.tick({});

    ASSERT_TRUE(process.dead());
    ASSERT_TRUE(updated);
}

TEST(ProcessAdaptor, Rejected) {
    bool updated = false;
    auto lambda = [&updated](std::uint64_t, void *, auto, auto rejected) {
        ASSERT_FALSE(updated);
        updated = true;
        rejected();
    };

    auto process = entt::process_adaptor<decltype(lambda), std::uint64_t>{lambda};

    process.tick({});
    process.tick({});

    ASSERT_TRUE(process.rejected());
    ASSERT_TRUE(updated);
}

TEST(ProcessAdaptor, Data) {
    int value = 0;

    auto lambda = [](std::uint64_t, void *data, auto resolve, auto) {
        *static_cast<int *>(data) = 42;
        resolve();
    };

    auto process = entt::process_adaptor<decltype(lambda), std::uint64_t>{lambda};

    process.tick({});
    process.tick({}, &value);

    ASSERT_TRUE(process.dead());
    ASSERT_EQ(value, 42);
}
