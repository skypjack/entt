#ifndef ENTT_ENTITY_POLYMORPHIC_HPP
#define ENTT_ENTITY_POLYMORPHIC_HPP

#include <variant>
#include <vector>
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

template<typename PoolsIterator, typename PoolHolderType>
class poly_components_iterator {
    using iterator_traits = typename std::iterator_traits<PoolsIterator>;

public:
    using entity_type = typename PoolHolderType::entity_type;
    using pointer = std::conditional_t<std::is_const_v<PoolHolderType>, typename PoolHolderType::const_pointer_type, typename PoolHolderType::pointer_type>;
    using value_type = std::conditional_t<std::is_pointer_v<typename PoolHolderType::value_type>, pointer, constness_as_t<typename PoolHolderType::value_type, PoolHolderType>>;
    using reference = std::conditional_t<std::is_pointer_v<typename PoolHolderType::value_type>, pointer, value_type&>;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::forward_iterator_tag;

    poly_components_iterator() ENTT_NOEXCEPT = default;

    poly_components_iterator(entity_type e, PoolsIterator pos, PoolsIterator last) ENTT_NOEXCEPT :
        ent(e), it(pos), end(last) {
        if (it != end) {
            current = static_cast<PoolHolderType&>(*it).try_get(ent);
            if(current == nullptr) {
                ++(*this);
            }
        }
    }

    poly_components_iterator &operator++() ENTT_NOEXCEPT {
        ++it;
        while (it != end) {
            current = static_cast<PoolHolderType&>(*it).try_get(ent);
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


template<typename T>
struct basic_inlined_vector {
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() {
        return std::visit([&] (auto& payload) -> iterator {
            using type = std::decay_t<decltype(payload)>;
            if constexpr(std::is_same_v<type, std::monostate>) {
                return nullptr;
            } else if constexpr(std::is_same_v<type, T>) {
                return std::addressof(payload);
            } else if constexpr(std::is_same_v<type, std::vector<T>>) {
                return std::addressof(*payload.begin());
            }
        }, storage);
    }

    const_iterator begin() const {
        return const_cast<basic_inlined_vector<T>*>(this)->begin();
    }

    iterator end() {
        return std::visit([&] (auto& payload) -> iterator {
            using type = std::decay_t<decltype(payload)>;
            if constexpr(std::is_same_v<type, std::monostate>) {
                return nullptr;
            } else if constexpr(std::is_same_v<type, T>) {
                return std::addressof(payload) + 1;
            } else if constexpr(std::is_same_v<type, std::vector<T>>) {
                return std::addressof(*payload.begin()) + payload.size();
            }
        }, storage);
    }

    const_iterator end() const {
        return const_cast<basic_inlined_vector<T>*>(this)->end();
    }

    template<typename... Args>
    void emplace_back(Args&& ...args) {
        auto& s = storage;
        std::visit([&] (auto& payload) -> void {
            using type = std::decay_t<decltype(payload)>;
            if constexpr(std::is_same_v<type, std::monostate>) {
                s.template emplace<T>(std::forward<Args>(args)...);
            } else if constexpr(std::is_same_v<type, T>) {
                std::vector<T> v;
                v.reserve(2);
                v.emplace_back(std::move(payload));
                v.emplace_back(std::forward<Args>(args)...);
                s.template emplace<std::vector<T>>(std::move(v));
            } else if constexpr(std::is_same_v<type, std::vector<T>>) {
                payload.emplace_back(std::forward<Args>(args)...);
            }
        }, storage);
    }

private:
    std::variant<std::monostate, T, std::vector<T>> storage;
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
    inline poly_pool_holder_base(void* pool,
                                 void* (*getter)(void*, Entity) ENTT_NOEXCEPT) :
          pool_ptr(pool), getter_ptr(getter) {}

protected:
    void* pool_ptr;
    void* (*getter_ptr)(void*, Entity) ENTT_NOEXCEPT;
};

/**
 * @brief Holds pointer to child type storage and a function to convert it into parent type
 * @tparam Entity underlying entity type
 * @tparam Type polymorphic component type to convert into
 * @tparam Allocator allocator type of the pool
 */
template<typename Entity, typename Type, typename Allocator>
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
    /** @brief type of hold sparse set */
    using sparse_set_type = basic_sparse_set<Entity, typename std::allocator_traits<Allocator>::template rebind_alloc<Entity>>;

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
        return const_cast<poly_pool_holder<Entity, Type, Allocator>*>(this)->try_get(ent);
    }

    /** @brief returns underlying pool */
    inline auto& pool() ENTT_NOEXCEPT {
        return *static_cast<sparse_set_type*>(this->pool_ptr);
    }

    /** @copydoc pool */
    inline const auto& pool() const ENTT_NOEXCEPT {
        return *static_cast<sparse_set_type*>(this->pool_ptr);
    }

    /**
     * @brief removes component from pool by entity
     * @param ent entity
     * @return true, if component was removed, false, if it didnt exist
     */
    inline bool remove(const Entity ent) {
        return pool().remove(ent);
    }
};

/**
 * @brief Holds runtime information about one polymorphic component type.
 * @tparam Entity underlying entity type
 * @tparam Type polymorphic component type
*  @tparam Allocator allocator type for all bound contained pool holders
 */
template<typename Entity, typename Type, typename Allocator>
class poly_type {
    /** @brief derived value to base pointer conversion, workaround for if constexpr bug on some compilers */
    template<typename Base, typename Derived, typename = void>
    struct derived_to_base_ptr {
        inline static Base* convert(Derived& ref) ENTT_NOEXCEPT {
            return static_cast<Base*>(std::addressof(ref));
        }
    };

