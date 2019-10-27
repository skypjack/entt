#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP


#include <vector>
#include <memory>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/family.hpp"
#include "../core/type_traits.hpp"
#include "sigh.hpp"


namespace entt {


/**
 * @brief Basic dispatcher implementation.
 *
 * A dispatcher can be used either to trigger an immediate event or to enqueue
 * events to be published all together once per tick.<br/>
 * Listeners are provided in the form of member functions. For each event of
 * type `Event`, listeners are such that they can be invoked with an argument of
 * type `const Event &`, no matter what the return type is.
 *
 * The types of the instances are `Class &`. Users must guarantee that the
 * lifetimes of the objects overcome the one of the dispatcher itself to avoid
 * crashes.
 */
class dispatcher {
    using event_family = family<struct internal_dispatcher_event_family>;

    template<typename Class, typename Event>
    using instance_type = typename sigh<void(const Event &)>::template instance_type<Class>;

    struct base_wrapper {
        virtual ~base_wrapper() = default;
        virtual void publish() = 0;
        virtual void clear() = 0;
    };

    template<typename Event>
    struct signal_wrapper: base_wrapper {
        using signal_type = sigh<void(const Event &)>;
        using sink_type = typename signal_type::sink_type;

        void publish() override {
            const auto length = events.size();

            for(std::size_t pos{}; pos < length; ++pos) {
                signal.publish(events[pos]);
            }

            events.erase(events.cbegin(), events.cbegin()+length);
        }

        void clear() override {
            events.clear();
        }

        sink_type sink() ENTT_NOEXCEPT {
            return entt::sink{signal};
        }

        template<typename... Args>
        void trigger(Args &&... args) {
            signal.publish({ std::forward<Args>(args)... });
        }

        template<typename... Args>
        void enqueue(Args &&... args) {
            events.emplace_back(std::forward<Args>(args)...);
        }

    private:
        signal_type signal{};
        std::vector<Event> events;
    };

    struct wrapper_data {
        std::unique_ptr<base_wrapper> wrapper;
        ENTT_ID_TYPE runtime_type;
    };

    template<typename Event>
    static auto type() ENTT_NOEXCEPT {
        if constexpr(is_named_type_v<Event>) {
            return named_type_traits_v<Event>;
        } else {
            return event_family::type<std::decay_t<Event>>;
        }
    }

    template<typename Event>
    signal_wrapper<Event> & assure() {
        const auto wtype = type<Event>();
        wrapper_data *wdata = nullptr;

        if constexpr(is_named_type_v<Event>) {
            const auto it = std::find_if(wrappers.begin(), wrappers.end(), [wtype](const auto &candidate) {
                return candidate.wrapper && candidate.runtime_type == wtype;
            });

            wdata = (it == wrappers.cend() ? &wrappers.emplace_back() : &(*it));
        } else {
            if(!(wtype < wrappers.size())) {
                wrappers.resize(wtype+1);
            }

            wdata = &wrappers[wtype];

            if(wdata->wrapper && wdata->runtime_type != wtype) {
                wrappers.emplace_back();
                std::swap(wrappers[wtype], wrappers.back());
                wdata = &wrappers[wtype];
            }
        }

        if(!wdata->wrapper) {
            wdata->wrapper = std::make_unique<signal_wrapper<Event>>();
            wdata->runtime_type = wtype;
        }

        return static_cast<signal_wrapper<Event> &>(*wdata->wrapper);
    }

public:
    /*! @brief Type of sink for the given event. */
    template<typename Event>
    using sink_type = typename signal_wrapper<Event>::sink_type;

    /**
     * @brief Returns a sink object for the given event.
     *
     * A sink is an opaque object used to connect listeners to events.
     *
     * The function type for a listener is:
     * @code{.cpp}
     * void(const Event &);
     * @endcode
     *
     * The order of invocation of the listeners isn't guaranteed.
     *
     * @sa sink
     *
     * @tparam Event Type of event of which to get the sink.
     * @return A temporary sink object.
     */
    template<typename Event>
    sink_type<Event> sink() ENTT_NOEXCEPT {
        return assure<Event>().sink();
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
        assure<Event>().trigger(std::forward<Args>(args)...);
    }

    /**
     * @brief Triggers an immediate event of the given type.
     *
     * All the listeners registered for the given type are immediately notified.
     * The event is discarded after the execution.
     *
     * @tparam Event Type of event to trigger.
     * @param event An instance of the given type of event.
     */
    template<typename Event>
    void trigger(Event &&event) {
        assure<std::decay_t<Event>>().trigger(std::forward<Event>(event));
    }

    /**
     * @brief Enqueues an event of the given type.
     *
     * An event of the given type is queued. No listener is invoked. Use the
     * `update` member function to notify listeners when ready.
     *
     * @tparam Event Type of event to enqueue.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename Event, typename... Args>
    void enqueue(Args &&... args) {
        assure<Event>().enqueue(std::forward<Args>(args)...);
    }

    /**
     * @brief Enqueues an event of the given type.
     *
     * An event of the given type is queued. No listener is invoked. Use the
     * `update` member function to notify listeners when ready.
     *
     * @tparam Event Type of event to enqueue.
     * @param event An instance of the given type of event.
     */
    template<typename Event>
    void enqueue(Event &&event) {
        assure<std::decay_t<Event>>().enqueue(std::forward<Event>(event));
    }

    /**
     * @brief Discards all the events queued so far.
     *
     * If no types are provided, the dispatcher will clear all the existing
     * pools.
     *
     * @tparam Event Type of events to discard.
     */
    template<typename... Event>
    void discard() {
        if constexpr(sizeof...(Event) == 0) {
            std::for_each(wrappers.begin(), wrappers.end(), [](auto &&wdata) {
                if(wdata.wrapper) {
                    wdata.wrapper->clear();
                }
            });
        } else {
            (assure<std::decay_t<Event>>().clear(), ...);
        }
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
    void update() {
        assure<Event>().publish();
    }

    /**
     * @brief Delivers all the pending events.
     *
     * This method is blocking and it doesn't return until all the events are
     * delivered to the registered listeners. It's responsibility of the users
     * to reduce at a minimum the time spent in the bodies of the listeners.
     */
    void update() const {
        for(auto pos = wrappers.size(); pos; --pos) {
            if(auto &wdata = wrappers[pos-1]; wdata.wrapper) {
                wdata.wrapper->publish();
            }
        }
    }

private:
    std::vector<wrapper_data> wrappers;
};


}


#endif // ENTT_SIGNAL_DISPATCHER_HPP
