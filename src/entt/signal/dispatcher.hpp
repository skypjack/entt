#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP


#include <vector>
#include <memory>
#include <utility>
#include <cstdint>
#include "../core/family.hpp"
#include "signal.hpp"
#include "sigh.hpp"


namespace entt {


/**
 * @brief Basic dispatcher implementation.
 *
 * A dispatcher can be used either to trigger an immediate event or to enqueue
 * events to be published all together once per tick.<br/>
 * Listeners are provided in the form of member functions. For each event of
 * type `Event`, listeners must have the following signature:
 * `void(const Event &)`. Member functions named `receive` are automatically
 * detected and registered or unregistered by the dispatcher.
 *
 * @tparam Sig Type of the signal handler to use.
 */
template<template<typename...> class Sig>
class Dispatcher final {
    using event_family = Family<struct InternalDispatcherEventFamily>;

    template<typename Class, typename Event>
    using instance_type = typename Sig<void(const Event &)>::template instance_type<Class>;

    struct BaseSignalWrapper {
        virtual ~BaseSignalWrapper() = default;
        virtual void publish(std::size_t) = 0;
    };

    template<typename Event>
    struct SignalWrapper final: BaseSignalWrapper {
        using sink_type = typename Sig<void(const Event &)>::Sink;

        void publish(std::size_t current) override {
            for(const auto &event: events[current]) {
                signal.publish(event);
            }

            events[current].clear();
        }

        inline sink_type sink() noexcept {
            return signal.sink();
        }

        template<typename... Args>
        inline void trigger(Args &&... args) {
            signal.publish({ std::forward<Args>(args)... });
        }

        template<typename... Args>
        inline void enqueue(std::size_t current, Args &&... args) {
            events[current].push_back({ std::forward<Args>(args)... });
        }

    private:
        Sig<void(const Event &)> signal{};
        std::vector<Event> events[2];
    };

    inline static std::size_t buffer(bool mode) {
        return mode ? 0 : 1;
    }

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

    /*! @brief Default constructor. */
    Dispatcher() noexcept
        : wrappers{}, mode{false}
    {}

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
     * @sa Signal::Sink
     * @sa SigH::Sink
     *
     * @tparam Event Type of event of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Event>
    sink_type<Event> sink() noexcept {
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
    void trigger(Args &&... args) {
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
    void enqueue(Args &&... args) {
        wrapper<Event>().enqueue(buffer(mode), std::forward<Args>(args)...);
    }

    /**
     * @brief Delivers all the pending events.
     *
     * This method is blocking and it doesn't return until all the events are
     * delivered to the registered listeners. It's responsability of the users
     * to reduce at a minimum the time spent in the bodies of the listeners.
     */
    void update() {
        const auto buf = buffer(mode);
        mode = !mode;

        for(auto &&wrapper: wrappers) {
            if(wrapper) {
                wrapper->publish(buf);
            }
        }
    }

private:
    std::vector<std::unique_ptr<BaseSignalWrapper>> wrappers;
    bool mode;
};


/**
 * @brief Managed dispatcher.
 *
 * A managed dispatcher uses the Signal class template as an underlying type.
 * The type of the instances is the one required by the signal handler:
 * `std::shared_ptr<Class>` (a shared pointer).
 */
using ManagedDispatcher = Dispatcher<Signal>;


/**
 * @brief Unmanaged dispatcher.
 *
 * An unmanaged dispatcher uses the SigH class template as an underlying type.
 * The type of the instances is the one required by the signal handler:
 * `Class *` (a naked pointer).<br/>
 * When it comes to work with this kind of dispatcher, users must guarantee that
 * the lifetimes of the instances overcome the one of the dispatcher itself.
 */
using UnmanagedDispatcher = Dispatcher<SigH>;


}


#endif // ENTT_SIGNAL_DISPATCHER_HPP
