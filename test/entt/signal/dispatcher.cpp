#include <memory>
#include <gtest/gtest.h>
#include <entt/signal/dispatcher.hpp>

struct Event {};

struct Receiver {
    void receive(const Event &) { ++cnt; }
    void reset() { cnt = 0; }
    std::size_t cnt{0};
};

template<typename Dispatcher, typename Rec>
void testDispatcher(Rec receiver) {
    Dispatcher dispatcher;

    dispatcher.template connect<Event>(receiver);
    dispatcher.template trigger<Event>();
    dispatcher.template enqueue<Event>();

    ASSERT_EQ(receiver->cnt, static_cast<decltype(receiver->cnt)>(1));

    dispatcher.update();
    dispatcher.update();
    dispatcher.template trigger<Event>();

    ASSERT_EQ(receiver->cnt, static_cast<decltype(receiver->cnt)>(3));

    receiver->reset();

    dispatcher.template disconnect<Event>(receiver);
    dispatcher.template trigger<Event>();
    dispatcher.template enqueue<Event>();
    dispatcher.update();
    dispatcher.template trigger<Event>();

    ASSERT_EQ(receiver->cnt, static_cast<decltype(receiver->cnt)>(0));
}

TEST(ManagedDispatcher, Basics) {
    testDispatcher<entt::ManagedDispatcher>(std::make_shared<Receiver>());
}

TEST(UnmanagedDispatcher, Basics) {
    auto ptr = std::make_unique<Receiver>();
    testDispatcher<entt::UnmanagedDispatcher>(ptr.get());
}
