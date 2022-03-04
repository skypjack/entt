#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP

#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/utility.hpp"
#include "fwd.hpp"
#include "sigh.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

struct basic_dispatcher_handler {
    virtual ~basic_dispatcher_handler() = default;
    virtual void publish() = 0;
    virtual void disconnect(void *) = 0;
    virtual void clear() ENTT_NOEXCEPT = 0;
    virtual std::size_t size() const ENTT_NOEXCEPT = 0;
};

template<typename Event, typename Allocator>
class dispatcher_handler final: public basic_dispatcher_handler {
    static_assert(std::is_same_v<Event, std::decay_t<Event>>, "Invalid event type");

    using alloc_traits = std::allocator_traits<Allocator>;
    using signal_type = sigh<void(Event &), typename alloc_traits::template rebind_alloc<void (*)(Event &)>>;
    using container_type = std::vector<Event, typename alloc_traits::template rebind_alloc<Event>>;

public:
    using allocator_type = Allocator;

    dispatcher_handler(const allocator_type &allocator)
        : signal{allocator},
          events{allocator} {}

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
    signal_type signal;
    container_type events;
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

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
 *
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Allocator>
class basic_dispatcher {
    template<typename Event>
    using handler_type = internal::dispatcher_handler<Event, Allocator>;

    using key_type = id_type;
    // std::shared_ptr because of its type erased allocator which is pretty useful here
    using mapped_type = std::shared_ptr<internal::basic_dispatcher_handler>;

    using alloc_traits = std::allocator_traits<Allocator>;
    using container_allocator = typename alloc_traits::template rebind_alloc<std::pair<const key_type, mapped_type>>;
    using container_type = dense_map<id_type, mapped_type, identity, std::equal_to<id_type>, container_allocator>;

    template<typename Event>
    [[nodiscard]] handler_type<Event> &assure(const id_type id) {
        auto &&ptr = pools.first()[id];

        if(!ptr) {
            const auto &allocator = pools.second();
            ptr = std::allocate_shared<handler_type<Event>>(allocator, allocator);
        }

        return static_cast<handler_type<Event> &>(*ptr);
    }

    template<typename Event>
    [[nodiscard]] const handler_type<Event> *assure(const id_type id) const {
        auto &container = pools.first();

        if(const auto it = container.find(id); it != container.end()) {
            return static_cast<const handler_type<Event> *>(it->second.get());
        }

        return nullptr;
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /*! @brief Default constructor. */
    basic_dispatcher()
        : basic_dispatcher{allocator_type{}} {}

    /**
     * @brief Constructs a dispatcher with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_dispatcher(const allocator_type &allocator)
        : pools{allocator, allocator} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_dispatcher(basic_dispatcher &&other) ENTT_NOEXCEPT
        : pools{std::move(other.pools)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_dispatcher(basic_dispatcher &&other, const allocator_type &allocator) ENTT_NOEXCEPT
        : pools{container_type{std::move(other.pools.first()), allocator}, allocator} {}

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This dispatcher.
     */
    basic_dispatcher &operator=(basic_dispatcher &&other) ENTT_NOEXCEPT {
        pools = std::move(other.pools);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given dispatcher.
     * @param other Dispatcher to exchange the content with.
     */
    void swap(basic_dispatcher &other) {
        using std::swap;
        swap(pools, other.pools);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const ENTT_NOEXCEPT {
        return pools.second();
    }

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

        for(auto &&cpool: pools.first()) {
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
        for(auto &&cpool: pools.first()) {
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
        for(auto &&cpool: pools.first()) {
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
        for(auto &&cpool: pools.first()) {
            cpool.second->publish();
        }
    }

private:
    compressed_pair<container_type, allocator_type> pools;
};

} // namespace entt

#endif
