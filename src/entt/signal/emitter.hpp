#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP

#include <functional>
#include <type_traits>
#include <utility>
#include "../container/dense_map.hpp"
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
 */
template<typename Derived>
class emitter {
    using function_type = std::function<void(void *)>;

public:
    /*! @brief Default constructor. */
    emitter()
        : handlers{} {}

    /*! @brief Default destructor. */
    virtual ~emitter() noexcept {
        static_assert(std::is_base_of_v<emitter<Derived>, Derived>, "Invalid emitter type");
    }

    /*! @brief Default move constructor. */
    emitter(emitter &&) noexcept = default;

    /**
     * @brief Default move assignment operator.
     * @return This emitter.
     */
    emitter &operator=(emitter &&) noexcept = default;

    /**
     * @brief Publishes a given event.
     * @tparam Type Type of event to trigger.
     * @param value An instance of the given type of event.
     */
    template<typename Type>
    void publish(Type &&value) {
        if(const auto id = type_id<Type>().hash(); handlers.contains(id)) {
            handlers[id](&value);
        }
    }

    /**
     * @brief Registers a listener with the event emitter.
     * @tparam Type Type of event to which to connect the listener.
     * @param func The listener to register.
     */
    template<typename Type>
    void on(std::function<void(Type &, Derived &)> func) {
        handlers.insert_or_assign(type_id<Type>().hash(), [func = std::move(func), this](void *value) {
            func(*static_cast<Type *>(value), static_cast<Derived &>(*this));
        });
    }

    /**
     * @brief Disconnects a listener from the event emitter.
     * @tparam Type Type of event of the listener.
     */
    template<typename Type>
    void erase() {
        handlers.erase(type_hash<std::remove_cv_t<std::remove_reference_t<Type>>>::value());
    }

    /*! @brief Disconnects all the listeners. */
    void clear() noexcept {
        handlers.clear();
    }

    /**
     * @brief Checks if there are listeners registered for the specific event.
     * @tparam Type Type of event to test.
     * @return True if there are no listeners registered, false otherwise.
     */
    template<typename Type>
    [[nodiscard]] bool contains() const {
        return handlers.contains(type_hash<std::remove_cv_t<std::remove_reference_t<Type>>>::value());
    }

    /**
     * @brief Checks if there are listeners registered with the event emitter.
     * @return True if there are no listeners registered, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return handlers.empty();
    }

private:
    dense_map<id_type, function_type, identity> handlers{};
};

} // namespace entt

#endif
