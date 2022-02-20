#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/utility.hpp"
#include "fwd.hpp"
#include "sigh.hpp"

namespace entt {

/**
 * @brief Basic dispatcher implementation.
 *
 * A dispatcher can be used either to trigger an immediate event or to enqueue
 * events to be published all together once per tick.<br/>
 * Listeners are provided in the form of member functions. For each event of
 * type `Event`, listeners are such that they can be invoked with an argument of
 * type `Event &`, no matter what the return type is.
 *
 * The dispatcher creates instances of the `sigh` class internally. Refer to the
 * documentation of the latter for more details.
 */
class dispatcher {
    struct basic_pool {
        virtual ~basic_pool() = default;
        virtual void publish() = 0;
        virtual void disconnect(void *) = 0;
        virtual void clear() ENTT_NOEXCEPT = 0;
        virtual std::size_t size() const ENTT_NOEXCEPT = 0;
    };

    template<typename Event>
    struct pool_handler final: basic_pool {
        static_assert(std::is_same_v<Event, std::decay_t<Event>>, "Invalid event type");

        void publish() override {
            const auto length = events.size();

            for(std::size_t pos{}; pos < length; ++pos) {
                signal.publish(events[pos]);
            }

            events.erase(events.cbegin(), events.cbegin() + length);
        }

        void disconnect(void *instance) override {
            bucket().disconnect(instance);
        }

        void clear() ENTT_NOEXCEPT override {
            events.clear();
        }

        [[nodiscard]] auto bucket() ENTT_NOEXCEPT {
            using sink_type = typename sigh<void(Event &)>::sink_type;
            return sink_type{signal};
        }

        void trigger(Event event) {
            signal.publish(event);
        }

        template<typename... Args>
        void enqueue(Args &&...args) {
            if constexpr(std::is_aggregate_v<Event>) {
                events.push_back(Event{std::forward<Args>(args)...});
            } else {
                events.emplace_back(std::forward<Args>(args)...);
            }
        }

        std::size_t size() const ENTT_NOEXCEPT override {
            return events.size();
        }

    private:
        sigh<void(Event &)> signal{};
        std::vector<Event> events;
    };

    template<typename Event>
    [[nodiscard]] pool_handler<Event> &assure(const id_type id) {
        if(auto &&ptr = pools[id]; !ptr) {
            auto *cpool = new pool_handler<Event>{};
            ptr.reset(cpool);
            return *cpool;
        } else {
            return static_cast<pool_handler<Event> &>(*ptr);
        }
    }

