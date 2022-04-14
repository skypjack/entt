#ifndef ENTT_ENTITY_POLYMORPHIC_HPP
#define ENTT_ENTITY_POLYMORPHIC_HPP

#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"
#include "poly_type_traits.hpp"
#include "poly_storage_mixin.hpp"


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace entt::internal {

template<typename Convert, typename It>
struct converting_iterator {
    using value_type = std::remove_reference_t<decltype(std::declval<Convert>()(*std::declval<It>()))>;
    using reference = value_type&;
    using pointer = value_type*;
    using difference_type = typename It::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;

    converting_iterator() ENTT_NOEXCEPT = default;

    converting_iterator(It it) ENTT_NOEXCEPT :
        wrapped(it) {}

    converting_iterator &operator++() ENTT_NOEXCEPT {
        wrapped++;
        return *this;
    }

    converting_iterator operator++(int) ENTT_NOEXCEPT {
        converting_iterator orig = *this;
        return ++(*this), orig;
    }

    converting_iterator &operator--() ENTT_NOEXCEPT {
        wrapped--;
        return *this;
    }

    converting_iterator operator--(int) ENTT_NOEXCEPT {
        converting_iterator orig = *this;
        return --(*this), orig;
    }

    bool operator==(const converting_iterator& other) const ENTT_NOEXCEPT {
        return wrapped == other.wrapped;
    }

    bool operator!=(const converting_iterator& other) const ENTT_NOEXCEPT {
        return wrapped != other.wrapped;
    }

    reference operator*() ENTT_NOEXCEPT {
        Convert convert{};
        return convert(*wrapped);
    }

    pointer operator->() ENTT_NOEXCEPT {
        return std::addressof(operator*());
    }

private:
    It wrapped;
};


template<typename PoolsIterator>
class poly_components_iterator {
    using iterator_traits = typename std::iterator_traits<PoolsIterator>;
    using pool_holder_type = typename iterator_traits::value_type;

public:
    using entity_type = typename pool_holder_type::entity_type;
    using pointer = std::conditional_t<std::is_const_v<pool_holder_type>, typename pool_holder_type::const_pointer_type, typename pool_holder_type::pointer_type>;
    using value_type = std::conditional_t<std::is_pointer_v<typename pool_holder_type::value_type>, pointer, constness_as_t<typename pool_holder_type::value_type, pool_holder_type>>;
    using reference = std::conditional_t<std::is_pointer_v<typename pool_holder_type::value_type>, pointer, value_type&>;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::forward_iterator_tag;

    poly_components_iterator() ENTT_NOEXCEPT = default;

    poly_components_iterator(entity_type e, PoolsIterator pos, PoolsIterator last) ENTT_NOEXCEPT :
        ent(e), it(pos), end(last) {
        if (it != end) {
            current = it->try_get(ent);
            if(current == nullptr) {
                ++(*this);
            }
        }
    }

    poly_components_iterator &operator++() ENTT_NOEXCEPT {
        ++it;
        while (it != end) {
            current = it->try_get(ent);
            if (current != nullptr)
                break;
            ++it;
        }
        return *this;
    }

    poly_components_iterator operator++(int) ENTT_NOEXCEPT {
        poly_components_iterator orig = *this;
        return ++(*this), orig;
    }

    bool operator==(const poly_components_iterator& other) const ENTT_NOEXCEPT {
        return it == other.it;
    }

    bool operator!=(const poly_components_iterator& other) const ENTT_NOEXCEPT {
        return it != other.it;
    }

    pointer operator->() ENTT_NOEXCEPT {
        return current;
    }

    reference operator*() ENTT_NOEXCEPT {
        if constexpr(std::is_pointer_v<value_type>) {
            return operator->();
        } else {
            return *operator->();
        }
    }

private:
    entity_type ent;
    PoolsIterator it;
    PoolsIterator end;
    pointer current = nullptr;
};

} // namespace entt::internal

/**
 * Internal details not to be documented.
 * @endcond
 */


namespace entt {

/** @brief base class for entt::poly_pool_holder */
template<typename Entity>
class poly_pool_holder_base {
public:
    inline poly_pool_holder_base(basic_sparse_set<Entity>* pool,
                                 void* (*getter)(void*, Entity) ENTT_NOEXCEPT,
                                 bool (*remover)(void*, Entity)) :
          pool_ptr(pool), getter_ptr(getter), remover_ptr(remover) {}

protected:
    basic_sparse_set<Entity>* pool_ptr;
    void* (*getter_ptr)(void*, Entity) ENTT_NOEXCEPT;
    bool (*remover_ptr)(void*, Entity);
};

/**
 * @brief Holds pointer to child type storage and a function to convert it into parent type
 * @tparam Entity underlying entity type
 * @tparam Type polymorphic component type to be converted into
 */
template<typename Entity, typename Type>
class poly_pool_holder : public poly_pool_holder_base<Entity> {
public:
    using poly_pool_holder_base<Entity>::poly_pool_holder_base;