    /** @copydoc derived_to_base_ptr */
    template<typename Base, typename Derived>
    struct derived_to_base_ptr<Base, Derived, std::enable_if_t<std::is_pointer_v<Derived>>> {
        inline static Base convert(Derived ptr) ENTT_NOEXCEPT {
            return static_cast<Base>(ptr);
        }
    };

    /** @brief value to pointer conversion, workaround for if constexpr bug on some compilers */
    template<typename T, typename = void>
    struct value_to_ptr {
        inline static T* convert(T& ref) ENTT_NOEXCEPT {
            return std::addressof(ref);
        }
    };

    /** @copydoc value_to_ptr */
    template<typename T>
    struct value_to_ptr<T, std::enable_if_t<std::is_pointer_v<T>>> {
        inline static T convert(T ptr) ENTT_NOEXCEPT {
            return ptr;
        }
    };

public:
    /** @brief entity type of the component pools */
    using entity_type = Entity;
    /** @brief component type */
    using value_type = Type;
    /** @brief allocator type */
    using allocator_type = Allocator;
    /** @brief type of underlying container for bound pools */
    using pools_container = internal::basic_inlined_vector<poly_pool_holder_base<Entity>>;
    /** @brief type of the pool holder */
    using pool_holder = poly_pool_holder<Entity, Type, allocator_type>;
    /** @brief single entity component iterator type */
    using component_iterator = internal::poly_components_iterator<typename pools_container::iterator, pool_holder>;
    /** @brief const single entity component iterator type */
    using const_component_iterator = internal::poly_components_iterator<typename pools_container::const_iterator, const pool_holder>;

    /**
     * @brief From a given set, makes a poly_storage_holder to hold child type and convert to parent type.
     * @tparam ChildType source type to convert from
     * @param pool_ptr child (source) type pool
     * @return poly_storage_holder to hold ChildType set and access it as Type
     */
    template<typename StorageType>
    inline static pool_holder make_pool_holder(StorageType* pool_ptr) ENTT_NOEXCEPT {
        using BaseType = Type;
        using DerivedType = typename StorageType::value_type;
        static_assert(is_poly_type_v<DerivedType>);
        static_assert(type_list_contains_v<poly_parent_types_t<DerivedType>, BaseType> || std::is_same_v<DerivedType, BaseType>);
        static_assert(std::is_pointer_v<DerivedType> == std::is_pointer_v<BaseType>);
        static_assert(std::is_same_v<typename std::allocator_traits<allocator_type>::template rebind_traits<Entity>::pointer, typename std::allocator_traits<typename StorageType::base_type::allocator_type>::pointer>,
                      "Allocator pointer types dont match for polymorphic types in one hierarchy. Use std::poly_type_allocator to explicitly provide allocator type for each polymorphic component type.");

        void* (*get)(void*, Entity entity) ENTT_NOEXCEPT = +[](void* pool, const Entity entity) ENTT_NOEXCEPT -> void* {
            if (static_cast<StorageType*>(pool)->contains(entity)) {
                // if entity is contained within the set
                if constexpr(std::is_base_of_v<std::remove_pointer_t<BaseType>, std::remove_pointer_t<DerivedType>>) {
                    // if base type is inherited from derived type, do pointer conversion
                    return derived_to_base_ptr<BaseType, DerivedType>::convert(static_cast<StorageType*>(pool)->get(entity));
                } else {
                    // no inheritance - no conversion required, just get the pointer
                    return value_to_ptr<DerivedType>::convert(static_cast<StorageType*>(pool)->get(entity));
                }
            }
            // otherwise, return null
            return nullptr;
        };

        return pool_holder(pool_ptr, get);
    }

