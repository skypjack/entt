#include <memory>
#include <gtest/gtest.h>
#include <entt/signal/bus.hpp>

struct EventA
{
    EventA(int x, int y): value{x+y} {}
    int value;
};

struct EventB {};
struct EventC {};

struct MyListener
{
    void receive(const EventA &) { A++; }
    static void listen(const EventB &) { B++; }
    void receive(const EventC &) { C++; }
    void reset() { A = 0; B = 0; C = 0; }
    int A{0};
    static int B;
    int C{0};
};

int MyListener::B = 0;

template<typename Bus, typename Listener>
void testRegUnregEmit(Listener listener) {
    Bus bus;

    listener->reset();
    bus.template publish<EventA>(40, 2);
    bus.template publish<EventB>();
    bus.template publish<EventC>();

    ASSERT_EQ(bus.size(), (decltype(bus.size()))0);
    ASSERT_TRUE(bus.empty());
    ASSERT_EQ(listener->A, 0);
    ASSERT_EQ(listener->B, 0);
    ASSERT_EQ(listener->C, 0);

    bus.reg(listener);
    bus.template connect<EventB, &MyListener::listen>();

    listener->reset();
    bus.template publish<EventA>(40, 2);
    bus.template publish<EventB>();
    bus.template publish<EventC>();

    ASSERT_EQ(bus.size(), (decltype(bus.size()))3);
    ASSERT_FALSE(bus.empty());
    ASSERT_EQ(listener->A, 1);
    ASSERT_EQ(listener->B, 1);
    ASSERT_EQ(listener->C, 1);

    bus.unreg(listener);

    listener->reset();
    bus.template publish<EventA>(40, 2);
    bus.template publish<EventB>();
    bus.template publish<EventC>();

    ASSERT_EQ(bus.size(), (decltype(bus.size()))1);
    ASSERT_FALSE(bus.empty());
    ASSERT_EQ(listener->A, 0);
    ASSERT_EQ(listener->B, 1);
    ASSERT_EQ(listener->C, 0);

    bus.template disconnect<EventB, MyListener::listen>();

    listener->reset();
    bus.template publish<EventA>(40, 2);
    bus.template publish<EventB>();
    bus.template publish<EventC>();

    ASSERT_EQ(bus.size(), (decltype(bus.size()))0);
    ASSERT_TRUE(bus.empty());
    ASSERT_EQ(listener->A, 0);
    ASSERT_EQ(listener->B, 0);
    ASSERT_EQ(listener->C, 0);
}

TEST(ManagedBus, RegUnregEmit) {
    using MyManagedBus = entt::ManagedBus<EventA, EventB, EventC>;
    testRegUnregEmit<MyManagedBus>(std::make_shared<MyListener>());
}

TEST(ManagedBus, ExpiredListeners) {
    entt::ManagedBus<EventA, EventB, EventC> bus;
    auto listener = std::make_shared<MyListener>();

    listener->reset();
    bus.reg(listener);
    bus.template publish<EventA>(40, 2);
    bus.template publish<EventB>();

    ASSERT_EQ(bus.size(), (decltype(bus.size()))2);
    ASSERT_FALSE(bus.empty());
    ASSERT_EQ(listener->A, 1);
    ASSERT_EQ(listener->B, 0);

    listener->reset();
    listener = nullptr;

    ASSERT_EQ(bus.size(), (decltype(bus.size()))2);
    ASSERT_FALSE(bus.empty());

    EXPECT_NO_THROW(bus.template publish<EventA>(40, 2));
    EXPECT_NO_THROW(bus.template publish<EventC>());

    ASSERT_EQ(bus.size(), (decltype(bus.size()))0);
    ASSERT_TRUE(bus.empty());
}

TEST(UnmanagedBus, RegUnregEmit) {
    using MyUnmanagedBus = entt::UnmanagedBus<EventA, EventB, EventC>;
    auto ptr = std::make_unique<MyListener>();
    testRegUnregEmit<MyUnmanagedBus>(ptr.get());
}

TEST(UnmanagedBus, ExpiredListeners) {
    entt::UnmanagedBus<EventA, EventB, EventC> bus;
    auto listener = std::make_unique<MyListener>();

    listener->reset();
    bus.reg(listener.get());
    bus.template publish<EventA>(40, 2);
    bus.template publish<EventB>();

    ASSERT_EQ(bus.size(), (decltype(bus.size()))2);
    ASSERT_FALSE(bus.empty());
    ASSERT_EQ(listener->A, 1);
    ASSERT_EQ(listener->B, 0);

    listener->reset();
    listener = nullptr;

    // dangling pointer inside ... well, unmanaged means unmanaged!! :-)
    ASSERT_EQ(bus.size(), (decltype(bus.size()))2);
    ASSERT_FALSE(bus.empty());
}
