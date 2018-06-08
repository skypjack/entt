#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP


#include <type_traits>
#include <functional>
#include <algorithm>
#include <utility>
#include <cstdint>
#include <memory>
#include <vector>
#include <list>
#include "../config/config.h"
#include "../core/family.hpp"


namespace entt {


/**
 * @brief General purpose event emitter.
 *
 * The emitter class template follows the CRTP idiom. To create a custom emitter
 * type, derived classes must inherit directly from the base class as:
 *
 * ```cpp
 * struct MyEmitter: Emitter<MyEmitter> {
 *     // ...
 * }
 * ```
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
class Emitter {
    using handler_family = Family<struct InternalEmitterHandlerFamily>;

    struct BaseHandler {
        virtual ~BaseHandler() = default;
        virtual bool empty() const ENTT_NOEXCEPT = 0;
        virtual void clear() ENTT_NOEXCEPT = 0;
    };

    template<typename Event>
    struct Handler final: BaseHandler {
        using listener_type = std::function<void(const Event &, Derived &)>;
        using element_type = std::pair<bool, listener_type>;
        using container_type = std::list<element_type>;
        using connection_type = typename container_type::iterator;

        bool empty() const ENTT_NOEXCEPT override {
            auto pred = [](auto &&element) { return element.first; };

            return std::all_of(onceL.cbegin(), onceL.cend(), pred) &&
                    std::all_of(onL.cbegin(), onL.cend(), pred);
        }

        void clear() ENTT_NOEXCEPT override {
            if(publishing) {
                auto func = [](auto &&element) { element.first = true; };
                std::for_each(onceL.begin(), onceL.end(), func);
                std::for_each(onL.begin(), onL.end(), func);
            } else {
                onceL.clear();
                onL.clear();
            }
        }

        inline connection_type once(listener_type listener) {
            return onceL.emplace(onceL.cend(), false, std::move(listener));
        }

        inline connection_type on(listener_type listener) {
            return onL.emplace(onL.cend(), false, std::move(listener));
        }

        void erase(connection_type conn) ENTT_NOEXCEPT {
            conn->first = true;

            if(!publishing) {
                auto pred = [](auto &&element) { return element.first; };
                onceL.remove_if(pred);
                onL.remove_if(pred);
            }
        }

        void publish(const Event &event, Derived &ref) {
            container_type currentL;
            onceL.swap(currentL);

            auto func = [&event, &ref](auto &&element) {
                return element.first ? void() : element.second(event, ref);
            };

            publishing = true;

            std::for_each(onL.rbegin(), onL.rend(), func);
            std::for_each(currentL.rbegin(), currentL.rend(), func);

            publishing = false;

            onL.remove_if([](auto &&element) { return element.first; });
        }

    private:
        bool publishing{false};
        container_type onceL{};
        container_type onL{};
    };

    template<typename Event>
    Handler<Event> & handler() ENTT_NOEXCEPT {
        const std::size_t family = handler_family::type<Event>();

        if(!(family < handlers.size())) {
            handlers.resize(family+1);
        }

        if(!handlers[family]) {
            handlers[family] = std::make_unique<Handler<Event>>();
        }

        return static_cast<Handler<Event> &>(*handlers[family]);
    }

public:
    /** @brief Type of listeners accepted for the given event. */
    template<typename Event>
    using Listener = typename Handler<Event>::listener_type;

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
    struct Connection final: private Handler<Event>::connection_type {
        /** @brief Event emitters are friend classes of connections. */
        friend class Emitter;

        /*! @brief Default constructor. */
        Connection() ENTT_NOEXCEPT = default;

        /**
         * @brief Creates a connection that wraps its underlying instance.
         * @param conn A connection object to wrap.
         */
        Connection(typename Handler<Event>::connection_type conn)
            : Handler<Event>::connection_type{std::move(conn)}
        {}

        /*! @brief Default copy constructor. */
        Connection(const Connection &) = default;
        /*! @brief Default move constructor. */
        Connection(Connection &&) = default;

        /**
         * @brief Default copy assignment operator.
         * @return This connection.
         */
        Connection & operator=(const Connection &) = default;

        /**
         * @brief Default move assignment operator.
         * @return This connection.
         */
        Connection & operator=(Connection &&) = default;
    };

    /*! @brief Default constructor. */
    Emitter() ENTT_NOEXCEPT = default;

    /*! @brief Default destructor. */
    virtual ~Emitter() ENTT_NOEXCEPT {
        static_assert(std::is_base_of<Emitter<Derived>, Derived>::value, "!");
    }

    /*! @brief Copying an emitter isn't allowed. */
    Emitter(const Emitter &) = delete;
    /*! @brief Default move constructor. */
    Emitter(Emitter &&) = default;

    /*! @brief Copying an emitter isn't allowed. @return This emitter. */
    Emitter & operator=(const Emitter &) = delete;
    /*! @brief Default move assignment operator. @return This emitter. */
    Emitter & operator=(Emitter &&) = default;

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
        handler<Event>().publish({ std::forward<Args>(args)... }, *static_cast<Derived *>(this));
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
     * @param listener The listener to register.
     * @return Connection object that can be used to disconnect the listener.
     */
    template<typename Event>
    Connection<Event> on(Listener<Event> listener) {
        return handler<Event>().on(std::move(listener));
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
     * @param listener The listener to register.
     * @return Connection object that can be used to disconnect the listener.
     */
    template<typename Event>
    Connection<Event> once(Listener<Event> listener) {
        return handler<Event>().once(std::move(listener));
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
    void erase(Connection<Event> conn) ENTT_NOEXCEPT {
        handler<Event>().erase(std::move(conn));
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
        handler<Event>().clear();
    }

    /**
     * @brief Disconnects all the listeners.
     *
     * All the connections previously returned are invalidated. Using them
     * results in undefined behavior.
     */
    void clear() ENTT_NOEXCEPT {
        std::for_each(handlers.begin(), handlers.end(), [](auto &&handler) {
            return handler ? handler->clear() : void();
        });
    }

    /**
     * @brief Checks if there are listeners registered for the specific event.
     * @tparam Event Type of event to test.
     * @return True if there are no listeners registered, false otherwise.
     */
    template<typename Event>
    bool empty() const ENTT_NOEXCEPT {
        const std::size_t family = handler_family::type<Event>();

        return (!(family < handlers.size()) ||
                !handlers[family] ||
                static_cast<Handler<Event> &>(*handlers[family]).empty());
    }

    /**
     * @brief Checks if there are listeners registered with the event emitter.
     * @return True if there are no listeners registered, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return std::all_of(handlers.cbegin(), handlers.cend(), [](auto &&handler) {
            return !handler || handler->empty();
        });
    }

private:
    std::vector<std::unique_ptr<BaseHandler>> handlers{};
};


}


#endif // ENTT_SIGNAL_EMITTER_HPP