    /** @brief entity type of the component pools */
    using entity_type = Entity;
    /** @brief component type */
    using value_type = Type;
    /** @brief pointer to the component, will be same as value_type, if component itself is a pointer */
    using pointer_type = std::remove_pointer_t<Type>*;
    /** @copydoc pointer_type */
    using const_pointer_type = const std::remove_pointer_t<Type>*;

    /**
     * @brief gets child from a child pool as parent type reference by given entity
     * @param ptr pointer from the child type set
     * @return Returns reference to Type, converted from a given pointer. Will return pointer instead of reference, if parent type is a pointer.
     */
    inline pointer_type try_get(const Entity ent) ENTT_NOEXCEPT {
        return pointer_type(this->getter_ptr(this->pool_ptr, ent));
    }

    /** @copydoc try_get */
    inline const_pointer_type try_get(const Entity ent) const ENTT_NOEXCEPT {
        return const_cast<poly_pool_holder<Entity, Type>*>(this)->try_get(ent);
    }

    /**
     * @brief removes component from pool by entity
     * @param ent entity
     * @return true, if component was removed, false, if it didnt exist
     */
    inline bool remove(const Entity ent) {
        return this->remover_ptr(this->pool_ptr, ent);
    }

    /** @brief returns underlying pool */
    inline auto& pool() ENTT_NOEXCEPT {
        return *this->pool_ptr;
    }

    /** @copydoc pool */
    inline const auto& pool() const ENTT_NOEXCEPT {
        return *this->pool_ptr;
    }
};

/**
 * @brief Holds runtime information about one polymorphic component type.
 * @tparam Entity
 */
template<typename Entity, typename Type>
class poly_type {
    /** @brief internally used to convert base_poly_pool_holder to a pool_poly_holder<Entity, Type> during the iteration */
    struct cast_pool_holder {
        auto& operator()(poly_pool_holder_base<Entity>& holder) {
            return static_cast<poly_pool_holder<Entity, Type>&>(holder);
        }

        const auto& operator()(const poly_pool_holder_base<Entity>& holder) const {
            return static_cast<const poly_pool_holder<Entity, Type>&>(holder);
        }
    };

public:
    /** @brief entity type of the component pools */
    using entity_type = Entity;
    /** @brief component type */
    using value_type = Type;
    /** @brief type of the underlying pool container */
    using pools_container = std::vector<poly_pool_holder_base<Entity>>;
    /** @brief pool iterator type */
    using pools_iterator = internal::converting_iterator<cast_pool_holder, typename pools_container::iterator>;
    /** @brief const pool iterator type */
    using const_pools_iterator = internal::converting_iterator<const cast_pool_holder, typename pools_container::const_iterator>;
    /** @brief single entity component iterator type */
    using component_iterator = internal::poly_components_iterator<pools_iterator>;
    /** @brief const single entity component iterator type */
    using const_component_iterator = internal::poly_components_iterator<const_pools_iterator>;

    /**
     * @brief From a given set, makes a poly_storage_holder to hold child type and convert to parent type.
     * @tparam ChildType source type to convert from
     * @param pool_ptr child (source) type pool
     * @return poly_storage_holder to hold ChildType set and access it as Type
     */
    template<typename StorageType>
    inline static poly_pool_holder<Entity, Type> make_pool_holder(StorageType* pool_ptr) ENTT_NOEXCEPT {
        using ChildType = typename StorageType::value_type;
        static_assert(is_poly_type_v<ChildType>);
        static_assert(type_list_contains_v<poly_parent_types_t<ChildType>, Type> || std::is_same_v<ChildType, Type>);
        static_assert(std::is_pointer_v<ChildType> == std::is_pointer_v<Type>);

        void* (*get)(void*, Entity entity) ENTT_NOEXCEPT = +[](void* pool, const Entity entity) ENTT_NOEXCEPT -> void* {
            if (static_cast<StorageType*>(pool)->contains(entity)) {
                // if entity is contained within the set
                if constexpr(std::is_base_of_v<std::remove_pointer_t<Type>, std::remove_pointer_t<ChildType>>) {
                    if constexpr(std::is_pointer_v<ChildType>) {
                        // Type and ChildType are pointers, dereference source pointer and do conversion
                        ChildType ptr = static_cast<StorageType*>(pool)->get(entity);
                        return static_cast<Type>(ptr);
                    } else {
                        // Type is base of ChildType, do pointer conversion
                        return static_cast<Type*>(std::addressof(static_cast<StorageType*>(pool)->get(entity)));
                    }
                } else {
                    // no inheritance - no conversion required
                    if constexpr(std::is_pointer_v<ChildType>) {
                        // in case of pointer type return it as it is
                        return static_cast<StorageType*>(pool)->get(entity);
                    } else {
                        // otherwise, get the address
                        return std::addressof(static_cast<StorageType*>(pool)->get(entity));
                    }
                }
            }
            // otherwise, return null
            return nullptr;
        };

        bool (*remove)(void*, Entity entity) = +[](void* pool, const Entity entity) -> bool {
            return static_cast<StorageType*>(pool)->remove(entity);
        };

        return poly_pool_holder<Entity, Type>(pool_ptr, get, remove);
    }

