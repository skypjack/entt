#ifndef ENTT_ENTITY_MIXIN_HPP
#define ENTT_ENTITY_MIXIN_HPP

#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../signal/sigh.hpp"
#include "component.hpp"
#include "entity.hpp"
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
 * @tparam Type Underlying storage type.
 * @tparam Registry Basic registry type.
 */
template<typename Type, typename Registry>
class basic_sigh_mixin final: public Type {
    using underlying_type = Type;
    using owner_type = Registry;

    using basic_registry_type = basic_registry<typename underlying_type::entity_type, typename underlying_type::base_type::allocator_type>;
    using sigh_type = sigh<void(owner_type &, const typename underlying_type::entity_type), typename underlying_type::allocator_type>;
    using underlying_iterator = typename underlying_type::base_type::basic_iterator;

    static_assert(std::is_base_of_v<basic_registry_type, owner_type>, "Invalid registry type");

    [[nodiscard]] auto &owner_or_assert() const noexcept {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        return static_cast<owner_type &>(*owner);
    }

private:
    void pop(underlying_iterator first, underlying_iterator last) final {
        if(auto &reg = owner_or_assert(); destruction.empty()) {
            underlying_type::pop(first, last);
        } else {
            for(; first != last; ++first) {
                const auto entt = *first;
                destruction.publish(reg, entt);
                const auto it = underlying_type::find(entt);
                underlying_type::pop(it, it + 1u);
            }
        }
    }

    void pop_all() final {
        if(auto &reg = owner_or_assert(); !destruction.empty()) {
            if constexpr(std::is_same_v<typename underlying_type::element_type, typename underlying_type::entity_type>) {
                for(typename Type::size_type pos{}, last = Type::free_list(); pos < last; ++pos) {
                    destruction.publish(reg, underlying_type::base_type::operator[](pos));
                }
            } else {
                for(auto entt: static_cast<typename underlying_type::base_type &>(*this)) {
                    if constexpr(component_traits<typename underlying_type::element_type>::in_place_delete) {
                        if(entt != tombstone) {
                            destruction.publish(reg, entt);
                        }
                    } else {
                        destruction.publish(reg, entt);
                    }
                }
            }
        }

        underlying_type::pop_all();
    }

    underlying_iterator try_emplace(const typename underlying_type::entity_type entt, const bool force_back, const void *value) final {
        const auto it = underlying_type::try_emplace(entt, force_back, value);

        if(auto &reg = owner_or_assert(); it != underlying_type::base_type::end()) {
            construction.publish(reg, *it);
        }

        return it;
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = typename underlying_type::allocator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Expected registry type. */
    using registry_type = owner_type;

    /*! @brief Default constructor. */
    basic_sigh_mixin()
        : basic_sigh_mixin{allocator_type{}} {}

    /**
     * @brief Constructs an empty storage with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_sigh_mixin(const allocator_type &allocator)
        : underlying_type{allocator},
          owner{},
          construction{allocator},
          destruction{allocator},
          update{allocator} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_sigh_mixin(const basic_sigh_mixin &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_sigh_mixin(basic_sigh_mixin &&other) noexcept
        : underlying_type{std::move(other)},
          owner{other.owner},
          construction{std::move(other.construction)},
          destruction{std::move(other.destruction)},
          update{std::move(other.update)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_sigh_mixin(basic_sigh_mixin &&other, const allocator_type &allocator)
        : underlying_type{std::move(other), allocator},
          owner{other.owner},
          construction{std::move(other.construction), allocator},
          destruction{std::move(other.destruction), allocator},
          update{std::move(other.update), allocator} {}

    /*! @brief Default destructor. */
    ~basic_sigh_mixin() noexcept override = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This mixin.
     */
    basic_sigh_mixin &operator=(const basic_sigh_mixin &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This mixin.
     */
    basic_sigh_mixin &operator=(basic_sigh_mixin &&other) noexcept {
        owner = other.owner;
        construction = std::move(other.construction);
        destruction = std::move(other.destruction);
        update = std::move(other.update);
        underlying_type::operator=(std::move(other));
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given storage.
     * @param other Storage to exchange the content with.
     */
    void swap(basic_sigh_mixin &other) {
        using std::swap;
        swap(owner, other.owner);
        swap(construction, other.construction);
        swap(destruction, other.destruction);
        swap(update, other.update);
        underlying_type::swap(other);
    }

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
    [[nodiscard]] auto on_construct() noexcept {
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
    [[nodiscard]] auto on_update() noexcept {
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
    [[nodiscard]] auto on_destroy() noexcept {
        return sink{destruction};
    }

    /**
     * @brief Emplace elements into a storage.
     *
     * The behavior of this operation depends on the underlying storage type
     * (for example, components vs entities).<br/>
     * Refer to the specific documentation for more details.
     *
     * @return A return value as returned by the underlying storage.
     */
    auto emplace() {
        const auto entt = underlying_type::emplace();
        construction.publish(owner_or_assert(), entt);
        return entt;
    }

    /**
     * @brief Emplace elements into a storage.
     *
     * The behavior of this operation depends on the underlying storage type
     * (for example, components vs entities).<br/>
     * Refer to the specific documentation for more details.
     *
     * @tparam Args Types of arguments to forward to the underlying storage.
     * @param hint A valid identifier.
     * @param args Parameters to forward to the underlying storage.
     * @return A return value as returned by the underlying storage.
     */
    template<typename... Args>
    decltype(auto) emplace(const entity_type hint, Args &&...args) {
        if constexpr(std::is_same_v<typename underlying_type::element_type, typename underlying_type::entity_type>) {
            const auto entt = underlying_type::emplace(hint, std::forward<Args>(args)...);
            construction.publish(owner_or_assert(), entt);
            return entt;
        } else {
            underlying_type::emplace(hint, std::forward<Args>(args)...);
            construction.publish(owner_or_assert(), hint);
            return this->get(hint);
        }
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
        underlying_type::patch(entt, std::forward<Func>(func)...);
        update.publish(owner_or_assert(), entt);
        return this->get(entt);
    }

    /**
     * @brief Emplace elements into a storage.
     *
     * The behavior of this operation depends on the underlying storage type
     * (for example, components vs entities).<br/>
     * Refer to the specific documentation for more details.
     *
     * @tparam It Iterator type (as required by the underlying storage type).
     * @tparam Args Types of arguments to forward to the underlying storage.
     * @param first An iterator to the first element of the range.
     * @param last An iterator past the last element of the range.
     * @param args Parameters to use to forward to the underlying storage.
     */
    template<typename It, typename... Args>
    void insert(It first, It last, Args &&...args) {
        auto from = underlying_type::size();
        underlying_type::insert(first, last, std::forward<Args>(args)...);

        if(auto &reg = owner_or_assert(); !construction.empty()) {
            for(const auto to = underlying_type::size(); from != to; ++from) {
                construction.publish(reg, underlying_type::operator[](from));
            }
        }
    }

    /**
     * @brief Forwards variables to derived classes, if any.
     * @param value A variable wrapped in an opaque container.
     */
    void bind(any value) noexcept final {
        auto *reg = any_cast<basic_registry_type>(&value);
        owner = reg ? reg : owner;
        underlying_type::bind(std::move(value));
    }

private:
    basic_registry_type *owner;
    sigh_type construction;
    sigh_type destruction;
    sigh_type update;
};

} // namespace entt

#endif
