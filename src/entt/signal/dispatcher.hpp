#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP


#include <vector>
#include <memory>
#include <utility>
#include <cstdint>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/family.hpp"
#include "sigh.hpp"


namespace entt {


/**
 * @brief Basic dispatcher implementation.
 *
 * A dispatcher can be used either to trigger an immediate event or to enqueue
 * events to be published all together once per tick.<br/>
 * Listeners are provided in the form of member functions. For each event of
 * type `Event`, listeners must have the following function type:
 * @code{.cpp}
 * void(const Event &)
 * @endcode
 *
 * Member functions named `receive` are automatically detected and registered or
 * unregistered by the dispatcher. The type of the instances is `Class *` (a
 * naked pointer). It means that users must guarantee that the lifetimes of the
 * instances overcome the one of the dispatcher itself to avoid crashes.
 */
class Dispatcher final {
    using event_family = Family<struct InternalDispatcherEventFamily>;

    template<typename Class, typename Event>
    using instance_type = typename SigH<void(const Event &)>::template instance_type<Class>;

    struct BaseSignalWrapper {
        virtual ~BaseSignalWrapper() = default;
        virtual void publish() = 0;
    };

    template<typename Event>
    struct SignalWrapper final: BaseSignalWrapper {
        using sink_type = typename SigH<void(const Event &)>::sink_type;

        void publish() override {
            const auto &curr = current++;
            current %= std::extent<decltype(events)>::value;
            std::for_each(events[curr].cbegin(), events[curr].cend(), [this](const auto &event) { signal.publish(event); });
            events[curr].clear();
        }

        inline sink_type sink() ENTT_NOEXCEPT {
            return signal.sink();
        }

        template<typename... Args>
        inline void trigger(Args &&... args) {
            signal.publish({ std::forward<Args>(args)... });
        }

        template<typename... Args>
        inline void enqueue(Args &&... args) {
            events[current].push_back({ std::forward<Args>(args)... });
        }

    private:
        SigH<void(const Event &)> signal{};
        std::vector<Event> events[2];
        int current{};
    };

    template<typename Event>
    SignalWrapper<Event> & wrapper() {
        const auto type = event_family::type<Event>();

        if(!(type < wrappers.size())) {
            wrappers.resize(type + 1);
        }

        if(!wrappers[type]) {
            wrappers[type] = std::make_unique<SignalWrapper<Event>>();
        }

        return static_cast<SignalWrapper<Event> &>(*wrappers[type]);
    }

public:
    /*! @brief Type of sink for the given event. */
    template<typename Event>
    using sink_type = typename SignalWrapper<Event>::sink_type;

    /**
     * @brief Returns a sink object for the given event.
     *
     * A sink is an opaque object used to connect listeners to events.
     *
     * The function type for a listener is:
     * @code{.cpp}
     * void(const Event &)
     * @endcode
     *
     * The order of invocation of the listeners isn't guaranteed.
     *
     * @sa SigH::Sink
     *
     * @tparam Event Type of event of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Event>
    inline sink_type<Event> sink() ENTT_NOEXCEPT {
        return wrapper<Event>().sink();
    }

    /**
     * @brief Triggers an immediate event of the given type.
     *
     * All the listeners registered for the given type are immediately notified.
     * The event is discarded after the execution.
     *
     * @tparam Event Type of event to trigger.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename Event, typename... Args>
    inline void trigger(Args &&... args) {
        wrapper<Event>().trigger(std::forward<Args>(args)...);
    }

    /**
     * @brief Enqueues an event of the given type.
     *
     * An event of the given type is queued. No listener is invoked. Use the
     * `update` member function to notify listeners when ready.
     *
     * @tparam Event Type of event to trigger.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename Event, typename... Args>
    inline void enqueue(Args &&... args) {
        wrapper<Event>().enqueue(std::forward<Args>(args)...);
    }

    /**
     * @brief Delivers all the pending events of the given type.
     *
     * This method is blocking and it doesn't return until all the events are
     * delivered to the registered listeners. It's responsibility of the users
     * to reduce at a minimum the time spent in the bodies of the listeners.
     *
     * @tparam Event Type of events to send.
     */
    template<typename Event>
    inline void update() {
        wrapper<Event>().publish();
    }

    /**
     * @brief Delivers all the pending events.
     *
     * This method is blocking and it doesn't return until all the events are
     * delivered to the registered listeners. It's responsibility of the users
     * to reduce at a minimum the time spent in the bodies of the listeners.
     */
    inline void update() const {
        for(auto pos = wrappers.size(); pos; --pos) {
            auto &wrapper = wrappers[pos-1];

            if(wrapper) {
                wrapper->publish();
            }
        }
    }

private:
    std::vector<std::unique_ptr<BaseSignalWrapper>> wrappers;
};


}


#endif // ENTT_SIGNAL_DISPATCHER_HPP
