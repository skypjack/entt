#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/signal/sigh.hpp>
#include "../../common/config.h"
#include "../../common/linter.hpp"

struct sigh_listener {
    static void f(int &iv) {
        ++iv;
    }

    [[nodiscard]] bool g(int) {
        val = !val;
        return true;
    }

    [[nodiscard]] bool h(const int &) const {
        return val;
    }

    // useless definition just because msvc does weird things if both are empty
    void i() {
        val = val && val;
    }

    bool val{false};
};

struct const_nonconst_noexcept {
    void f() {
        ++cnt;
    }

    void g() noexcept {
        ++cnt;
    }

    void h() const {
        ++cnt;
    }

    void i() const noexcept {
        ++cnt;
    }

    mutable int cnt{0};
};

void connect_and_auto_disconnect(entt::sigh<void(int &)> &sigh, const int &) {
    entt::sink sink{sigh};
    sink.connect<sigh_listener::f>();
    sink.disconnect<&connect_and_auto_disconnect>(sigh);
}

ENTT_DEBUG_TEST(SinkDeathTest, Invalid) {
    sigh_listener listener;
    entt::sigh<void(int &)> sigh;
    entt::sink<entt::sigh<void(int &)>> sink{};

    ASSERT_FALSE(sink);

    ASSERT_DEATH([[maybe_unused]] const bool empty = sink.empty(), "");
    ASSERT_DEATH(sink.connect<&sigh_listener::f>(), "");
    ASSERT_DEATH(sink.disconnect<&sigh_listener::f>(), "");
    ASSERT_DEATH(sink.disconnect(&listener), "");
    ASSERT_DEATH(sink.disconnect(), "");

    sink = entt::sink{sigh};

    ASSERT_TRUE(sink);
    ASSERT_TRUE(sink.empty());
}

TEST(SigH, Lifetime) {
    using signal = entt::sigh<void(void)>;

    ASSERT_NO_THROW(signal{});

    signal src{};
    signal other{};

    ASSERT_NO_THROW(signal{src});
    ASSERT_NO_THROW(signal{std::move(other)});

    other = {};

    ASSERT_NO_THROW(src = other);
    ASSERT_NO_THROW(src = std::move(other));

    ASSERT_NO_THROW(delete new signal{});
}

TEST(SigH, Disconnect) {
    sigh_listener listener;
    entt::sigh<void(int &)> sigh;
    entt::sink sink{sigh};

    sink.connect<&sigh_listener::f>();

    ASSERT_FALSE(sink.empty());
    ASSERT_FALSE(sigh.empty());

    sink.disconnect<&sigh_listener::f>();

    ASSERT_TRUE(sink.empty());
    ASSERT_TRUE(sigh.empty());

    sink.connect<&sigh_listener::g>(listener);

    ASSERT_FALSE(sink.empty());
    ASSERT_FALSE(sigh.empty());

    sink.disconnect<&sigh_listener::g>(listener);

    ASSERT_TRUE(sink.empty());
    ASSERT_TRUE(sigh.empty());

    sink.connect<&sigh_listener::g>(listener);

    ASSERT_FALSE(sink.empty());
    ASSERT_FALSE(sigh.empty());

    sink.disconnect(&listener);

    ASSERT_TRUE(sink.empty());
    ASSERT_TRUE(sigh.empty());

    sink.connect<&sigh_listener::f>();

    ASSERT_FALSE(sink.empty());
    ASSERT_FALSE(sigh.empty());

    sink.disconnect();

    ASSERT_TRUE(sink.empty());
    ASSERT_TRUE(sigh.empty());
}

TEST(SigH, Swap) {
    entt::sigh<void(int &)> sigh1;
    entt::sigh<void(int &)> sigh2;
    entt::sink sink1{sigh1};
    const entt::sink sink2{sigh2};

    sink1.connect<&sigh_listener::f>();

    ASSERT_FALSE(sink1.empty());
    ASSERT_TRUE(sink2.empty());

    ASSERT_FALSE(sigh1.empty());
    ASSERT_TRUE(sigh2.empty());

    sigh1.swap(sigh2);

    ASSERT_TRUE(sink1.empty());
    ASSERT_FALSE(sink2.empty());

    ASSERT_TRUE(sigh1.empty());
    ASSERT_FALSE(sigh2.empty());
}

TEST(SigH, Functions) {
    entt::sigh<void(int &)> sigh;
    entt::sink sink{sigh};
    int value = 0;

    sink.connect<&sigh_listener::f>();
    sigh.publish(value);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(sigh.size(), 1u);
    ASSERT_EQ(value, 1);

    value = 0;
    sink.disconnect<&sigh_listener::f>();
    sigh.publish(value);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(sigh.size(), 0u);
    ASSERT_EQ(value, 0);
}