    /** @brief adds given storage pointer as a child type pool to this polymorphic type */
    template<typename Storage>
    void bind_child_storage(Storage* storage_ptr) {
        this->child_pool_holders.emplace_back(make_pool_holder(storage_ptr));
    }

    /** @brief returns an iterable to iterate through all pool holders */
    [[nodiscard]] iterable_adaptor<pools_iterator> pools() {
        return { pools_iterator(this->child_pool_holders.begin()), pools_iterator(this->child_pool_holders.end()) };
    }

    /** @copydoc pools */
    [[nodiscard]] iterable_adaptor<const_pools_iterator> pools() const {
        return { const_pools_iterator(this->child_pool_holders.cbegin()), const_pools_iterator(this->child_pool_holders.cend()) };
    }

    /** @brief returns an iterable to iterate through all polymorphic components, derived from this type, attached to a given entities */
    [[nodiscard]] iterable_adaptor<component_iterator> each(const entity_type ent) {
        auto begin = pools_iterator(this->child_pool_holders.begin());
        auto end = pools_iterator(this->child_pool_holders.end());
        return { component_iterator(ent, begin, end), component_iterator(ent, end, end) };
    }

    /** @copydoc each */
    [[nodiscard]] iterable_adaptor<const_component_iterator> each(const entity_type ent) const {
        auto begin = const_pools_iterator(this->child_pool_holders.cbegin());
        auto end = const_pools_iterator(this->child_pool_holders.cend());
        return { const_component_iterator(ent, begin, end), const_component_iterator(ent, end, end) };
    }

private:
    pools_container child_pool_holders{};
};


/**
 * @brief Provides access of runtime polymorphic type data. Allows user to create specializations
 * to store polymorphic type data separately from the registry
 * Must specialize assure<Type>() method, where Type is poly_type<entity, component>
 * @tparam PolyTypesData underlying polymorphic types data storage type
 */
template<typename PolyTypesData>
struct poly_types_accessor;

/**
 * poly_types_accessor specialisation to access runtime polymorphic type data, stored in the registry context
 * @tparam Entity
 */
template<typename Entity>
struct poly_types_accessor<basic_registry<Entity>> {
    /**
     * @tparam T polymorphic type
     * @return poly_type for given type
     */
    template<typename T>
    static inline decltype(auto) assure(basic_registry<Entity>& reg) {
        return reg.ctx().template emplace<T>();
    }
};

/**
 * Converts const T* to T* const, used in assure_poly_type, because first one looks much more intuitive,
 * when the second is identical to const T for value types, but for pointers
 */
template<typename T>
using sanitize_poly_type_t = std::conditional_t<std::is_pointer_v<T> && !std::is_const_v<T>,
                                                constness_as_t<std::remove_const_t<std::remove_pointer_t<T>>*, std::remove_pointer_t<T>>, T>;

/**
 * @brief Assures runtime polymorphic type data for a given entity and component types.
 * @tparam Entity entity type
 * @tparam Component polymorphic component type
 * @tparam PolyTypesData underlying runtime polymorphic types data storage type
 * @param data runtime polymorphic types data
 * @return reference to poly_type<Entity, Component>, keeps const qualifiers both for Component and PolyData
 */
template<typename Entity, typename Component, typename PolyTypesData>
[[nodiscard]] decltype(auto) assure_poly_type(PolyTypesData& data) {
    using type = sanitize_poly_type_t<Component>;
    using no_const_type = std::remove_const_t<type>;
    static_assert(is_poly_type_v<no_const_type>, "must be a polymorphic type");

    using result_type = poly_type<Entity, no_const_type>;
    result_type& result = poly_types_accessor<PolyTypesData>::template assure<result_type>(const_cast<std::remove_const_t<PolyTypesData>&>(data));
    if constexpr(std::is_const_v<type> || std::is_const_v<PolyTypesData>) {
        return const_cast<const result_type&>(result);
    } else {
        return result;
    }
}

} // namespace entt


