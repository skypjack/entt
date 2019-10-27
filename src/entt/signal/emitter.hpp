#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP


#include <type_traits>
#include <functional>
#include <algorithm>
#include <utility>
#include <memory>
#include <vector>
#include <list>
#include "../config/config.h"
#include "../core/family.hpp"
#include "../core/type_traits.hpp"


namespace entt {


/**
 * @brief General purpose event emitter.
 *
 * The emitter class template follows the CRTP idiom. To create a custom emitter
 * type, derived classes must inherit directly from the base class as:
 *
 * @code{.cpp}
 * struct my_emitter: emitter<my_emitter> {
 *     // ...
 * }
 * @endcode
 *
 * Handlers for the type of events are created internally on the fly. It's not
 * required to specify in advance the full list of accepted types.<br/>
 * Moreover, whenever an event is published, an emitter provides the listeners
 * with a reference to itself along with a const reference to the event.
 * Therefore listeners have an handy way to work with it without incurring in
 * the need of capturing a reference to the emitter.
 *
 * @tparam Derived Actual type of emitter that extends the class template.
 */
template<typename Derived>
class emitter {
    using handler_family = family<struct internal_emitter_handler_family>;

    struct base_handler {
        virtual ~base_handler() = default;
        virtual bool empty() const ENTT_NOEXCEPT = 0;
        virtual void clear() ENTT_NOEXCEPT = 0;
    };

    template<typename Event>
    struct event_handler: base_handler {
        using listener_type = std::function<void(const Event &, Derived &)>;
        using element_type = std::pair<bool, listener_type>;
        using container_type = std::list<element_type>;
        using connection_type = typename container_type::iterator;

        bool empty() const ENTT_NOEXCEPT override {
            auto pred = [](auto &&element) { return element.first; };

            return std::all_of(once_list.cbegin(), once_list.cend(), pred) &&
                    std::all_of(on_list.cbegin(), on_list.cend(), pred);
        }

        void clear() ENTT_NOEXCEPT override {
            if(publishing) {
                auto func = [](auto &&element) { element.first = true; };
                std::for_each(once_list.begin(), once_list.end(), func);
                std::for_each(on_list.begin(), on_list.end(), func);
            } else {
                once_list.clear();
                on_list.clear();
            }
        }

        connection_type once(listener_type listener) {
            return once_list.emplace(once_list.cend(), false, std::move(listener));
        }

        connection_type on(listener_type listener) {
            return on_list.emplace(on_list.cend(), false, std::move(listener));
        }

        void erase(connection_type conn) ENTT_NOEXCEPT {
            conn->first = true;

            if(!publishing) {
                auto pred = [](auto &&element) { return element.first; };
                once_list.remove_if(pred);
                on_list.remove_if(pred);
            }
        }

        void publish(const Event &event, Derived &ref) {
            container_type swap_list;
            once_list.swap(swap_list);

            auto func = [&event, &ref](auto &&element) {
                return element.first ? void() : element.second(event, ref);
            };

            publishing = true;

            std::for_each(on_list.rbegin(), on_list.rend(), func);
            std::for_each(swap_list.rbegin(), swap_list.rend(), func);

            publishing = false;

            on_list.remove_if([](auto &&element) { return element.first; });
        }

    private:
        bool publishing{false};
        container_type once_list{};
        container_type on_list{};
    };

    struct handler_data {
        std::unique_ptr<base_handler> handler;
        ENTT_ID_TYPE runtime_type;
    };

    template<typename Event>
    static auto type() ENTT_NOEXCEPT {
        if constexpr(is_named_type_v<Event>) {
            return named_type_traits_v<Event>;
        } else {
            return handler_family::type<std::decay_t<Event>>;
        }
    }

    template<typename Event>
    event_handler<Event> * assure() const ENTT_NOEXCEPT {
        const auto htype = type<Event>();
        handler_data *hdata = nullptr;

        if constexpr(is_named_type_v<Event>) {
            const auto it = std::find_if(handlers.begin(), handlers.end(), [htype](const auto &candidate) {
                return candidate.handler && candidate.runtime_type == htype;
            });

            hdata = (it == handlers.cend() ? &handlers.emplace_back() : &(*it));
        } else {
            if(!(htype < handlers.size())) {
                handlers.resize(htype+1);
            }

            hdata = &handlers[htype];

            if(hdata->handler && hdata->runtime_type != htype) {
                handlers.emplace_back();
                std::swap(handlers[htype], handlers.back());
                hdata = &handlers[htype];
            }
        }

        if(!hdata->handler) {
            hdata->handler = std::make_unique<event_handler<Event>>();
            hdata->runtime_type = htype;
        }

        return static_cast<event_handler<Event> *>(hdata->handler.get());
    }

public:
    /** @brief Type of listeners accepted for the given event. */
    template<typename Event>
    using listener = typename event_handler<Event>::listener_type;