    template<typename Event>
    [[nodiscard]] const pool_handler<Event> *assure(const id_type id) const {
        if(const auto it = pools.find(id); it != pools.end()) {
            return static_cast<const pool_handler<Event> *>(it->second.get());
        }

        return nullptr;
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /*! @brief Default constructor. */
    dispatcher() = default;

    /*! @brief Default move constructor. */
    dispatcher(dispatcher &&) = default;

    /*! @brief Default move assignment operator. @return This dispatcher. */
    dispatcher &operator=(dispatcher &&) = default;

    /**
     * @brief Returns the number of pending events for a given type.
     * @tparam Event Type of event for which to return the count.
     * @param id Name used to map the event queue within the dispatcher.
     * @return The number of pending events for the given type.
     */
    template<typename Event>
    size_type size(const id_type id = type_hash<Event>::value()) const ENTT_NOEXCEPT {
        const auto *cpool = assure<Event>(id);
        return cpool ? cpool->size() : 0u;
    }

    /**
     * @brief Returns the total number of pending events.
     * @return The total number of pending events.
     */
    size_type size() const ENTT_NOEXCEPT {
        size_type count{};

        for(auto &&cpool: pools) {
            count += cpool.second->size();
        }

        return count;
    }

    /**
     * @brief Returns a sink object for the given event and queue.
     *
     * A sink is an opaque object used to connect listeners to events.
     *
     * The function type for a listener is _compatible_ with:
     * @code{.cpp}
     * void(Event &);
     * @endcode
     *
     * The order of invocation of the listeners isn't guaranteed.
     *
     * @sa sink
     *
     * @tparam Event Type of event of which to get the sink.
     * @param id Name used to map the event queue within the dispatcher.
     * @return A temporary sink object.
     */
    template<typename Event>
    [[nodiscard]] auto sink(const id_type id = type_hash<Event>::value()) {
        return assure<Event>(id).bucket();
    }

    /**
     * @brief Triggers an immediate event of a given type.
     * @tparam Event Type of event to trigger.
     * @param event An instance of the given type of event.
     */
    template<typename Event>
    void trigger(Event &&event = {}) {
        trigger(type_hash<std::decay_t<Event>>::value(), std::forward<Event>(event));
    }

    /**
     * @brief Triggers an immediate event on a queue of a given type.
     * @tparam Event Type of event to trigger.
     * @param event An instance of the given type of event.
     * @param id Name used to map the event queue within the dispatcher.
     */
    template<typename Event>
    void trigger(const id_type id, Event &&event = {}) {
        assure<std::decay_t<Event>>(id).trigger(std::forward<Event>(event));
    }

    /**
     * @brief Enqueues an event of the given type.
     * @tparam Event Type of event to enqueue.
     * @tparam Args Types of arguments to use to construct the event.
     * @param args Arguments to use to construct the event.
     */
    template<typename Event, typename... Args>
    void enqueue(Args &&...args) {
        enqueue_hint<Event>(type_hash<Event>::value(), std::forward<Args>(args)...);
    }

    /**
     * @brief Enqueues an event of the given type.
     * @tparam Event Type of event to enqueue.
     * @param event An instance of the given type of event.
     */
    template<typename Event>
    void enqueue(Event &&event) {
        enqueue_hint(type_hash<std::decay_t<Event>>::value(), std::forward<Event>(event));
    }

    /**
     * @brief Enqueues an event of the given type.
     * @tparam Event Type of event to enqueue.
     * @tparam Args Types of arguments to use to construct the event.
     * @param id Name used to map the event queue within the dispatcher.
     * @param args Arguments to use to construct the event.
     */
    template<typename Event, typename... Args>
    void enqueue_hint(const id_type id, Args &&...args) {
        assure<Event>(id).enqueue(std::forward<Args>(args)...);
    }

    /**
     * @brief Enqueues an event of the given type.
     * @tparam Event Type of event to enqueue.
     * @param id Name used to map the event queue within the dispatcher.
     * @param event An instance of the given type of event.
     */
    template<typename Event>
    void enqueue_hint(const id_type id, Event &&event) {
        assure<std::decay_t<Event>>(id).enqueue(std::forward<Event>(event));
    }

    /**
     * @brief Utility function to disconnect everything related to a given value
     * or instance from a dispatcher.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type &value_or_instance) {
        disconnect(&value_or_instance);
    }

    /**
     * @brief Utility function to disconnect everything related to a given value
     * or instance from a dispatcher.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type *value_or_instance) {
        for(auto &&cpool: pools) {
            cpool.second->disconnect(value_or_instance);
        }
    }

    /**
     * @brief Discards all the events stored so far in a given queue.
     * @tparam Event Type of event to discard.
     * @param id Name used to map the event queue within the dispatcher.
     */
    template<typename Event>
    void clear(const id_type id = type_hash<Event>::value()) {
        assure<Event>(id).clear();
    }

    /*! @brief Discards all the events queued so far. */
    void clear() ENTT_NOEXCEPT {
        for(auto &&cpool: pools) {
            cpool.second->clear();
        }
    }

    /**
     * @brief Delivers all the pending events of a given queue.
     * @tparam Event Type of event to send.
     * @param id Name used to map the event queue within the dispatcher.
     */
    template<typename Event>
    void update(const id_type id = type_hash<Event>::value()) {
        assure<Event>(id).publish();
    }

    /*! @brief Delivers all the pending events. */
    void update() const {
        for(auto &&cpool: pools) {
            cpool.second->publish();
        }
    }

private:
    dense_map<id_type, std::unique_ptr<basic_pool>, identity> pools;
};

} // namespace entt

#endif
