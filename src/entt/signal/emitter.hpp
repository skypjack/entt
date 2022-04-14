#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
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
    template<typename Type>
    using function_type = std::function<void(Type &, Derived &)>;

    template<typename Type>
    [[nodiscard]] function_type<Type> &assure() {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Non-decayed types not allowed");
        auto &&ptr = handlers[type_hash<Type>::value()];

        if(!ptr) {
            ptr = std::make_shared<function_type<Type>>();
        }

        return *static_cast<function_type<Type> *>(ptr.get());
    }

    template<typename Type>
    [[nodiscard]] const function_type<Type> *assure() const {
        const auto it = handlers.find(type_hash<Type>::value());
        return (it == handlers.cend()) ? nullptr : static_cast<const function_type<Type> *>(it->second.get());
    }

public:
    /*! @brief Default constructor. */
    emitter()
        : handlers{} {}

    /*! @brief Default destructor. */
    virtual ~emitter() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<emitter<Derived>, Derived>, "Invalid emitter type");
    }

    /*! @brief Default move constructor. */
    emitter(emitter &&) = default;

    /**
     * @brief Default move assignment operator.
     * @return This emitter.
     */
    emitter &operator=(emitter &&) = default;

    /**
     * @brief Publishes a given event.
     * @tparam Type Type of event to trigger.
     * @param value An instance of the given type of event.
     */
    template<typename Type>
    void publish(Type &&value) {
        if(auto &handler = assure<std::remove_cv_t<std::remove_reference_t<Type>>>(); handler) {
            handler(value, *static_cast<Derived *>(this));
        }
    }

    /**
     * @brief Registers a listener with the event emitter.
     * @tparam Type Type of event to which to connect the listener.
     * @param func The listener to register.
     */
    template<typename Type>
    void on(std::function<void(Type &, Derived &)> func) {
        assure<Type>() = std::move(func);
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
    void clear() ENTT_NOEXCEPT {
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
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return handlers.empty();
    }

private:
    dense_map<id_type, std::shared_ptr<void>, identity> handlers{};
};

} // namespace entt

#endif