TEST(SigH, FunctionsWithPayload) {
    entt::sigh<void()> sigh;
    entt::sink sink{sigh};
    int value = 0;

    sink.connect<&sigh_listener::f>(value);
    sigh.publish();

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(sigh.size(), 1u);
    ASSERT_EQ(value, 1);

    value = 0;
    sink.disconnect<&sigh_listener::f>(value);
    sigh.publish();

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(sigh.size(), 0u);
    ASSERT_EQ(value, 0);

    sink.connect<&sigh_listener::f>(value);
    sink.disconnect(&value);
    sigh.publish();

    ASSERT_EQ(value, 0);
}

TEST(SigH, Members) {
    sigh_listener l1;
    sigh_listener l2;
    entt::sigh<bool(int)> sigh;
    entt::sink sink{sigh};

    sink.connect<&sigh_listener::g>(l1);
    sigh.publish(3);

    ASSERT_TRUE(l1.val);
    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(sigh.size(), 1u);

    sink.disconnect<&sigh_listener::g>(l1);
    sigh.publish(3);

    ASSERT_TRUE(l1.val);
    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(sigh.size(), 0u);

    sink.connect<&sigh_listener::g>(&l1);
    sink.connect<&sigh_listener::h>(l2);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(sigh.size(), 2u);

    sink.disconnect(&l1);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(sigh.size(), 1u);
}

TEST(SigH, Collector) {
    sigh_listener listener;
    entt::sigh<bool(int)> sigh;
    entt::sink sink{sigh};
    int cnt = 0;

    sink.connect<&sigh_listener::g>(&listener);
    sink.connect<&sigh_listener::h>(listener);

    auto no_return = [&listener, &cnt](bool value) {
        ASSERT_TRUE(value);
        listener.val = true;
        ++cnt;
    };

    listener.val = true;
    sigh.collect(std::move(no_return), 3);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(cnt, 2);

    auto bool_return = [&cnt](bool value) {
        // gtest and its macro hell are sometimes really annoying...
        [](auto curr) { ASSERT_TRUE(curr); }(value);
        ++cnt;
        return true;
    };

    cnt = 0;
    sigh.collect(std::move(bool_return), 3);

    ASSERT_EQ(cnt, 1);
}

TEST(SigH, CollectorVoid) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    entt::sink sink{sigh};
    int cnt = 0;

    sink.connect<&sigh_listener::g>(&listener);
    sink.connect<&sigh_listener::h>(listener);
    sigh.collect([&cnt]() { ++cnt; }, 3);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(cnt, 2);

    cnt = 0;
    sigh.collect([&cnt]() { ++cnt; return true; }, 3);

    ASSERT_EQ(cnt, 1);
}

TEST(SigH, Connection) {
    entt::sigh<void(int &)> sigh;
    entt::sink sink{sigh};
    int value = 0;

    auto conn = sink.connect<&sigh_listener::f>();
    sigh.publish(value);

    ASSERT_FALSE(sigh.empty());
    ASSERT_TRUE(conn);
    ASSERT_EQ(value, 1);

    value = 0;
    conn.release();
    sigh.publish(value);

    ASSERT_TRUE(sigh.empty());
    ASSERT_FALSE(conn);
    ASSERT_EQ(0, value);
}

TEST(SigH, ScopedConnection) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    entt::sink sink{sigh};

    {
        ASSERT_FALSE(listener.val);

        const entt::scoped_connection conn = sink.connect<&sigh_listener::g>(listener);
        sigh.publish(1);

        ASSERT_FALSE(sigh.empty());
        ASSERT_TRUE(listener.val);
        ASSERT_TRUE(conn);
    }

    sigh.publish(1);

    ASSERT_TRUE(sigh.empty());
    ASSERT_TRUE(listener.val);
}

TEST(SigH, ScopedConnectionMove) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    entt::sink sink{sigh};

    entt::scoped_connection outer{sink.connect<&sigh_listener::g>(listener)};

    ASSERT_FALSE(sigh.empty());
    ASSERT_TRUE(outer);

    {
        const entt::scoped_connection inner{std::move(outer)};

        test::is_initialized(outer);

        ASSERT_FALSE(listener.val);
        ASSERT_FALSE(outer);
        ASSERT_TRUE(inner);

        sigh.publish(1);

        ASSERT_TRUE(listener.val);
    }

    ASSERT_TRUE(sigh.empty());

    outer = sink.connect<&sigh_listener::g>(listener);

    ASSERT_FALSE(sigh.empty());
    ASSERT_TRUE(outer);

    {
        entt::scoped_connection inner{};

        ASSERT_TRUE(listener.val);
        ASSERT_TRUE(outer);
        ASSERT_FALSE(inner);

        inner = std::move(outer);
        test::is_initialized(outer);

        ASSERT_FALSE(outer);
        ASSERT_TRUE(inner);

        sigh.publish(1);

        ASSERT_FALSE(listener.val);
    }

    ASSERT_TRUE(sigh.empty());
}

