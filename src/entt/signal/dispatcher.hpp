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
        void publish(std::size_t current) override {
            for(const auto &event: events[current]) {
                signal.publish(event);
            }

            events[current].clear();
        }

        template<typename Class, void(Class::*Member)(const Event &)>
        inline void connect(instance_type<Class, Event> instance) noexcept {
            signal.template connect<Class, Member>(std::move(instance));
        }

        template<typename Class, void(Class::*Member)(const Event &)>
        inline void disconnect(instance_type<Class, Event> instance) noexcept {
            signal.template disconnect<Class, Member>(std::move(instance));
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
    /*! @brief Default constructor. */
    Dispatcher() noexcept
        : wrappers{}, mode{false}
    {}

    /**
     * @brief Registers a listener given in the form of a member function.
     *
     * A matching member function has the following signature:
     * `void receive(const Event &)`. Member functions named `receive` are
     * automatically detected and registered if available.
     *
     * @warning
     * Connecting a listener during an update may lead to unexpected behavior.
     * Register listeners before or after invoking the update if possible.
     *
     * @tparam Event Type of event to which to connect the function.
     * @tparam Class Type of class to which the member function belongs.
     * @tparam Member Member function to connect to the signal.
     * @param instance A valid instance of the right type.
     */
    template<typename Event, typename Class, void(Class::*Member)(const Event &) = &Class::receive>
    void connect(instance_type<Class, Event> instance) noexcept {
        wrapper<Event>().template connect<Class, Member>(std::move(instance));
    }

    /**
     * @brief Unregisters a listener given in the form of a member function.
     *
     * A matching member function has the following signature:
     * `void receive(const Event &)`. Member functions named `receive` are
     * automatically detected and unregistered if available.
     *
     * @warning
     * Disconnecting a listener during an update may lead to unexpected
     * behavior. Unregister listeners before or after invoking the update if
     * possible.
     *
     * @tparam Event Type of event from which to disconnect the function.
     * @tparam Class Type of class to which the member function belongs.
     * @tparam Member Member function to connect to the signal.
     * @param instance A valid instance of the right type.
     */
    template<typename Event, typename Class, void(Class::*Member)(const Event &) = &Class::receive>
    void disconnect(instance_type<Class, Event> instance) noexcept {
        wrapper<Event>().template disconnect<Class, Member>(std::move(instance));
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

        for(auto pos = wrappers.size(); pos; --pos) {
            auto &wrapper = wrappers[pos-1];

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
