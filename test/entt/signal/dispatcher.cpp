#include <memory>
#include <gtest/gtest.h>
#include <entt/entity/entt_traits.hpp>
#include <entt/signal/dispatcher.hpp>

struct an_event {};
struct another_event {};
struct one_more_event {};

ENTT_SHARED_TYPE(an_event)

struct receiver {
    void receive(const an_event &, int value) { cnt += value; }
    void reset() { cnt = 0; }
    int cnt{0};
};

TEST(Dispatcher, Functionalities) {
    entt::dispatcher<int> dispatcher;
    receiver receiver;

    dispatcher.trigger<one_more_event>(1);
    dispatcher.enqueue<one_more_event>();
    dispatcher.update<one_more_event>(1);

    dispatcher.sink<an_event>().connect<&receiver::receive>(&receiver);
    dispatcher.trigger<an_event>(1);
    dispatcher.enqueue<an_event>();

    dispatcher.enqueue<another_event>();
    dispatcher.update<another_event>(1);

    ASSERT_EQ(receiver.cnt, 1);

    dispatcher.update<an_event>(2);
    dispatcher.trigger<an_event>(1);

    ASSERT_EQ(receiver.cnt, 4);

    receiver.reset();

    an_event event{};
    const an_event &cevent = event;

    dispatcher.sink<an_event>().disconnect<&receiver::receive>(&receiver);
    dispatcher.trigger(an_event{}, 1);
    dispatcher.enqueue(event);
    dispatcher.update(1);
    dispatcher.trigger(cevent, 1);

    ASSERT_EQ(receiver.cnt, 0);
}