namespace entt::algorithm {

/**
 * @brief For given polymorphic component type iterate over all child instances of it, attached to a given entity
 * @tparam Component Polymorphic component type
 * @param entity Entity, to get components from
 * @return Iterable to iterate each component
 */
template<typename Component, typename Entity>
[[nodiscard]] decltype(auto) poly_get_all(basic_registry<Entity>& reg, [[maybe_unused]] const Entity entity) {
    ENTT_ASSERT(reg.valid(entity), "Invalid entity");
    return assure_poly_type<Entity, Component>(reg).each(entity);
}

/**
 * @brief For given polymorphic component type find and return any child instance of this type, attached to a given entity
 * @tparam Component Polymorphic component type
 * @param entity Entity, to get components from
 * @return Pointer to attached component or nullptr, if none attached. NOTE: will return pointer type polymorphic components by value (as a single pointer)
 */
template<typename Component, typename Entity>
[[nodiscard]] decltype(auto) poly_get_any(basic_registry<Entity>& reg, [[maybe_unused]] const Entity entity) {
    auto all = poly_get_all<Component>(reg, entity);
    return all.begin() != all.end() ? all.begin().operator->() : nullptr;
}

/**
 * @brief For given polymorphic component type remove all child instances of this type, attached to a given entity
 * @tparam Component Polymorphic component type
 * @param entity Entity, to remove components from
 * @return count of removed components
 */
template<typename Component, typename Entity>
int poly_remove(basic_registry<Entity>& reg, [[maybe_unused]] const Entity entity) {
    ENTT_ASSERT(reg.valid(entity), "Invalid entity");
    int removed_count = 0;
    for (auto& pool : assure_poly_type<Entity, Component>(reg).pools()) {
        removed_count += static_cast<int>(pool.remove(entity));
    }
    return removed_count;
}

/**
 * @brief For a given component type applies given func to all child instances of this type in registry.
 * @tparam Component polymorphic component type
 * @tparam Entity entity type of underlying registry
 * @tparam Func type of given function
 * @param reg registry to operate on
 * @param func function to call for each component, parameters are (entity, component&) or only one of them, or even none. Note: for pointer type components the component parameter is a value instead of value (single pointer)
 */
template<typename Component, typename Entity, typename Func>
void each_poly(basic_registry<Entity>& reg, Func func) {
    for (auto& pool : assure_poly_type<Entity, Component>(reg).pools()) {
        for (auto& ent : pool.pool()) {
            if constexpr(std::is_invocable_v<Func, Entity, Component&>) {
                auto ptr = pool.try_get(ent);
                if constexpr(std::is_pointer_v<Component>) {
                    func(ent, ptr);
                } else {
                    func(ent, *ptr);
                }
            } else if constexpr(std::is_invocable_v<Func, Component&>) {
                auto ptr = pool.try_get(ent);
                if constexpr(std::is_pointer_v<Component>) {
                    func(ptr);
                } else {
                    func(*ptr);
                }
            } else if constexpr(std::is_invocable_v<Func, Entity>) {
                func(ent);
            } else {
                func();
            }
        }
    }
}

} // namespace entt::algorithm


/**
 * @brief storage_traits template specialization for polymorphic component types
 * @tparam Entity entity type
 * @tparam Type polymorphic component type
 */
template<typename Entity, typename Type>
struct entt::storage_traits<Entity, Type, std::enable_if_t<entt::is_poly_type_v<Type>>> {
    template<typename... T>
    struct parent_guard;

    /** @brief used to assert, that all parent types of the component are polymorphic */
    template<typename T, typename... Parents>
    struct parent_guard<T, type_list<Parents...>> {
        static_assert((is_poly_type_v<Parents> && ...), "all parents of polymorphic type must be also declared polymorphic");
        using type = T;
    };

    using guarded_type = typename parent_guard<Type, poly_parent_types_t<Type>>::type;

public:
    using storage_type = poly_storage_mixin<basic_storage<Entity, guarded_type>, Entity, guarded_type>;
};


#endif // ENTT_ENTITY_POLYMORPHIC_HPP
