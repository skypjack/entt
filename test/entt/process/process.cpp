#include <cstdint>
#include <gtest/gtest.h>
#include <entt/process/process.hpp>
#include "../../common/empty.h"

template<typename Delta>
struct fake_process: entt::process<fake_process<Delta>, Delta> {
    using process_type = entt::process<fake_process<Delta>, Delta>;
    using delta_type = typename process_type::delta_type;

    void succeed() noexcept {
        process_type::succeed();
    }

    void fail() noexcept {
        process_type::fail();
    }

    void pause() noexcept {
        process_type::pause();
    }

    void unpause() noexcept {
        process_type::unpause();
    }

    void init() {
        init_invoked = true;
    }

    void succeeded() {
        succeeded_invoked = true;
    }

    void failed() {
        failed_invoked = true;
    }

    void aborted() {
        aborted_invoked = true;
    }

    void update(typename entt::process<fake_process<Delta>, Delta>::delta_type, void *data) {
        if(data != nullptr) {
            (*static_cast<int *>(data))++;
        }

        update_invoked = true;
    }

    bool init_invoked{};
    bool update_invoked{};
    bool succeeded_invoked{};
    bool failed_invoked{};
    bool aborted_invoked{};
};

TEST(Process, Basics) {
    fake_process<int> process{};

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
    fake_process<test::empty> process{};

    process.tick({});
    process.tick({});
    process.succeed();
    process.tick({});

    ASSERT_FALSE(process.alive());
    ASSERT_TRUE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_FALSE(process.rejected());

    ASSERT_TRUE(process.init_invoked);
    ASSERT_TRUE(process.update_invoked);
    ASSERT_TRUE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, Fail) {
    fake_process<int> process{};

    process.tick(0);
    process.tick(0);
    process.fail();
    process.tick(0);

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_TRUE(process.rejected());

    ASSERT_TRUE(process.init_invoked);
    ASSERT_TRUE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_TRUE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, Data) {
    fake_process<test::empty> process{};
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
    ASSERT_TRUE(process.init_invoked);
    ASSERT_TRUE(process.update_invoked);
    ASSERT_TRUE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_FALSE(process.aborted_invoked);
}

TEST(Process, AbortNextTick) {
    fake_process<int> process{};

    process.tick(0);
    process.abort();
    process.tick(0);

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_TRUE(process.rejected());

    ASSERT_TRUE(process.init_invoked);
    ASSERT_FALSE(process.update_invoked);
    ASSERT_FALSE(process.succeeded_invoked);
    ASSERT_FALSE(process.failed_invoked);
    ASSERT_TRUE(process.aborted_invoked);
}

TEST(Process, AbortImmediately) {
    fake_process<test::empty> process{};

    process.tick({});
    process.abort(true);

    ASSERT_FALSE(process.alive());
    ASSERT_FALSE(process.finished());
    ASSERT_FALSE(process.paused());
    ASSERT_TRUE(process.rejected());

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

    process.tick(0);
    process.tick(0);

    ASSERT_TRUE(process.finished());
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

    process.tick(0);
    process.tick(0);

    ASSERT_TRUE(process.rejected());
    ASSERT_TRUE(updated);
}

TEST(ProcessAdaptor, Data) {
    int value = 0;

    auto lambda = [](std::uint64_t, void *data, auto resolve, auto) {
        *static_cast<int *>(data) = 2;
        resolve();
    };

    auto process = entt::process_adaptor<decltype(lambda), std::uint64_t>{lambda};

    process.tick(0);
    process.tick(0, &value);

    ASSERT_TRUE(process.finished());
    ASSERT_EQ(value, 2);
}