    /**
     * @brief Generic connection type for events.
     *
     * Type of the connection object returned by the event emitter whenever a
     * listener for the given type is registered.<br/>
     * It can be used to break connections still in use.
     *
     * @tparam Event Type of event for which the connection is created.
     */
    template<typename Event>
    struct connection: private event_handler<Event>::connection_type {
        /** @brief Event emitters are friend classes of connections. */
        friend class emitter;

        /*! @brief Default constructor. */
        connection() ENTT_NOEXCEPT = default;

        /**
         * @brief Creates a connection that wraps its underlying instance.
         * @param conn A connection object to wrap.
         */
        connection(typename event_handler<Event>::connection_type conn)
            : event_handler<Event>::connection_type{std::move(conn)}
        {}
    };

    /*! @brief Default constructor. */
    emitter() ENTT_NOEXCEPT = default;

    /*! @brief Default destructor. */
    virtual ~emitter() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<emitter<Derived>, Derived>);
    }

    /*! @brief Default move constructor. */
    emitter(emitter &&) = default;

    /*! @brief Default move assignment operator. @return This emitter. */
    emitter & operator=(emitter &&) = default;

    /**
     * @brief Emits the given event.
     *
     * All the listeners registered for the specific event type are invoked with
     * the given event. The event type must either have a proper constructor for
     * the arguments provided or be an aggregate type.
     *
     * @tparam Event Type of event to publish.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Parameters to use to initialize the event.
     */
    template<typename Event, typename... Args>
    void publish(Args &&... args) {
        assure<Event>()->publish({ std::forward<Args>(args)... }, *static_cast<Derived *>(this));
    }

    /**
     * @brief Registers a long-lived listener with the event emitter.
     *
     * This method can be used to register a listener designed to be invoked
     * more than once for the given event type.<br/>
     * The connection returned by the method can be freely discarded. It's meant
     * to be used later to disconnect the listener if required.
     *
     * The listener is as a callable object that can be moved and the type of
     * which is `void(const Event &, Derived &)`.
     *
     * @note
     * Whenever an event is emitted, the emitter provides the listener with a
     * reference to the derived class. Listeners don't have to capture those
     * instances for later uses.
     *
     * @tparam Event Type of event to which to connect the listener.
     * @param instance The listener to register.
     * @return Connection object that can be used to disconnect the listener.
     */
    template<typename Event>
    connection<Event> on(listener<Event> instance) {
        return assure<Event>()->on(std::move(instance));
    }

    /**
     * @brief Registers a short-lived listener with the event emitter.
     *
     * This method can be used to register a listener designed to be invoked
     * only once for the given event type.<br/>
     * The connection returned by the method can be freely discarded. It's meant
     * to be used later to disconnect the listener if required.
     *
     * The listener is as a callable object that can be moved and the type of
     * which is `void(const Event &, Derived &)`.
     *
     * @note
     * Whenever an event is emitted, the emitter provides the listener with a
     * reference to the derived class. Listeners don't have to capture those
     * instances for later uses.
     *
     * @tparam Event Type of event to which to connect the listener.
     * @param instance The listener to register.
     * @return Connection object that can be used to disconnect the listener.
     */
    template<typename Event>
    connection<Event> once(listener<Event> instance) {
        return assure<Event>()->once(std::move(instance));
    }

    /**
     * @brief Disconnects a listener from the event emitter.
     *
     * Do not use twice the same connection to disconnect a listener, it results
     * in undefined behavior. Once used, discard the connection object.
     *
     * @tparam Event Type of event of the connection.
     * @param conn A valid connection.
     */
    template<typename Event>
    void erase(connection<Event> conn) ENTT_NOEXCEPT {
        assure<Event>()->erase(std::move(conn));
    }

    /**
     * @brief Disconnects all the listeners for the given event type.
     *
     * All the connections previously returned for the given event are
     * invalidated. Using them results in undefined behavior.
     *
     * @tparam Event Type of event to reset.
     */
    template<typename Event>
    void clear() ENTT_NOEXCEPT {
        assure<Event>()->clear();
    }

    /**
     * @brief Disconnects all the listeners.
     *
     * All the connections previously returned are invalidated. Using them
     * results in undefined behavior.
     */
    void clear() ENTT_NOEXCEPT {
        std::for_each(handlers.begin(), handlers.end(), [](auto &&hdata) {
            return hdata.handler ? hdata.handler->clear() : void();
        });
    }

    /**
     * @brief Checks if there are listeners registered for the specific event.
     * @tparam Event Type of event to test.
     * @return True if there are no listeners registered, false otherwise.
     */
    template<typename Event>
    bool empty() const ENTT_NOEXCEPT {
        return assure<Event>()->empty();
    }

    /**
     * @brief Checks if there are listeners registered with the event emitter.
     * @return True if there are no listeners registered, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return std::all_of(handlers.cbegin(), handlers.cend(), [](auto &&hdata) {
            return !hdata.handler || hdata.handler->empty();
        });
    }

private:
    mutable std::vector<handler_data> handlers{};
};


}


#endif // ENTT_SIGNAL_EMITTER_HPP
