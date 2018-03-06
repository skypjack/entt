#ifndef ENTT_SIGNAL_BUS_HPP
#define ENTT_SIGNAL_BUS_HPP


#include <cstddef>
#include <utility>
#include "signal.hpp"
#include "sigh.hpp"


namespace entt {


/**
 * @brief Minimal event bus.
 *
 * Primary template isn't defined on purpose. The main reason for which it
 * exists is to work around the doxygen's parsing capabilities. In fact, there
 * is no need to declare it actually.
 */
template<template<typename...> class, typename...>
class Bus;


/**
 * @brief Event bus specialization for multiple types.
 *
 * The event bus is designed to allow an easy registration of specific member
 * functions to a bunch of signal handlers (either manager or unmanaged).
 * Classes must publicly expose the required member functions to allow the bus
 * to detect them for the purpose of registering and unregistering
 * instances.<br/>
 * In particular, for each event type `E`, a matching member function has the
 * following signature: `void receive(const E &)`. Events will be properly
 * redirected to all the listeners by calling the right member functions, if
 * any.
 *
 * @tparam Sig Type of signal handler to use.
 * @tparam Event The list of events managed by the bus.
 */
template<template<typename...> class Sig, typename Event, typename... Other>
class Bus<Sig, Event, Other...>
        : private Bus<Sig, Event>, private Bus<Sig, Other>...
{
public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Unregisters all the member functions of an instance.
     *
     * A bus is used to convey a certain set of events. This method detects
     * and unregisters from the bus all the matching member functions of an
     * instance.<br/>
     * For each event type `E`, a matching member function has the following
     * signature: `void receive(const E &)`.
     *
     * @tparam Instance Type of instance to unregister.
     * @param instance A valid instance of the right type.
     */
    template<typename Instance>
    void unreg(Instance instance) {
        using accumulator_type = int[];
        accumulator_type accumulator = {
            (Bus<Sig, Event>::unreg(instance), 0),
            (Bus<Sig, Other>::unreg(instance), 0)...
        };
        return void(accumulator);
    }

    /**
     * @brief Registers all the member functions of an instance.
     *
     * A bus is used to convey a certain set of events. This method detects
     * and registers to the bus all the matching member functions of an
     * instance.<br/>
     * For each event type `E`, a matching member function has the following
     * signature: `void receive(const E &)`.
     *
     * @tparam Instance Type of instance to register.
     * @param instance A valid instance of the right type.
     */
    template<typename Instance>
    void reg(Instance instance) {
        using accumulator_type = int[];
        accumulator_type accumulator = {
            (Bus<Sig, Event>::reg(instance), 0),
            (Bus<Sig, Other>::reg(instance), 0)...
        };
        return void(accumulator);
    }

    /**
     * @brief Number of listeners connected to the bus.
     * @return Number of listeners currently connected.
     */
    size_type size() const noexcept {
        using accumulator_type = std::size_t[];
        std::size_t sz = Bus<Sig, Event>::size();
        accumulator_type accumulator = { sz, (sz += Bus<Sig, Other>::size())... };
        return void(accumulator), sz;
    }

    /**
     * @brief Returns false if at least a listener is connected to the bus.
     * @return True if the bus has no listeners connected, false otherwise.
     */
    bool empty() const noexcept {
        using accumulator_type = bool[];
        bool ret = Bus<Sig, Event>::empty();
        accumulator_type accumulator = { ret, (ret = ret && Bus<Sig, Other>::empty())... };
        return void(accumulator), ret;
    }

    /**
     * @brief Connects a free function to the bus.
     * @tparam Type Type of event to which to connect the function.
     * @tparam Function A valid free function pointer.
     */
    template<typename Type, void(*Function)(const Type &)>
    void connect() {
        Bus<Sig, Type>::template connect<Function>();
    }

    /**
     * @brief Disconnects a free function from the bus.
     * @tparam Type Type of event from which to disconnect the function.
     * @tparam Function A valid free function pointer.
     */
    template<typename Type, void(*Function)(const Type &)>
    void disconnect() {
        Bus<Sig, Type>::template disconnect<Function>();
    }

    /**
     * @brief Publishes an event.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @tparam Type Type of event to publish.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename Type, typename... Args>
    void publish(Args &&... args) {
        Bus<Sig, Type>::publish(std::forward<Args>(args)...);
    }
};


/**
 * @brief Event bus specialization for a single type.
 *
 * The event bus is designed to allow an easy registration of a specific member
 * function to a signal handler (either manager or unmanaged).
 * Classes must publicly expose the required member function to allow the bus to
 * detect it for the purpose of registering and unregistering instances.<br/>
 * In particular, a matching member function has the following signature:
 * `void receive(const Event &)`. Events of the given type will be properly
 * redirected to all the listeners by calling the right member function, if any.
 *
 * @tparam Sig Type of signal handler to use.
 * @tparam Event Type of event managed by the bus.
 */
template<template<typename...> class Sig, typename Event>
class Bus<Sig, Event> {
    using signal_type = Sig<void(const Event &)>;

    template<typename Class>
    using instance_type = typename signal_type::template instance_type<Class>;

    template<typename Class>
    auto disconnect(int, instance_type<Class> instance)
    -> decltype(std::declval<Class>().receive(std::declval<Event>()), void()) {
        signal.template disconnect<Class, &Class::receive>(std::move(instance));
    }

    template<typename Class>
    auto connect(int, instance_type<Class> instance)
    -> decltype(std::declval<Class>().receive(std::declval<Event>()), void()) {
        signal.template connect<Class, &Class::receive>(std::move(instance));
    }

    template<typename Class> void disconnect(char, instance_type<Class>) {}
    template<typename Class> void connect(char, instance_type<Class>) {}

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename signal_type::size_type;

    /**
     * @brief Unregisters member functions of instances.
     *
     * This method tries to detect and unregister from the bus matching member
     * functions of instances.<br/>
     * A matching member function has the following signature:
     * `void receive(const Event &)`.
     *
     * @tparam Class Type of instance to unregister.
     * @param instance A valid instance of the right type.
     */
    template<typename Class>
    void unreg(instance_type<Class> instance) {
        disconnect(0, std::move(instance));
    }

    /**
     * @brief Tries to register an instance.
     *
     * This method tries to detect and register to the bus matching member
     * functions of instances.<br/>
     * A matching member function has the following signature:
     * `void receive(const Event &)`.
     *
     * @tparam Class Type of instance to register.
     * @param instance A valid instance of the right type.
     */
    template<typename Class>
    void reg(instance_type<Class> instance) {
        connect(0, std::move(instance));
    }

    /**
     * @brief Number of listeners connected to the bus.
     * @return Number of listeners currently connected.
     */
    size_type size() const noexcept {
        return signal.size();
    }

    /**
     * @brief Returns false if at least a listener is connected to the bus.
     * @return True if the bus has no listeners connected, false otherwise.
     */
    bool empty() const noexcept {
        return signal.empty();
    }

    /**
     * @brief Connects a free function to the bus.
     * @tparam Function A valid free function pointer.
     */
    template<void(*Function)(const Event &)>
    void connect() {
        signal.template connect<Function>();
    }

    /**
     * @brief Disconnects a free function from the bus.
     * @tparam Function A valid free function pointer.
     */
    template<void(*Function)(const Event &)>
    void disconnect() {
        signal.template disconnect<Function>();
    }

    /**
     * @brief Publishes an event.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename... Args>
    void publish(Args &&... args) {
        signal.publish({ std::forward<Args>(args)... });
    }

private:
    signal_type signal;
};


/**
 * @brief Managed event bus.
 *
 * A managed event bus uses the Signal class template as an underlying type. The
 * type of the instances is the one required by the signal handler:
 * `std::shared_ptr<Class>` (a shared pointer).
 *
 * @tparam Event The list of events managed by the bus.
 */
template<typename... Event>
using ManagedBus = Bus<Signal, Event...>;

/**
 * @brief Unmanaged event bus.
 *
 * An unmanaged event bus uses the SigH class template as an underlying type.
 * The type of the instances is the one required by the signal handler:
 * `Class *` (a naked pointer).<br/>
 * When it comes to work with this kind of bus, users must guarantee that the
 * lifetimes of the instances overcome the one of the bus itself.
 *
 * @tparam Event The list of events managed by the bus.
 */
template<typename... Event>
using UnmanagedBus = Bus<SigH, Event...>;


}


#endif // ENTT_SIGNAL_BUS_HPP