    /** @brief binds given storage pointer as a child type pool to this polymorphic type */
    template<typename Storage>
    void bind_storage(Storage* storage_ptr) {
        bound_pools.emplace_back(make_pool_holder(storage_ptr));
    }

    /** @brief returns all pools bound for this type */
    [[nodiscard]] auto& get_bound_pools() ENTT_NOEXCEPT {
        return bound_pools;
    }

    /** @copydoc get_bound_pools */
    [[nodiscard]] const auto& get_bound_pools() const ENTT_NOEXCEPT {
        return bound_pools;
    }

    /** @brief calls given function for all child pool holders */
    template<typename Func>
    void each_pool(Func func) {
        for (auto& pool : bound_pools) {
            func(static_cast<pool_holder&>(pool));
        }
    }

    /** @copydoc each_pool */
    template<typename Func>
    void each_pool(Func func) const {
        for (auto& pool : bound_pools) {
            func(static_cast<const pool_holder&>(pool));
        }
    }

    /** @brief returns an iterable to iterate through all polymorphic components, derived from this type, attached to a given entities */
    [[nodiscard]] iterable_adaptor<component_iterator> each(const entity_type ent) {
        auto begin = bound_pools.begin();
        auto end = bound_pools.end();
        return { component_iterator(ent, begin, end), component_iterator(ent, end, end) };
    }

    /** @copydoc each */
    [[nodiscard]] iterable_adaptor<const_component_iterator> each(const entity_type ent) const {
        auto begin = bound_pools.begin();
        auto end = bound_pools.end();
        return { const_component_iterator(ent, begin, end), const_component_iterator(ent, end, end) };
    }

private:
    pools_container bound_pools{};
};


/**
 * @brief Provides access of runtime polymorphic type data from the underlying pools holder type.
 * Allows user to create specializations to store polymorphic type data separately from the registry.
 *
 * The poly_types_accessor specialization must define:<br/>
 * - Type& assure<Type>(PolyPoolsHolder&) method, it must assure and return instance of @b Type stored given @b PolyPoolsHolder<br/>
 * - holder_type - same as PolyPoolsHolder<br/>
 * - entity_type - underlying entity type<br/>
 * - allocator_type<Type> - allocator type getter for given component type Type<br/>
 *
 * @tparam PolyPoolsHolder Underlying polymorphic types data holder
 */
template<typename PolyPoolsHolder>
struct poly_types_accessor;

/**
 * poly_types_accessor specialisation to access polymorphic data, stored in the registry context
 * @tparam Entity
 */
template<typename Entity>
struct poly_types_accessor<basic_registry<Entity>> {
    /** @brief Underlying polymorphic types data holder  */
    using holder_type = basic_registry<Entity>;

    /** @brief Underlying entity type  */
    using entity_type = Entity;

    /** @brief Returns allocator type for Type */
    template<typename Type>
    using allocator_type = poly_type_allocator_t<Type>;

