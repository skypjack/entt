#include <memory>
#include <utility>
#include <gtest/gtest.h>
#include <entt/signal/sigh.hpp>

struct sigh_listener {
    static void f(int &v) {
        v = 42;
    }

    bool g(int) {
        k = !k;
        return true;
    }

    bool h(const int &) {
        return k;
    }

    // useless definition just because msvc does weird things if both are empty
    void l() {
        k = true && k;
    }

    bool k{false};
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

TEST(SigH, Lifetime) {
    using signal = entt::sigh<void(void)>;

    ASSERT_NO_FATAL_FAILURE(signal{});

    signal src{}, other{};

    ASSERT_NO_FATAL_FAILURE(signal{src});
    ASSERT_NO_FATAL_FAILURE(signal{std::move(other)});
    ASSERT_NO_FATAL_FAILURE(src = other);
    ASSERT_NO_FATAL_FAILURE(src = std::move(other));

    ASSERT_NO_FATAL_FAILURE(delete new signal{});
}

TEST(SigH, Clear) {
    entt::sigh<void(int &)> sigh;
    entt::sink sink{sigh};

    sink.connect<&sigh_listener::f>();

    ASSERT_FALSE(sink.empty());
    ASSERT_FALSE(sigh.empty());

    sink.disconnect(static_cast<const void *>(nullptr));

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
    entt::sink sink2{sigh2};

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
    int v = 0;

    sink.connect<&sigh_listener::f>();
    sigh.publish(v);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(1u, sigh.size());
    ASSERT_EQ(42, v);

    v = 0;
    sink.disconnect<&sigh_listener::f>();
    sigh.publish(v);

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(0u, sigh.size());
    ASSERT_EQ(v, 0);
}

TEST(SigH, FunctionsWithPayload) {
    entt::sigh<void()> sigh;
    entt::sink sink{sigh};
    int v = 0;

    sink.connect<&sigh_listener::f>(v);
    sigh.publish();

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(1u, sigh.size());
    ASSERT_EQ(42, v);

    v = 0;
    sink.disconnect<&sigh_listener::f>(v);
    sigh.publish();

    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(0u, sigh.size());
    ASSERT_EQ(v, 0);

    sink.connect<&sigh_listener::f>(v);
    sink.disconnect(&v);
    sigh.publish();

    ASSERT_EQ(v, 0);
}

TEST(SigH, Members) {
    sigh_listener l1, l2;
    entt::sigh<bool(int)> sigh;
    entt::sink sink{sigh};

    sink.connect<&sigh_listener::g>(l1);
    sigh.publish(42);

    ASSERT_TRUE(l1.k);
    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(1u, sigh.size());

    sink.disconnect<&sigh_listener::g>(l1);
    sigh.publish(42);

    ASSERT_TRUE(l1.k);
    ASSERT_TRUE(sigh.empty());
    ASSERT_EQ(0u, sigh.size());

    sink.connect<&sigh_listener::g>(&l1);
    sink.connect<&sigh_listener::h>(l2);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(2u, sigh.size());

    sink.disconnect(static_cast<const void *>(nullptr));

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(2u, sigh.size());

    sink.disconnect(&l1);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(1u, sigh.size());
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
        listener.k = true;
        ++cnt;
    };

    listener.k = true;
    sigh.collect(std::move(no_return), 42);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(cnt, 2);

    auto bool_return = [&cnt](bool value) {
        // gtest and its macro hell are sometimes really annoying...
        [](auto v) { ASSERT_TRUE(v); }(value);
        ++cnt;
        return true;
    };

    cnt = 0;
    sigh.collect(std::move(bool_return), 42);

    ASSERT_EQ(cnt, 1);
}

TEST(SigH, CollectorVoid) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    entt::sink sink{sigh};
    int cnt = 0;

    sink.connect<&sigh_listener::g>(&listener);
    sink.connect<&sigh_listener::h>(listener);
    sigh.collect([&cnt]() { ++cnt; }, 42);

    ASSERT_FALSE(sigh.empty());
    ASSERT_EQ(cnt, 2);

    auto test = [&cnt]() {
        ++cnt;
        return true;
    };

    cnt = 0;
    sigh.collect(std::move(test), 42);

    ASSERT_EQ(cnt, 1);
}

