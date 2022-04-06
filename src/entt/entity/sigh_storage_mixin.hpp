#ifndef ENTT_ENTITY_SIGH_STORAGE_MIXIN_HPP
#define ENTT_ENTITY_SIGH_STORAGE_MIXIN_HPP

#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../signal/sigh.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @brief Mixin type used to add signal support to storage types.
 *
 * The function type of a listener is equivalent to:
 *
 * @code{.cpp}
 * void(basic_registry<entity_type> &, entity_type);
 * @endcode
 *
 * This applies to all signals made available.
 *
 * @tparam Type The type of the underlying storage.
 */
template<typename Type>
class sigh_storage_mixin final: public Type {
    using basic_iterator = typename Type::basic_iterator;

    template<typename Func>
    void notify_destruction(basic_iterator first, basic_iterator last, Func func) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");

        for(; first != last; ++first) {
            const auto entt = *first;
            destruction.publish(*owner, entt);
            const auto it = Type::find(entt);
            func(it, it + 1u);
        }
    }

    void swap_and_pop(basic_iterator first, basic_iterator last) final {
        notify_destruction(std::move(first), std::move(last), [this](auto... args) { Type::swap_and_pop(args...); });
    }

    void in_place_pop(basic_iterator first, basic_iterator last) final {
        notify_destruction(std::move(first), std::move(last), [this](auto... args) { Type::in_place_pop(args...); });
    }

    basic_iterator try_emplace(const typename Type::entity_type entt, const bool force_back, const void *value) final {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Type::try_emplace(entt, force_back, value);
        construction.publish(*owner, entt);
        return Type::find(entt);
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename Type::entity_type;

    /*! @brief Inherited constructors. */
    using Type::Type;

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever a new instance is created and assigned to an entity.<br/>
     * Listeners are invoked after the object has been assigned to the entity.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_construct() ENTT_NOEXCEPT {
        return sink{construction};
    }

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is explicitly updated.<br/>
     * Listeners are invoked after the object has been updated.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_update() ENTT_NOEXCEPT {
        return sink{update};
    }

    /**
     * @brief Returns a sink object.
     *
     * The sink returned by this function can be used to receive notifications
     * whenever an instance is removed from an entity and thus destroyed.<br/>
     * Listeners are invoked before the object has been removed from the entity.
     *
     * @sa sink
     *
     * @return A temporary sink object.
     */
    [[nodiscard]] auto on_destroy() ENTT_NOEXCEPT {
        return sink{destruction};
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam Args Types of arguments to use to construct the object.
     * @param entt A valid identifier.
     * @param args Parameters to use to initialize the object.
     * @return A reference to the newly created object.
     */
    template<typename... Args>
    decltype(auto) emplace(const entity_type entt, Args &&...args) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Type::emplace(entt, std::forward<Args>(args)...);
        construction.publish(*owner, entt);
        return this->get(entt);
    }

    /**
     * @brief Patches the given instance for an entity.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     * @return A reference to the patched instance.
     */
    template<typename... Func>
    decltype(auto) patch(const entity_type entt, Func &&...func) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Type::patch(entt, std::forward<Func>(func)...);
        update.publish(*owner, entt);
        return this->get(entt);
    }

    /**
     * @brief Assigns entities to a storage.
     * @tparam It Type of input iterator.
     * @tparam Args Types of arguments to use to construct the objects assigned
     * to the entities.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param args Parameters to use to initialize the objects assigned to the
     * entities.
     */
    template<typename It, typename... Args>
    void insert(It first, It last, Args &&...args) {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        Type::insert(first, last, std::forward<Args>(args)...);

        for(auto it = construction.empty() ? last : first; it != last; ++it) {
            construction.publish(*owner, *it);
        }
    }

    /**
     * @brief Forwards variables to mixins, if any.
     * @param value A variable wrapped in an opaque container.
     */
    void bind(any value) ENTT_NOEXCEPT final {
        auto *reg = any_cast<basic_registry<entity_type>>(&value);
        owner = reg ? reg : owner;
        Type::bind(std::move(value));
    }

private:
    sigh<void(basic_registry<entity_type> &, const entity_type)> construction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> destruction{};
    sigh<void(basic_registry<entity_type> &, const entity_type)> update{};
    basic_registry<entity_type> *owner{};
};

} // namespace entt

#endif
