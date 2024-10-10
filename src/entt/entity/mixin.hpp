#ifndef ENTT_ENTITY_MIXIN_HPP
#define ENTT_ENTITY_MIXIN_HPP

#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../core/type_info.hpp"
#include "../signal/sigh.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename, typename, typename = void>
struct has_on_construct final: std::false_type {};

template<typename Type, typename Registry>
struct has_on_construct<Type, Registry, std::void_t<decltype(Type::on_construct(std::declval<Registry &>(), std::declval<Registry>().create()))>>
    : std::true_type {};

template<typename, typename, typename = void>
struct has_on_update final: std::false_type {};

template<typename Type, typename Registry>
struct has_on_update<Type, Registry, std::void_t<decltype(Type::on_update(std::declval<Registry &>(), std::declval<Registry>().create()))>>
    : std::true_type {};

template<typename, typename, typename = void>
struct has_on_destroy final: std::false_type {};

template<typename Type, typename Registry>
struct has_on_destroy<Type, Registry, std::void_t<decltype(Type::on_destroy(std::declval<Registry &>(), std::declval<Registry>().create()))>>
    : std::true_type {};

template<typename Type>
auto *any_to_owner(any &value) noexcept {
    using base_type = basic_registry<typename Type::entity_type, typename Type::allocator_type>;
    auto *reg = any_cast<base_type>(&value);

    if constexpr(!std::is_same_v<Type, base_type>) {
        if(!reg) {
            reg = any_cast<Type>(&value);
        }
    }

    return reg;
}

} // namespace internal
/*! @endcond */

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

    using basic_registry_type = basic_registry<typename owner_type::entity_type, typename owner_type::allocator_type>;
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
            if constexpr(std::is_same_v<typename underlying_type::element_type, entity_type>) {
                for(typename underlying_type::size_type pos{}, last = underlying_type::free_list(); pos < last; ++pos) {
                    destruction.publish(reg, underlying_type::base_type::operator[](pos));
                }
            } else {
                for(auto entt: static_cast<typename underlying_type::base_type &>(*this)) {
                    if constexpr(underlying_type::storage_policy == deletion_policy::in_place) {
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

    void bind_any(any value) noexcept final {
        owner = internal::any_to_owner<registry_type>(value);
        underlying_type::bind_any(std::move(value));
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
          update{allocator} {
        if constexpr(internal::has_on_construct<typename underlying_type::element_type, Registry>::value) {
            entt::sink{construction}.template connect<&underlying_type::element_type::on_construct>();
        }

        if constexpr(internal::has_on_update<typename underlying_type::element_type, Registry>::value) {
            entt::sink{update}.template connect<&underlying_type::element_type::on_update>();
        }

        if constexpr(internal::has_on_destroy<typename underlying_type::element_type, Registry>::value) {
            entt::sink{destruction}.template connect<&underlying_type::element_type::on_destroy>();
        }
    }

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
    ~basic_sigh_mixin() override = default;

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
        swap(other);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given storage.
     * @param other Storage to exchange the content with.
     */
    void swap(basic_sigh_mixin &other) noexcept {
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
     * @brief Checks if a mixin refers to a valid registry.
     * @return True if the mixin refers to a valid registry, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return (owner != nullptr);
    }

    /**
     * @brief Returns a pointer to the underlying registry, if any.
     * @return A pointer to the underlying registry, if any.
     */
    [[nodiscard]] const registry_type &registry() const noexcept {
        return owner_or_assert();
    }

    /*! @copydoc registry */
    [[nodiscard]] registry_type &registry() noexcept {
        return owner_or_assert();
    }

    /**
     * @brief Emplace elements into a storage.
     *
     * The behavior of this operation depends on the underlying storage type
     * (for example, components vs entities).<br/>
     * Refer to the specific documentation for more details.
     *
     * @return Whatever the underlying storage returns.
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
     * @return Whatever the underlying storage returns.
     */
    template<typename... Args>
    std::conditional_t<std::is_same_v<typename underlying_type::element_type, entity_type>, entity_type, decltype(std::declval<underlying_type>().get({}))>
    emplace(const entity_type hint, Args &&...args) {
        if constexpr(std::is_same_v<typename underlying_type::element_type, entity_type>) {
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

private:
    basic_registry_type *owner;
    sigh_type construction;
    sigh_type destruction;
    sigh_type update;
};

/**
 * @brief Mixin type used to add _reactive_ support to storage types.
 * @tparam Type Underlying storage type.
 * @tparam Registry Basic registry type.
 */
template<typename Type, typename Registry>
class basic_reactive_mixin final: public Type {
    using underlying_type = Type;
    using owner_type = Registry;

    using basic_registry_type = basic_registry<typename owner_type::entity_type, typename owner_type::allocator_type>;

    static_assert(std::is_base_of_v<basic_registry_type, owner_type>, "Invalid registry type");

    [[nodiscard]] auto &owner_or_assert() const noexcept {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        return static_cast<owner_type &>(*owner);
    }

    void emplace_element(const Registry &, typename underlying_type::entity_type entity) {
        if(!underlying_type::contains(entity)) {
            underlying_type::emplace(entity);
        }
    }

private:
    void bind_any(any value) noexcept final {
        owner = internal::any_to_owner<registry_type>(value);
        underlying_type::bind_any(std::move(value));
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = typename underlying_type::allocator_type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Expected registry type. */
    using registry_type = owner_type;

    /*! @brief Default constructor. */
    basic_reactive_mixin()
        : basic_reactive_mixin{allocator_type{}} {}

    /**
     * @brief Constructs an empty storage with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_reactive_mixin(const allocator_type &allocator)
        : underlying_type{allocator},
          owner{} {
    }

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_reactive_mixin(const basic_reactive_mixin &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_reactive_mixin(basic_reactive_mixin &&other) noexcept
        : underlying_type{std::move(other)},
          owner{other.owner} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_reactive_mixin(basic_reactive_mixin &&other, const allocator_type &allocator)
        : underlying_type{std::move(other), allocator},
          owner{other.owner} {}

    /*! @brief Default destructor. */
    ~basic_reactive_mixin() override = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This mixin.
     */
    basic_reactive_mixin &operator=(const basic_reactive_mixin &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This mixin.
     */
    basic_reactive_mixin &operator=(basic_reactive_mixin &&other) noexcept {
        swap(other);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given storage.
     * @param other Storage to exchange the content with.
     */
    void swap(basic_reactive_mixin &other) noexcept {
        using std::swap;
        swap(owner, other.owner);
        underlying_type::swap(other);
    }

    /**
     * @brief Makes storage _react_ to creation of objects of the given type.
     * @tparam Clazz Type of element to _react_ to.
     * @tparam Candidate Function to use to _react_ to the event.
     * @param id Optional name used to map the storage within the registry.
     * @return This mixin.
     */
    template<typename Clazz, auto Candidate = &basic_reactive_mixin::emplace_element>
    basic_reactive_mixin &on_construct(const id_type id = type_hash<Clazz>::value()) {
        owner_or_assert().template storage<Clazz>(id).on_construct().template connect<Candidate>(*this);
        return *this;
    }

    /**
     * @brief Makes storage _react_ to update of objects of the given type.
     * @tparam Clazz Type of element to _react_ to.
     * @tparam Candidate Function to use to _react_ to the event.
     * @param id Optional name used to map the storage within the registry.
     * @return This mixin.
     */
    template<typename Clazz, auto Candidate = &basic_reactive_mixin::emplace_element>
    basic_reactive_mixin &on_update(const id_type id = type_hash<Clazz>::value()) {
        owner_or_assert().template storage<Clazz>(id).on_update().template connect<Candidate>(*this);
        return *this;
    }

    /**
     * @brief Makes storage _react_ to destruction of objects of the given type.
     * @tparam Clazz Type of element to _react_ to.
     * @tparam Candidate Function to use to _react_ to the event.
     * @param id Optional name used to map the storage within the registry.
     * @return This mixin.
     */
    template<typename Clazz, auto Candidate = &basic_reactive_mixin::emplace_element>
    basic_reactive_mixin &on_destroy(const id_type id = type_hash<Clazz>::value()) {
        owner_or_assert().template storage<Clazz>(id).on_destroy().template connect<Candidate>(*this);
        return *this;
    }

    /**
     * @brief Checks if a mixin refers to a valid registry.
     * @return True if the mixin refers to a valid registry, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return (owner != nullptr);
    }

    /**
     * @brief Returns a pointer to the underlying registry, if any.
     * @return A pointer to the underlying registry, if any.
     */
    [[nodiscard]] const registry_type &registry() const noexcept {
        return owner_or_assert();
    }

    /*! @copydoc registry */
    [[nodiscard]] registry_type &registry() noexcept {
        return owner_or_assert();
    }

    /**
     * @brief Returns a view that is filtered by the underlying storage.
     * @tparam Get Types of elements used to construct the view.
     * @tparam Exclude Types of elements used to filter the view.
     * @return A newly created view.
     */
    template<typename... Get, typename... Exclude>
    [[nodiscard]] basic_view<get_t<const basic_reactive_mixin, typename basic_registry_type::template storage_for_type<const Get>...>, exclude_t<typename basic_registry_type::template storage_for_type<const Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) const {
        const owner_type &parent = owner_or_assert();
        basic_view<get_t<const basic_reactive_mixin, typename basic_registry_type::template storage_for_type<const Get>...>, exclude_t<typename basic_registry_type::template storage_for_type<const Exclude>...>> elem{};
        [&elem](const auto *...curr) { ((curr ? elem.storage(*curr) : void()), ...); }(parent.template storage<std::remove_const_t<Exclude>>()..., parent.template storage<std::remove_const_t<Get>>()..., this);
        return elem;
    }

    /*! @copydoc view */
    template<typename... Get, typename... Exclude>
    [[nodiscard]] basic_view<get_t<const basic_reactive_mixin, typename basic_registry_type::template storage_for_type<Get>...>, exclude_t<typename basic_registry_type::template storage_for_type<Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) {
        owner_type &parent = owner_or_assert();
        return {*this, parent.template storage<std::remove_const_t<Get>>()..., parent.template storage<std::remove_const_t<Exclude>>()...};
    }

private:
    basic_registry_type *owner;
};

} // namespace entt

#endif
