#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../core/algorithm.hpp"
#include "../core/any.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/memory.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "group.hpp"
#include "mixin.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"
#include "view.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename It>
class registry_storage_iterator final {
    template<typename Other>
    friend class registry_storage_iterator;

    using mapped_type = std::remove_reference_t<decltype(std::declval<It>()->second)>;

public:
    using value_type = std::pair<id_type, constness_as_t<typename mapped_type::element_type, mapped_type> &>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;

    constexpr registry_storage_iterator() noexcept
        : it{} {}

    constexpr registry_storage_iterator(It iter) noexcept
        : it{iter} {}

    template<typename Other, typename = std::enable_if_t<!std::is_same_v<It, Other> && std::is_constructible_v<It, Other>>>
    constexpr registry_storage_iterator(const registry_storage_iterator<Other> &other) noexcept
        : registry_storage_iterator{other.it} {}

    constexpr registry_storage_iterator &operator++() noexcept {
        return ++it, *this;
    }

    constexpr registry_storage_iterator operator++(int) noexcept {
        const registry_storage_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr registry_storage_iterator &operator--() noexcept {
        return --it, *this;
    }

    constexpr registry_storage_iterator operator--(int) noexcept {
        const registry_storage_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr registry_storage_iterator &operator+=(const difference_type value) noexcept {
        it += value;
        return *this;
    }

    constexpr registry_storage_iterator operator+(const difference_type value) const noexcept {
        registry_storage_iterator copy = *this;
        return (copy += value);
    }

    constexpr registry_storage_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr registry_storage_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return {it[value].first, *it[value].second};
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    template<typename Lhs, typename Rhs>
    friend constexpr std::ptrdiff_t operator-(const registry_storage_iterator<Lhs> &, const registry_storage_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator==(const registry_storage_iterator<Lhs> &, const registry_storage_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator<(const registry_storage_iterator<Lhs> &, const registry_storage_iterator<Rhs> &) noexcept;

private:
    It it;
};

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return lhs.it - rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator==(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator!=(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return lhs.it < rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return rhs < lhs;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<=(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>=(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs < rhs);
}

template<typename Allocator>
class registry_context {
    using alloc_traits = std::allocator_traits<Allocator>;
    using allocator_type = typename alloc_traits::template rebind_alloc<std::pair<const id_type, basic_any<0u>>>;

public:
    explicit registry_context(const allocator_type &allocator)
        : ctx{allocator} {}

    template<typename Type, typename... Args>
    Type &emplace_as(const id_type id, Args &&...args) {
        return any_cast<Type &>(ctx.try_emplace(id, std::in_place_type<Type>, std::forward<Args>(args)...).first->second);
    }

    template<typename Type, typename... Args>
    Type &emplace(Args &&...args) {
        return emplace_as<Type>(type_id<Type>().hash(), std::forward<Args>(args)...);
    }

    template<typename Type>
    Type &insert_or_assign(const id_type id, Type &&value) {
        return any_cast<std::remove_cv_t<std::remove_reference_t<Type>> &>(ctx.insert_or_assign(id, std::forward<Type>(value)).first->second);
    }

    template<typename Type>
    Type &insert_or_assign(Type &&value) {
        return insert_or_assign(type_id<Type>().hash(), std::forward<Type>(value));
    }

    template<typename Type>
    bool erase(const id_type id = type_id<Type>().hash()) {
        const auto it = ctx.find(id);
        return it != ctx.end() && it->second.type() == type_id<Type>() ? (ctx.erase(it), true) : false;
    }

    template<typename Type>
    [[nodiscard]] const Type &get(const id_type id = type_id<Type>().hash()) const {
        return any_cast<const Type &>(ctx.at(id));
    }

    template<typename Type>
    [[nodiscard]] Type &get(const id_type id = type_id<Type>().hash()) {
        return any_cast<Type &>(ctx.at(id));
    }

    template<typename Type>
    [[nodiscard]] const Type *find(const id_type id = type_id<Type>().hash()) const {
        const auto it = ctx.find(id);
        return it != ctx.cend() ? any_cast<const Type>(&it->second) : nullptr;
    }

    template<typename Type>
    [[nodiscard]] Type *find(const id_type id = type_id<Type>().hash()) {
        const auto it = ctx.find(id);
        return it != ctx.end() ? any_cast<Type>(&it->second) : nullptr;
    }

    template<typename Type>
    [[nodiscard]] bool contains(const id_type id = type_id<Type>().hash()) const {
        const auto it = ctx.find(id);
        return it != ctx.cend() && it->second.type() == type_id<Type>();
    }

private:
    dense_map<id_type, basic_any<0u>, identity, std::equal_to<>, allocator_type> ctx;
};

} // namespace internal
/*! @endcond */

/**
 * @brief Fast and reliable entity-component system.
 * @tparam Entity A valid entity type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Entity, typename Allocator>
class basic_registry {
    using base_type = basic_sparse_set<Entity, Allocator>;
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Entity>, "Invalid value type");
    // std::shared_ptr because of its type erased allocator which is useful here
    using pool_container_type = dense_map<id_type, std::shared_ptr<base_type>, identity, std::equal_to<>, typename alloc_traits::template rebind_alloc<std::pair<const id_type, std::shared_ptr<base_type>>>>;
    using group_container_type = dense_map<id_type, std::shared_ptr<internal::group_descriptor>, identity, std::equal_to<>, typename alloc_traits::template rebind_alloc<std::pair<const id_type, std::shared_ptr<internal::group_descriptor>>>>;
    using traits_type = entt_traits<Entity>;

    template<typename Type>
    [[nodiscard]] auto &assure([[maybe_unused]] const id_type id = type_hash<Type>::value()) {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Non-decayed types not allowed");

        if constexpr(std::is_same_v<Type, entity_type>) {
            ENTT_ASSERT(id == type_hash<Type>::value(), "User entity storage not allowed");
            return entities;
        } else {
            using storage_type = storage_for_type<Type>;

            if(auto it = pools.find(id); it != pools.cend()) {
                ENTT_ASSERT(it->second->type() == type_id<Type>(), "Unexpected type");
                return static_cast<storage_type &>(*it->second);
            }

            using alloc_type = typename storage_type::allocator_type;
            typename pool_container_type::mapped_type cpool{};

            if constexpr(std::is_void_v<Type> && !std::is_constructible_v<alloc_type, allocator_type>) {
                // std::allocator<void> has no cross constructors (waiting for C++20)
                cpool = std::allocate_shared<storage_type>(get_allocator(), alloc_type{});
            } else {
                cpool = std::allocate_shared<storage_type>(get_allocator(), get_allocator());
            }

            pools.emplace(id, cpool);
            cpool->bind(*this);

            return static_cast<storage_type &>(*cpool);
        }
    }

    template<typename Type>
    [[nodiscard]] const auto *assure([[maybe_unused]] const id_type id = type_hash<Type>::value()) const {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Non-decayed types not allowed");

        if constexpr(std::is_same_v<Type, entity_type>) {
            ENTT_ASSERT(id == type_hash<Type>::value(), "User entity storage not allowed");
            return &entities;
        } else {
            if(const auto it = pools.find(id); it != pools.cend()) {
                ENTT_ASSERT(it->second->type() == type_id<Type>(), "Unexpected type");
                return static_cast<const storage_for_type<Type> *>(it->second.get());
            }

            return static_cast<const storage_for_type<Type> *>(nullptr);
        }
    }

    void rebind() {
        entities.bind(*this);

        for(auto &&curr: pools) {
            curr.second->bind(*this);
        }
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename traits_type::value_type;
    /*! @brief Underlying version type. */
    using version_type = typename traits_type::version_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Common type among all storage types. */
    using common_type = base_type;
    /*! @brief Context type. */
    using context = internal::registry_context<allocator_type>;
    /*! @brief Iterable registry type. */
    using iterable = iterable_adaptor<internal::registry_storage_iterator<typename pool_container_type::iterator>>;
    /*! @brief Constant iterable registry type. */
    using const_iterable = iterable_adaptor<internal::registry_storage_iterator<typename pool_container_type::const_iterator>>;

    /**
     * @copybrief storage_for
     * @tparam Type Storage value type, eventually const.
     */
    template<typename Type>
    using storage_for_type = typename storage_for<Type, Entity, typename alloc_traits::template rebind_alloc<std::remove_const_t<Type>>>::type;

    /*! @brief Default constructor. */
    basic_registry()
        : basic_registry{allocator_type{}} {}

    /**
     * @brief Constructs an empty registry with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_registry(const allocator_type &allocator)
        : basic_registry{0u, allocator} {}

    /**
     * @brief Allocates enough memory upon construction to store `count` pools.
     * @param count The number of pools to allocate memory for.
     * @param allocator The allocator to use.
     */
    basic_registry(const size_type count, const allocator_type &allocator = allocator_type{})
        : vars{allocator},
          pools{allocator},
          groups{allocator},
          entities{allocator} {
        pools.reserve(count);
        rebind();
    }

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_registry(const basic_registry &) = delete;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_registry(basic_registry &&other) noexcept
        : vars{std::move(other.vars)},
          pools{std::move(other.pools)},
          groups{std::move(other.groups)},
          entities{std::move(other.entities)} {
        rebind();
    }

    /*! @brief Default destructor. */
    ~basic_registry() = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This mixin.
     */
    basic_registry &operator=(const basic_registry &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This registry.
     */
    basic_registry &operator=(basic_registry &&other) noexcept {
        swap(other);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given registry.
     * @param other Registry to exchange the content with.
     */
    void swap(basic_registry &other) noexcept {
        using std::swap;

        swap(vars, other.vars);
        swap(pools, other.pools);
        swap(groups, other.groups);
        swap(entities, other.entities);

        rebind();
        other.rebind();
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return entities.get_allocator();
    }

    /**
     * @brief Returns an iterable object to use to _visit_ a registry.
     *
     * The iterable object returns a pair that contains the name and a reference
     * to the current storage.
     *
     * @return An iterable object to use to _visit_ the registry.
     */
    [[nodiscard]] iterable storage() noexcept {
        return iterable{pools.begin(), pools.end()};
    }

    /*! @copydoc storage */
    [[nodiscard]] const_iterable storage() const noexcept {
        return const_iterable{pools.cbegin(), pools.cend()};
    }

    /**
     * @brief Finds the storage associated with a given name, if any.
     * @param id Name used to map the storage within the registry.
     * @return A pointer to the storage if it exists, a null pointer otherwise.
     */
    [[nodiscard]] common_type *storage(const id_type id) {
        return const_cast<common_type *>(std::as_const(*this).storage(id));
    }

    /**
     * @brief Finds the storage associated with a given name, if any.
     * @param id Name used to map the storage within the registry.
     * @return A pointer to the storage if it exists, a null pointer otherwise.
     */
    [[nodiscard]] const common_type *storage(const id_type id) const {
        const auto it = pools.find(id);
        return it == pools.cend() ? nullptr : it->second.get();
    }

    /**
     * @brief Returns the storage for a given element type.
     * @tparam Type Type of element of which to return the storage.
     * @param id Optional name used to map the storage within the registry.
     * @return The storage for the given element type.
     */
    template<typename Type>
    storage_for_type<Type> &storage(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id);
    }

    /**
     * @brief Returns the storage for a given element type, if any.
     * @tparam Type Type of element of which to return the storage.
     * @param id Optional name used to map the storage within the registry.
     * @return The storage for the given element type.
     */
    template<typename Type>
    [[nodiscard]] const storage_for_type<Type> *storage(const id_type id = type_hash<Type>::value()) const {
        return assure<Type>(id);
    }

    /**
     * @brief Discards the storage associated with a given name, if any.
     * @param id Name used to map the storage within the registry.
     * @return True in case of success, false otherwise.
     */
    bool reset(const id_type id) {
        ENTT_ASSERT(id != type_hash<entity_type>::value(), "Cannot reset entity storage");
        return !(pools.erase(id) == 0u);
    }

    /**
     * @brief Checks if an identifier refers to a valid entity.
     * @param entt An identifier, either valid or not.
     * @return True if the identifier is valid, false otherwise.
     */
    [[nodiscard]] bool valid(const entity_type entt) const {
        return static_cast<size_type>(entities.find(entt).index()) < entities.free_list();
    }

    /**
     * @brief Returns the actual version for an identifier.
     * @param entt A valid identifier.
     * @return The version for the given identifier if valid, the tombstone
     * version otherwise.
     */
    [[nodiscard]] version_type current(const entity_type entt) const {
        return entities.current(entt);
    }

    /**
     * @brief Creates a new entity or recycles a destroyed one.
     * @return A valid identifier.
     */
    [[nodiscard]] entity_type create() {
        return entities.generate();
    }

    /**
     * @copybrief create
     *
     * If the requested entity isn't in use, the suggested identifier is used.
     * Otherwise, a new identifier is generated.
     *
     * @param hint Required identifier.
     * @return A valid identifier.
     */
    [[nodiscard]] entity_type create(const entity_type hint) {
        return entities.generate(hint);
    }

    /**
     * @brief Assigns each element in a range an identifier.
     *
     * @sa create
     *
     * @tparam It Type of forward iterator.
     * @param first An iterator to the first element of the range to generate.
     * @param last An iterator past the last element of the range to generate.
     */
    template<typename It>
    void create(It first, It last) {
        entities.generate(std::move(first), std::move(last));
    }

    /**
     * @brief Destroys an entity and releases its identifier.
     *
     * @warning
     * Adding or removing elements to an entity that is being destroyed can
     * result in undefined behavior.
     *
     * @param entt A valid identifier.
     * @return The version of the recycled entity.
     */
    version_type destroy(const entity_type entt) {
        for(size_type pos = pools.size(); pos != 0u; --pos) {
            pools.begin()[pos - 1u].second->remove(entt);
        }

        entities.erase(entt);
        return entities.current(entt);
    }

    /**
     * @brief Destroys an entity and releases its identifier.
     *
     * The suggested version or the valid version closest to the suggested one
     * is used instead of the implicitly generated version.
     *
     * @sa destroy
     *
     * @param entt A valid identifier.
     * @param version A desired version upon destruction.
     * @return The version actually assigned to the entity.
     */
    version_type destroy(const entity_type entt, const version_type version) {
        destroy(entt);
        const auto elem = traits_type::construct(traits_type::to_entity(entt), version);
        return entities.bump((elem == tombstone) ? traits_type::next(elem) : elem);
    }

    /**
     * @brief Destroys all entities in a range and releases their identifiers.
     *
     * @sa destroy
     *
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename It>
    void destroy(It first, It last) {
        const auto to = entities.sort_as(first, last);
        const auto from = entities.cend() - entities.free_list();

        for(auto &&curr: pools) {
            curr.second->remove(from, to);
        }

        entities.erase(from, to);
    }

    /**
     * @brief Assigns the given element to an entity.
     *
     * The element must have a proper constructor or be of aggregate type.
     *
     * @warning
     * Attempting to assign an element to an entity that already owns it results
     * in undefined behavior.
     *
     * @tparam Type Type of element to create.
     * @tparam Args Types of arguments to use to construct the element.
     * @param entt A valid identifier.
     * @param args Parameters to use to initialize the element.
     * @return A reference to the newly created element.
     */
    template<typename Type, typename... Args>
    decltype(auto) emplace(const entity_type entt, Args &&...args) {
        ENTT_ASSERT(valid(entt), "Invalid entity");
        return assure<Type>().emplace(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Assigns each entity in a range the given element.
     *
     * @sa emplace
     *
     * @tparam Type Type of element to create.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param value An instance of the element to assign.
     */
    template<typename Type, typename It>
    void insert(It first, It last, const Type &value = {}) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entt) { return valid(entt); }), "Invalid entity");
        assure<Type>().insert(std::move(first), std::move(last), value);
    }

    /**
     * @brief Assigns each entity in a range the given elements.
     *
     * @sa emplace
     *
     * @tparam Type Type of element to create.
     * @tparam EIt Type of input iterator.
     * @tparam CIt Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @param from An iterator to the first element of the range of elements.
     */
    template<typename Type, typename EIt, typename CIt, typename = std::enable_if_t<std::is_same_v<typename std::iterator_traits<CIt>::value_type, Type>>>
    void insert(EIt first, EIt last, CIt from) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entt) { return valid(entt); }), "Invalid entity");
        assure<Type>().insert(first, last, from);
    }

    /**
     * @brief Assigns or replaces the given element for an entity.
     *
     * @sa emplace
     * @sa replace
     *
     * @tparam Type Type of element to assign or replace.
     * @tparam Args Types of arguments to use to construct the element.
     * @param entt A valid identifier.
     * @param args Parameters to use to initialize the element.
     * @return A reference to the newly created element.
     */
    template<typename Type, typename... Args>
    decltype(auto) emplace_or_replace(const entity_type entt, Args &&...args) {
        auto &cpool = assure<Type>();
        ENTT_ASSERT(valid(entt), "Invalid entity");
        return cpool.contains(entt) ? cpool.patch(entt, [&args...](auto &...curr) { ((curr = Type{std::forward<Args>(args)...}), ...); }) : cpool.emplace(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Patches the given element for an entity.
     *
     * The signature of the function should be equivalent to the following:
     *
     * @code{.cpp}
     * void(Type &);
     * @endcode
     *
     * @warning
     * Attempting to patch an element of an entity that doesn't own it results
     * in undefined behavior.
     *
     * @tparam Type Type of element to patch.
     * @tparam Func Types of the function objects to invoke.
     * @param entt A valid identifier.
     * @param func Valid function objects.
     * @return A reference to the patched element.
     */
    template<typename Type, typename... Func>
    decltype(auto) patch(const entity_type entt, Func &&...func) {
        return assure<Type>().patch(entt, std::forward<Func>(func)...);
    }

    /**
     * @brief Replaces the given element for an entity.
     *
     * The element must have a proper constructor or be of aggregate type.
     *
     * @warning
     * Attempting to replace an element of an entity that doesn't own it results
     * in undefined behavior.
     *
     * @tparam Type Type of element to replace.
     * @tparam Args Types of arguments to use to construct the element.
     * @param entt A valid identifier.
     * @param args Parameters to use to initialize the element.
     * @return A reference to the element being replaced.
     */
    template<typename Type, typename... Args>
    decltype(auto) replace(const entity_type entt, Args &&...args) {
        return patch<Type>(entt, [&args...](auto &...curr) { ((curr = Type{std::forward<Args>(args)...}), ...); });
    }

    /**
     * @brief Removes the given elements from an entity.
     * @tparam Type Type of element to remove.
     * @tparam Other Other types of elements to remove.
     * @param entt A valid identifier.
     * @return The number of elements actually removed.
     */
    template<typename Type, typename... Other>
    size_type remove(const entity_type entt) {
        return (assure<Type>().remove(entt) + ... + assure<Other>().remove(entt));
    }

    /**
     * @brief Removes the given elements from all the entities in a range.
     *
     * @sa remove
     *
     * @tparam Type Type of element to remove.
     * @tparam Other Other types of elements to remove.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     * @return The number of elements actually removed.
     */
    template<typename Type, typename... Other, typename It>
    size_type remove(It first, It last) {
        size_type count{};

        if constexpr(std::is_same_v<It, typename common_type::iterator>) {
            std::array cpools{static_cast<common_type *>(&assure<Type>()), static_cast<common_type *>(&assure<Other>())...};

            for(auto from = cpools.begin(), to = cpools.end(); from != to; ++from) {
                if constexpr(sizeof...(Other) != 0u) {
                    if((*from)->data() == first.data()) {
                        std::swap((*from), cpools.back());
                    }
                }

                count += (*from)->remove(first, last);
            }

        } else {
            for(auto cpools = std::forward_as_tuple(assure<Type>(), assure<Other>()...); first != last; ++first) {
                count += std::apply([entt = *first](auto &...curr) { return (curr.remove(entt) + ... + 0u); }, cpools);
            }
        }

        return count;
    }

    /**
     * @brief Erases the given elements from an entity.
     *
     * @warning
     * Attempting to erase an element from an entity that doesn't own it results
     * in undefined behavior.
     *
     * @tparam Type Types of elements to erase.
     * @tparam Other Other types of elements to erase.
     * @param entt A valid identifier.
     */
    template<typename Type, typename... Other>
    void erase(const entity_type entt) {
        (assure<Type>().erase(entt), (assure<Other>().erase(entt), ...));
    }

    /**
     * @brief Erases the given elements from all the entities in a range.
     *
     * @sa erase
     *
     * @tparam Type Types of elements to erase.
     * @tparam Other Other types of elements to erase.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of entities.
     * @param last An iterator past the last element of the range of entities.
     */
    template<typename Type, typename... Other, typename It>
    void erase(It first, It last) {
        if constexpr(std::is_same_v<It, typename common_type::iterator>) {
            std::array cpools{static_cast<common_type *>(&assure<Type>()), static_cast<common_type *>(&assure<Other>())...};

            for(auto from = cpools.begin(), to = cpools.end(); from != to; ++from) {
                if constexpr(sizeof...(Other) != 0u) {
                    if((*from)->data() == first.data()) {
                        std::swap(*from, cpools.back());
                    }
                }

                (*from)->erase(first, last);
            }
        } else {
            for(auto cpools = std::forward_as_tuple(assure<Type>(), assure<Other>()...); first != last; ++first) {
                std::apply([entt = *first](auto &...curr) { (curr.erase(entt), ...); }, cpools);
            }
        }
    }

    /**
     * @brief Erases elements satisfying specific criteria from an entity.
     *
     * The function type is equivalent to:
     *
     * @code{.cpp}
     * void(const id_type, typename basic_registry<Entity>::common_type &);
     * @endcode
     *
     * Only storage where the entity exists are passed to the function.
     *
     * @tparam Func Type of the function object to invoke.
     * @param entt A valid identifier.
     * @param func A valid function object.
     */
    template<typename Func>
    void erase_if(const entity_type entt, Func func) {
        for(auto [id, cpool]: storage()) {
            if(cpool.contains(entt) && func(id, std::as_const(cpool))) {
                cpool.erase(entt);
            }
        }
    }

    /**
     * @brief Removes all tombstones from a registry or only the pools for the
     * given elements.
     * @tparam Type Types of elements for which to clear all tombstones.
     */
    template<typename... Type>
    void compact() {
        if constexpr(sizeof...(Type) == 0u) {
            for(auto &&curr: pools) {
                curr.second->compact();
            }
        } else {
            (assure<Type>().compact(), ...);
        }
    }

    /**
     * @brief Check if an entity is part of all the given storage.
     * @tparam Type Type of storage to check for.
     * @param entt A valid identifier.
     * @return True if the entity is part of all the storage, false otherwise.
     */
    template<typename... Type>
    [[nodiscard]] bool all_of([[maybe_unused]] const entity_type entt) const {
        if constexpr(sizeof...(Type) == 1u) {
            auto *cpool = assure<std::remove_const_t<Type>...>();
            return cpool && cpool->contains(entt);
        } else {
            return (all_of<Type>(entt) && ...);
        }
    }

    /**
     * @brief Check if an entity is part of at least one given storage.
     * @tparam Type Type of storage to check for.
     * @param entt A valid identifier.
     * @return True if the entity is part of at least one storage, false
     * otherwise.
     */
    template<typename... Type>
    [[nodiscard]] bool any_of([[maybe_unused]] const entity_type entt) const {
        return (all_of<Type>(entt) || ...);
    }

    /**
     * @brief Returns references to the given elements for an entity.
     *
     * @warning
     * Attempting to get an element from an entity that doesn't own it results
     * in undefined behavior.
     *
     * @tparam Type Types of elements to get.
     * @param entt A valid identifier.
     * @return References to the elements owned by the entity.
     */
    template<typename... Type>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        if constexpr(sizeof...(Type) == 1u) {
            return (assure<std::remove_const_t<Type>>()->get(entt), ...);
        } else {
            return std::forward_as_tuple(get<Type>(entt)...);
        }
    }

    /*! @copydoc get */
    template<typename... Type>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) {
        if constexpr(sizeof...(Type) == 1u) {
            return (static_cast<storage_for_type<Type> &>(assure<std::remove_const_t<Type>>()).get(entt), ...);
        } else {
            return std::forward_as_tuple(get<Type>(entt)...);
        }
    }

    /**
     * @brief Returns a reference to the given element for an entity.
     *
     * In case the entity doesn't own the element, the parameters provided are
     * used to construct it.
     *
     * @sa get
     * @sa emplace
     *
     * @tparam Type Type of element to get.
     * @tparam Args Types of arguments to use to construct the element.
     * @param entt A valid identifier.
     * @param args Parameters to use to initialize the element.
     * @return Reference to the element owned by the entity.
     */
    template<typename Type, typename... Args>
    [[nodiscard]] decltype(auto) get_or_emplace(const entity_type entt, Args &&...args) {
        auto &cpool = assure<Type>();
        ENTT_ASSERT(valid(entt), "Invalid entity");
        return cpool.contains(entt) ? cpool.get(entt) : cpool.emplace(entt, std::forward<Args>(args)...);
    }

    /**
     * @brief Returns pointers to the given elements for an entity.
     *
     * @note
     * The registry retains ownership of the pointed-to elements.
     *
     * @tparam Type Types of elements to get.
     * @param entt A valid identifier.
     * @return Pointers to the elements owned by the entity.
     */
    template<typename... Type>
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entt) const {
        if constexpr(sizeof...(Type) == 1u) {
            const auto *cpool = assure<std::remove_const_t<Type>...>();
            return (cpool && cpool->contains(entt)) ? std::addressof(cpool->get(entt)) : nullptr;
        } else {
            return std::make_tuple(try_get<Type>(entt)...);
        }
    }

    /*! @copydoc try_get */
    template<typename... Type>
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entt) {
        if constexpr(sizeof...(Type) == 1u) {
            return (const_cast<Type *>(std::as_const(*this).template try_get<Type>(entt)), ...);
        } else {
            return std::make_tuple(try_get<Type>(entt)...);
        }
    }

    /**
     * @brief Clears a whole registry or the pools for the given elements.
     * @tparam Type Types of elements to remove from their entities.
     */
    template<typename... Type>
    void clear() {
        if constexpr(sizeof...(Type) == 0u) {
            for(size_type pos = pools.size(); pos; --pos) {
                pools.begin()[pos - 1u].second->clear();
            }

            const auto elem = entities.each();
            entities.erase(elem.begin().base(), elem.end().base());
        } else {
            (assure<Type>().clear(), ...);
        }
    }

    /**
     * @brief Checks if an entity has elements assigned.
     * @param entt A valid identifier.
     * @return True if the entity has no elements assigned, false otherwise.
     */
    [[nodiscard]] bool orphan(const entity_type entt) const {
        return std::none_of(pools.cbegin(), pools.cend(), [entt](auto &&curr) { return curr.second->contains(entt); });
    }

    /**
     * @brief Returns a sink object for the given element.
     *
     * Use this function to receive notifications whenever a new instance of the
     * given element is created and assigned to an entity.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** assigning the element to the entity.
     *
     * @sa sink
     *
     * @tparam Type Type of element of which to get the sink.
     * @param id Optional name used to map the storage within the registry.
     * @return A temporary sink object.
     */
    template<typename Type>
    [[nodiscard]] auto on_construct(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id).on_construct();
    }

    /**
     * @brief Returns a sink object for the given element.
     *
     * Use this function to receive notifications whenever an instance of the
     * given element is explicitly updated.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **after** updating the element.
     *
     * @sa sink
     *
     * @tparam Type Type of element of which to get the sink.
     * @param id Optional name used to map the storage within the registry.
     * @return A temporary sink object.
     */
    template<typename Type>
    [[nodiscard]] auto on_update(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id).on_update();
    }

    /**
     * @brief Returns a sink object for the given element.
     *
     * Use this function to receive notifications whenever an instance of the
     * given element is removed from an entity and thus destroyed.<br/>
     * The function type for a listener is equivalent to:
     *
     * @code{.cpp}
     * void(basic_registry<Entity> &, Entity);
     * @endcode
     *
     * Listeners are invoked **before** removing the element from the entity.
     *
     * @sa sink
     *
     * @tparam Type Type of element of which to get the sink.
     * @param id Optional name used to map the storage within the registry.
     * @return A temporary sink object.
     */
    template<typename Type>
    [[nodiscard]] auto on_destroy(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id).on_destroy();
    }

    /**
     * @brief Returns a view for the given elements.
     * @tparam Type Type of element used to construct the view.
     * @tparam Other Other types of elements used to construct the view.
     * @tparam Exclude Types of elements used to filter the view.
     * @return A newly created view.
     */
    template<typename Type, typename... Other, typename... Exclude>
    [[nodiscard]] basic_view<get_t<storage_for_type<const Type>, storage_for_type<const Other>...>, exclude_t<storage_for_type<const Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) const {
        basic_view<get_t<storage_for_type<const Type>, storage_for_type<const Other>...>, exclude_t<storage_for_type<const Exclude>...>> elem{};
        [&elem](const auto *...curr) { ((curr ? elem.storage(*curr) : void()), ...); }(assure<std::remove_const_t<Exclude>>()..., assure<std::remove_const_t<Other>>()..., assure<std::remove_const_t<Type>>());
        return elem;
    }

    /*! @copydoc view */
    template<typename Type, typename... Other, typename... Exclude>
    [[nodiscard]] basic_view<get_t<storage_for_type<Type>, storage_for_type<Other>...>, exclude_t<storage_for_type<Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) {
        return {assure<std::remove_const_t<Type>>(), assure<std::remove_const_t<Other>>()..., assure<std::remove_const_t<Exclude>>()...};
    }

    /**
     * @brief Returns a group for the given elements.
     * @tparam Owned Types of storage _owned_ by the group.
     * @tparam Get Types of storage _observed_ by the group, if any.
     * @tparam Exclude Types of storage used to filter the group, if any.
     * @return A newly created group.
     */
    template<typename... Owned, typename... Get, typename... Exclude>
    basic_group<owned_t<storage_for_type<Owned>...>, get_t<storage_for_type<Get>...>, exclude_t<storage_for_type<Exclude>...>>
    group(get_t<Get...> = get_t{}, exclude_t<Exclude...> = exclude_t{}) {
        using group_type = basic_group<owned_t<storage_for_type<Owned>...>, get_t<storage_for_type<Get>...>, exclude_t<storage_for_type<Exclude>...>>;
        using handler_type = typename group_type::handler;

        if(auto it = groups.find(group_type::group_id()); it != groups.cend()) {
            return {*std::static_pointer_cast<handler_type>(it->second)};
        }

        std::shared_ptr<handler_type> handler{};

        if constexpr(sizeof...(Owned) == 0u) {
            handler = std::allocate_shared<handler_type>(get_allocator(), get_allocator(), std::forward_as_tuple(assure<std::remove_const_t<Get>>()...), std::forward_as_tuple(assure<std::remove_const_t<Exclude>>()...));
        } else {
            handler = std::allocate_shared<handler_type>(get_allocator(), std::forward_as_tuple(assure<std::remove_const_t<Owned>>()..., assure<std::remove_const_t<Get>>()...), std::forward_as_tuple(assure<std::remove_const_t<Exclude>>()...));
            ENTT_ASSERT(std::all_of(groups.cbegin(), groups.cend(), [](const auto &data) { return !(data.second->owned(type_id<Owned>().hash()) || ...); }), "Conflicting groups");
        }

        groups.emplace(group_type::group_id(), handler);
        return {*handler};
    }

    /*! @copydoc group */
    template<typename... Owned, typename... Get, typename... Exclude>
    [[nodiscard]] basic_group<owned_t<storage_for_type<const Owned>...>, get_t<storage_for_type<const Get>...>, exclude_t<storage_for_type<const Exclude>...>>
    group_if_exists(get_t<Get...> = get_t{}, exclude_t<Exclude...> = exclude_t{}) const {
        using group_type = basic_group<owned_t<storage_for_type<const Owned>...>, get_t<storage_for_type<const Get>...>, exclude_t<storage_for_type<const Exclude>...>>;
        using handler_type = typename group_type::handler;

        if(auto it = groups.find(group_type::group_id()); it != groups.cend()) {
            return {*std::static_pointer_cast<handler_type>(it->second)};
        }

        return {};
    }

    /**
     * @brief Checks whether the given elements belong to any group.
     * @tparam Type Types of elements in which one is interested.
     * @return True if the pools of the given elements are _free_, false
     * otherwise.
     */
    template<typename... Type>
    [[nodiscard]] bool owned() const {
        return std::any_of(groups.cbegin(), groups.cend(), [](auto &&data) { return (data.second->owned(type_id<Type>().hash()) || ...); });
    }

    /**
     * @brief Sorts the elements of a given element.
     *
     * The comparison function object returns `true` if the first element is
     * _less_ than the second one, `false` otherwise. Its signature is also
     * equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(const Entity, const Entity);
     * bool(const Type &, const Type &);
     * @endcode
     *
     * Moreover, it shall induce a _strict weak ordering_ on the values.<br/>
     * The sort function object offers an `operator()` that accepts:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function object to use to compare the elements.
     *
     * The comparison function object hasn't necessarily the type of the one
     * passed along with the other parameters to this member function.
     *
     * @warning
     * Pools of elements owned by a group cannot be sorted.
     *
     * @tparam Type Type of elements to sort.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename Type, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        ENTT_ASSERT(!owned<Type>(), "Cannot sort owned storage");
        auto &cpool = assure<Type>();

        if constexpr(std::is_invocable_v<Compare, decltype(cpool.get({})), decltype(cpool.get({}))>) {
            auto comp = [&cpool, compare = std::move(compare)](const auto lhs, const auto rhs) { return compare(std::as_const(cpool.get(lhs)), std::as_const(cpool.get(rhs))); };
            cpool.sort(std::move(comp), std::move(algo), std::forward<Args>(args)...);
        } else {
            cpool.sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Sorts two pools of elements in the same way.
     *
     * Entities and elements in `To` which are part of both storage are sorted
     * internally with the order they have in `From`. The others follow in no
     * particular order.
     *
     * @warning
     * Pools of elements owned by a group cannot be sorted.
     *
     * @tparam To Type of elements to sort.
     * @tparam From Type of elements to use to sort.
     */
    template<typename To, typename From>
    void sort() {
        ENTT_ASSERT(!owned<To>(), "Cannot sort owned storage");
        const base_type &cpool = assure<From>();
        assure<To>().sort_as(cpool.begin(), cpool.end());
    }

    /**
     * @brief Returns the context object, that is, a general purpose container.
     * @return The context object, that is, a general purpose container.
     */
    [[nodiscard]] context &ctx() noexcept {
        return vars;
    }

    /*! @copydoc ctx */
    [[nodiscard]] const context &ctx() const noexcept {
        return vars;
    }

private:
    context vars;
    pool_container_type pools;
    group_container_type groups;
    storage_for_type<entity_type> entities;
};

} // namespace entt

#endif