TEST(SigH, Connection) {
    entt::sigh<void(int &)> sigh;
    entt::sink sink{sigh};
    int v = 0;

    auto conn = sink.connect<&sigh_listener::f>();
    sigh.publish(v);

    ASSERT_FALSE(sigh.empty());
    ASSERT_TRUE(conn);
    ASSERT_EQ(42, v);

    v = 0;
    conn.release();
    sigh.publish(v);

    ASSERT_TRUE(sigh.empty());
    ASSERT_FALSE(conn);
    ASSERT_EQ(0, v);
}

TEST(SigH, ScopedConnection) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    entt::sink sink{sigh};

    {
        ASSERT_FALSE(listener.k);

        entt::scoped_connection conn = sink.connect<&sigh_listener::g>(listener);
        sigh.publish(42);

        ASSERT_FALSE(sigh.empty());
        ASSERT_TRUE(listener.k);
        ASSERT_TRUE(conn);
    }

    sigh.publish(42);

    ASSERT_TRUE(sigh.empty());
    ASSERT_TRUE(listener.k);
}

TEST(SigH, ScopedConnectionMove) {
    sigh_listener listener;
    entt::sigh<void(int)> sigh;
    entt::sink sink{sigh};

    entt::scoped_connection outer{sink.connect<&sigh_listener::g>(listener)};

    ASSERT_FALSE(sigh.empty());
    ASSERT_TRUE(outer);

    {
        entt::scoped_connection inner{std::move(outer)};

        ASSERT_FALSE(listener.k);
        ASSERT_FALSE(outer);
        ASSERT_TRUE(inner);

        sigh.publish(42);

        ASSERT_TRUE(listener.k);
    }

    ASSERT_TRUE(sigh.empty());

    outer = sink.connect<&sigh_listener::g>(listener);

    ASSERT_FALSE(sigh.empty());
    ASSERT_TRUE(outer);

    {
        entt::scoped_connection inner{};

        ASSERT_TRUE(listener.k);
        ASSERT_TRUE(outer);
        ASSERT_FALSE(inner);

        inner = std::move(outer);

        ASSERT_FALSE(outer);
        ASSERT_TRUE(inner);

        sigh.publish(42);

        ASSERT_FALSE(listener.k);
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
        ASSERT_FALSE(listener.k);
        ASSERT_FALSE(inner);

        inner = sink.connect<&sigh_listener::g>(listener);
        sigh.publish(42);

        ASSERT_FALSE(sigh.empty());
        ASSERT_TRUE(listener.k);
        ASSERT_TRUE(inner);

        inner.release();

        ASSERT_TRUE(sigh.empty());
        ASSERT_FALSE(inner);

        auto basic = sink.connect<&sigh_listener::g>(listener);
        inner = std::as_const(basic);
        sigh.publish(42);

        ASSERT_FALSE(sigh.empty());
        ASSERT_FALSE(listener.k);
        ASSERT_TRUE(inner);
    }

    sigh.publish(42);

    ASSERT_TRUE(sigh.empty());
    ASSERT_FALSE(listener.k);
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

    ASSERT_FALSE(listener.k);

    sink.connect<&sigh_listener::k>();
    sigh.collect([](bool &value) { value = !value; }, listener);

    ASSERT_TRUE(listener.k);
}

TEST(SigH, UnboundMemberFunction) {
    sigh_listener listener;
    entt::sigh<void(sigh_listener *, int)> sigh;
    entt::sink sink{sigh};

    ASSERT_FALSE(listener.k);

    sink.connect<&sigh_listener::g>();
    sigh.publish(&listener, 42);

    ASSERT_TRUE(listener.k);
}

TEST(SigH, ConnectAndAutoDisconnect) {
    sigh_listener listener;
    entt::sigh<void(int &)> sigh;
    entt::sink sink{sigh};
    int v = 0;

    sink.connect<&sigh_listener::g>(listener);
    sink.connect<&connect_and_auto_disconnect>(sigh);

    ASSERT_FALSE(listener.k);
    ASSERT_EQ(sigh.size(), 2u);
    ASSERT_EQ(v, 0);

    sigh.publish(v);

    ASSERT_TRUE(listener.k);
    ASSERT_EQ(sigh.size(), 2u);
    ASSERT_EQ(v, 0);

    sigh.publish(v);

    ASSERT_FALSE(listener.k);
    ASSERT_EQ(sigh.size(), 2u);
    ASSERT_EQ(v, 42);
}

TEST(SigH, CustomAllocator) {
    std::allocator<void (*)(int)> allocator;
    entt::sigh<void(int), decltype(allocator)> sigh{allocator};

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