TEST(SigH, ScopedConnectionConstructorsAndOperators) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    entt::sink sink{sigh};

    {
        entt::scoped_connection inner{};

        ASSERT_TRUE(sigh.empty());
        ASSERT_FALSE(listener.val);
        ASSERT_FALSE(inner);

        inner = sink.connect<&sigh_listener::g>(listener);
        sigh.publish(1);

        ASSERT_FALSE(sigh.empty());
        ASSERT_TRUE(listener.val);
        ASSERT_TRUE(inner);

        inner.release();

        ASSERT_TRUE(sigh.empty());
        ASSERT_FALSE(inner);

        auto basic = sink.connect<&sigh_listener::g>(listener);
        inner = std::as_const(basic);
        sigh.publish(1);

        ASSERT_FALSE(sigh.empty());
        ASSERT_FALSE(listener.val);
        ASSERT_TRUE(inner);
    }

    sigh.publish(1);

    ASSERT_TRUE(sigh.empty());
    ASSERT_FALSE(listener.val);
}

TEST(SigH, ConstNonConstNoExcept) {
    entt::sigh<void()> sigh;
    entt::sink sink{sigh};
    const_nonconst_noexcept functor;
    const const_nonconst_noexcept cfunctor;

    sink.connect<&const_nonconst_noexcept::f>(functor);
    sink.connect<&const_nonconst_noexcept::g>(&functor);
    sink.connect<&const_nonconst_noexcept::h>(cfunctor);
    sink.connect<&const_nonconst_noexcept::i>(&cfunctor);
    sigh.publish();

    ASSERT_EQ(functor.cnt, 2);
    ASSERT_EQ(cfunctor.cnt, 2);

    sink.disconnect<&const_nonconst_noexcept::f>(functor);
    sink.disconnect<&const_nonconst_noexcept::g>(&functor);
    sink.disconnect<&const_nonconst_noexcept::h>(cfunctor);
    sink.disconnect<&const_nonconst_noexcept::i>(&cfunctor);
    sigh.publish();

    ASSERT_EQ(functor.cnt, 2);
    ASSERT_EQ(cfunctor.cnt, 2);
}

TEST(SigH, UnboundDataMember) {
    sigh_listener listener;
    entt::sigh<bool &(sigh_listener &)> sigh;
    entt::sink sink{sigh};

    ASSERT_FALSE(listener.val);

    sink.connect<&sigh_listener::val>();
    sigh.collect([](bool &value) { value = !value; }, listener);

    ASSERT_TRUE(listener.val);
}

TEST(SigH, UnboundMemberFunction) {
    sigh_listener listener;
    entt::sigh<void(sigh_listener *, int)> sigh;
    entt::sink sink{sigh};

    ASSERT_FALSE(listener.val);

    sink.connect<&sigh_listener::g>();
    sigh.publish(&listener, 1);

    ASSERT_TRUE(listener.val);
}

TEST(SigH, ConnectAndAutoDisconnect) {
    sigh_listener listener;
    entt::sigh<void(int &)> sigh;
    entt::sink sink{sigh};
    int value = 0;

    sink.connect<&sigh_listener::g>(listener);
    sink.connect<&connect_and_auto_disconnect>(sigh);

    ASSERT_FALSE(listener.val);
    ASSERT_EQ(sigh.size(), 2u);
    ASSERT_EQ(value, 0);

    sigh.publish(value);

    ASSERT_TRUE(listener.val);
    ASSERT_EQ(sigh.size(), 2u);
    ASSERT_EQ(value, 0);

    sigh.publish(value);

    ASSERT_FALSE(listener.val);
    ASSERT_EQ(sigh.size(), 2u);
    ASSERT_EQ(value, 1);
}

TEST(SigH, CustomAllocator) {
    const std::allocator<void (*)(int)> allocator;
    entt::sigh<void(int), std::allocator<void (*)(int)>> sigh{allocator};

    ASSERT_EQ(sigh.get_allocator(), allocator);
    ASSERT_FALSE(sigh.get_allocator() != allocator);
    ASSERT_TRUE(sigh.empty());

    entt::sink sink{sigh};
    sigh_listener listener;
    sink.template connect<&sigh_listener::g>(listener);

    decltype(sigh) copy{sigh, allocator};
    sink.disconnect(&listener);

    ASSERT_TRUE(sigh.empty());
    ASSERT_FALSE(copy.empty());

    sigh = copy;

    ASSERT_FALSE(sigh.empty());
    ASSERT_FALSE(copy.empty());

    decltype(sigh) move{std::move(copy), allocator};

    test::is_initialized(copy);

    ASSERT_TRUE(copy.empty());
    ASSERT_FALSE(move.empty());

    sink = entt::sink{move};
    sink.disconnect(&listener);

    ASSERT_TRUE(copy.empty());
    ASSERT_TRUE(move.empty());

    sink.template connect<&sigh_listener::g>(listener);
    copy.swap(move);

    ASSERT_FALSE(copy.empty());
    ASSERT_TRUE(move.empty());

    sink = entt::sink{copy};
    sink.disconnect();

    ASSERT_TRUE(copy.empty());
    ASSERT_TRUE(move.empty());
}