    /**
     * @brief Assures and returns instance of some type stored given polymorphic types data holder
     * @tparam Type Default-constructed type to assure in the underlying data holder and return
     * @param holder Polymorphic types data holder reference
     * @return
     */
    template<typename Type>
    static inline decltype(auto) assure(basic_registry<Entity>& holder) {
        return holder.ctx().template emplace<Type>();
    }
};

/**
 * @brief Converts const T* to T* const, used in assure_poly_type, because first one looks much more intuitive,
 * when the second is identical to const T for value types, but for pointers
 */
template<typename T>
using poly_type_sanitize_t = std::conditional_t<std::is_pointer_v<T>,
    std::conditional_t<std::is_const_v<T>,
        std::remove_const_t<std::remove_pointer_t<T>>* const, // (const?) T* const
        constness_as_t<std::remove_const_t<std::remove_pointer_t<T>>*, std::remove_pointer_t<T>>> // (const?) T*
                                                , T>; // (const?) T

/**
 * @brief Assures runtime polymorphic type data for a given component types in the given polymorphic data holder
 * @tparam Component Polymorphic component type to assure
 * @tparam PolyPoolsHolder Polymorphic types data holder type
 * @param holder Polymorphic types data holder to operate on
 * @return Reference to poly_type<PolyPoolsHolder::entity_type, Component>, keeps const qualifiers both for Component and PolyPoolsHolder
 */
template<typename Component, typename PolyPoolsHolder>
[[nodiscard]] decltype(auto) assure_poly_type(PolyPoolsHolder& holder) {
    using type = poly_type_sanitize_t<Component>;
    using no_const_type = poly_type_validate_t<std::remove_const_t<type>>;
    static_assert(is_poly_type_v<no_const_type>, "must be a polymorphic type");

    using accessor_type = poly_types_accessor<PolyPoolsHolder>;
    using result_type = poly_type<typename accessor_type::entity_type, no_const_type, typename accessor_type::template allocator_type<no_const_type>>;
    result_type& result = accessor_type::template assure<result_type>(const_cast<std::remove_const_t<PolyPoolsHolder>&>(holder));
    if constexpr(std::is_const_v<type> || std::is_const_v<PolyPoolsHolder>) {
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
 * @param reg Registry, or any other polymorphic data holder to operate on
 * @param entity Entity, to get components from
 * @return Iterable to iterate each component
 */
template<typename Component, typename Entity, typename PolyPoolsHolder>
[[nodiscard]] decltype(auto) poly_get_all(PolyPoolsHolder& reg, [[maybe_unused]] const Entity entity) {
    return assure_poly_type<Component>(reg).each(entity);
}

/**
 * @brief For given polymorphic component type find and return any child instance of this type, attached to a given entity
 * @tparam Component Polymorphic component type
 * @param reg Registry, or any other polymorphic data holder to operate on
 * @param entity Entity, to get components from
 * @return Pointer to attached component or nullptr, if none attached. NOTE: will return pointer type polymorphic components by value (as a single pointer)
 */
template<typename Component, typename Entity, typename PolyPoolsHolder>
[[nodiscard]] decltype(auto) poly_get_any(PolyPoolsHolder& reg, [[maybe_unused]] const Entity entity) {
    auto all = poly_get_all<Component>(reg, entity);
    return all.begin() != all.end() ? all.begin().operator->() : nullptr;
}

/**
 * @brief For given polymorphic component type remove all child instances of this type, attached to a given entity
 * @tparam Component Polymorphic component type
 * @param reg Registry, or any other polymorphic data holder to operate on
 * @param entity Entity, to remove components from
 * @return Count of removed components
 */
template<typename Component, typename Entity, typename PolyPoolsHolder>
int poly_remove(PolyPoolsHolder& reg, [[maybe_unused]] const Entity entity) {
    int removed_count = 0;
    assure_poly_type<Component>(reg).each_pool([&](auto& pool){
        removed_count += static_cast<int>(pool.remove(entity));
    });
    return removed_count;
}

/**
* @brief For given polymorphic component type count all child instances of this type, attached to a given entity
* @tparam Component Polymorphic component type
* @param reg Registry, or any other polymorphic data holder to operate on
* @param entity Entity, to count attached components
* @return Count of components, attached to a given entity
  */
template<typename Component, typename Entity, typename PolyPoolsHolder>
size_t poly_count(PolyPoolsHolder& reg, [[maybe_unused]] const Entity entity) {
    size_t count = 0;
    assure_poly_type<Component>(reg).each_pool([&](auto& pool){
        count += static_cast<int>(pool.pool().contains(entity));
    });
    return count;
}

/**
* @brief For given polymorphic component type count all child instances of this type
* @tparam Component Polymorphic component type
* @param reg Registry, or any other polymorphic data holder to operate on
* @return Count of components
 */
template<typename Component, typename PolyPoolsHolder>
size_t poly_count(PolyPoolsHolder& reg) {
    size_t count = 0;
    assure_poly_type<Component>(reg).each_pool([&](auto& pool){
        count += pool.pool().size();
    });
    return count;
}

/**
 * @brief For a given component type applies given func to all child instances of this type in registry.
 * @tparam Component Polymorphic component type
 * @tparam PolyPoolsHolder Entity type of underlying registry
 * @tparam Entity Entity type of underlying registry
 * @tparam Func Type of given function
 * @param reg Registry, or any other polymorphic data holder to operate on
 * @param func Function to call for each component, parameters are (entity, component&) or only one of them, or even none.
 * Note: for pointer type components (T*) the component parameter is its value (T*) instead of pointer to value (T**)
 */
template<typename Component, typename PolyPoolsHolder, typename Entity = typename PolyPoolsHolder::entity_type, typename Func>
void poly_each(PolyPoolsHolder& reg, Func func) {
    assure_poly_type<Component>(reg).each_pool([&](auto& pool){
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
    });
}

} // namespace entt::algorithm


/**
 * @brief storage_traits template specialization for polymorphic component types
 * @tparam Entity entity type
 * @tparam Type polymorphic component type
 */
template<typename Entity, typename Type>
struct entt::storage_traits<Entity, Type, std::enable_if_t<entt::is_poly_type_v<Type>>> {
    using storage_type = sigh_storage_mixin<poly_storage_mixin<basic_storage<Entity, poly_type_validate_t<Type>, poly_type_allocator_t<Type>>>>;
};


#endif
