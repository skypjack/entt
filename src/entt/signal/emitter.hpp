#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP

#include <functional>
#include <type_traits>
#include <utility>
#include "../container/dense_map.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/utility.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @brief General purpose event emitter.
 *
 * To create an emitter type, derived classes must inherit from the base as:
 *
 * @code{.cpp}
 * struct my_emitter: emitter<my_emitter> {
 *     // ...
 * }
 * @endcode
 *
 * Handlers for the different events are created internally on the fly. It's not
 * required to specify in advance the full list of accepted events.<br/>
 * Moreover, whenever an event is published, an emitter also passes a reference
 * to itself to its listeners.
 *
 * @tparam Derived Emitter type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Derived, typename Allocator>
class emitter {
    using key_type = id_type;
    using mapped_type = std::function<void(void *)>;

    using alloc_traits = std::allocator_traits<Allocator>;
    using container_allocator = typename alloc_traits::template rebind_alloc<std::pair<const key_type, mapped_type>>;
    using container_type = dense_map<key_type, mapped_type, identity, std::equal_to<key_type>, container_allocator>;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /*! @brief Default constructor. */
    emitter()
        : emitter{allocator_type{}} {}

    /**
     * @brief Constructs an emitter with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit emitter(const allocator_type &allocator)
        : handlers{allocator, allocator} {}

    /*! @brief Default destructor. */
    virtual ~emitter() noexcept {
        static_assert(std::is_base_of_v<emitter<Derived, Allocator>, Derived>, "Invalid emitter type");
    }

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    emitter(emitter &&other) noexcept
        : handlers{std::move(other.handlers)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    emitter(emitter &&other, const allocator_type &allocator) noexcept
        : handlers{container_type{std::move(other.handlers.first()), allocator}, allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || handlers.second() == other.handlers.second(), "Copying an emitter is not allowed");
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This dispatcher.
     */
    emitter &operator=(emitter &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || handlers.second() == other.handlers.second(), "Copying an emitter is not allowed");

        handlers = std::move(other.handlers);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given emitter.
     * @param other Emitter to exchange the content with.
     */
    void swap(emitter &other) {
        using std::swap;
        swap(handlers, other.handlers);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return handlers.second();
    }

    /**
     * @brief Publishes a given event.
     * @tparam Type Type of event to trigger.
     * @param value An instance of the given type of event.
     */
    template<typename Type>
    void publish(Type &&value) {
        if(const auto id = type_id<Type>().hash(); handlers.first().contains(id)) {
            handlers.first()[id](&value);
        }
    }

    /**
     * @brief Registers a listener with the event emitter.
     * @tparam Type Type of event to which to connect the listener.
     * @param func The listener to register.
     */
    template<typename Type>
    void on(std::function<void(Type &, Derived &)> func) {
        handlers.first().insert_or_assign(type_id<Type>().hash(), [func = std::move(func), this](void *value) {
            func(*static_cast<Type *>(value), static_cast<Derived &>(*this));
        });
    }

    /**
     * @brief Disconnects a listener from the event emitter.
     * @tparam Type Type of event of the listener.
     */
    template<typename Type>
    void erase() {
        handlers.first().erase(type_hash<std::remove_cv_t<std::remove_reference_t<Type>>>::value());
    }

    /*! @brief Disconnects all the listeners. */
    void clear() noexcept {
        handlers.first().clear();
    }

    /**
     * @brief Checks if there are listeners registered for the specific event.
     * @tparam Type Type of event to test.
     * @return True if there are no listeners registered, false otherwise.
     */
    template<typename Type>
    [[nodiscard]] bool contains() const {
        return handlers.first().contains(type_hash<std::remove_cv_t<std::remove_reference_t<Type>>>::value());
    }

    /**
     * @brief Checks if there are listeners registered with the event emitter.
     * @return True if there are no listeners registered, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return handlers.first().empty();
    }

private:
    compressed_pair<container_type, allocator_type> handlers;
};

} // namespace entt

#endif
