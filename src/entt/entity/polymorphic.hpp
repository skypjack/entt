#ifndef ENTT_ENTITY_POLYMORPHIC_HPP
#define ENTT_ENTITY_POLYMORPHIC_HPP

#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"


namespace entt::internal {

template<typename Convert, typename It>
struct converting_iterator {
    using value_type = std::decay_t<decltype(std::declval<Convert>()(*std::declval<It>()))>;
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


template<typename Type, typename PoolsIterator>
class poly_components_iterator {
    using iterator_traits = typename std::iterator_traits<PoolsIterator>;
    using pool_holder_type = typename iterator_traits::value_type;

public:
    using entity_type = typename pool_holder_type::entity_type;
    using value_type = typename pool_holder_type::value_type;
    using pointer = typename pool_holder_type::pointer_type;
    using reference = std::conditional_t<std::is_pointer_v<value_type>, value_type, value_type&>;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::forward_iterator_tag;

    poly_components_iterator() ENTT_NOEXCEPT = default;

    poly_components_iterator(entity_type e, PoolsIterator pos, PoolsIterator last) ENTT_NOEXCEPT :
        ent(e), it(pos), end(last) {
        if (it != end) {
            current = it->template try_get(ent);
            if(current == nullptr) {
                ++(*this);
            }
        }
    }

    poly_components_iterator &operator++() ENTT_NOEXCEPT {
        ++it;
        while (it != end) {
            current = it->template try_get(ent);
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


namespace entt {

/**
 * @brief declares direct parent types of polymorphic component type.
 * By default it uses the list from T::direct_parent_types, if it present, otherwise empty list is used.
 * All parent types must be declared polymorphic.
 * @code{.cpp}
 * struct A {};
 * struct B : A {};
 *
 * struct entt::poly_direct_parent_types<A> {
 *     using parent_types = entt::type_list<>; // declares A as polymorphic type with no parents
 * }
 * struct entt::poly_direct_parent_types<B> {
 *     using parent_types = entt::type_list<A>; // declares B as polymorphic type with parent A
 * }
 * @endcode
 * @tparam T polymorphic component type
 */
template<typename T, typename = void>
struct poly_direct_parent_types {
    /** @brief entt::type_list of direct parent types */
    using parent_types = type_list<>;

    /** @brief used to detect, if this template was specialized for a type */
    using not_redefined_tag = void;
};

/** @copydoc poly_direct_parent_types */
template<typename T>
struct poly_direct_parent_types<T, std::void_t<typename T::direct_parent_types>> {
    using parent_types = typename T::direct_parent_types;
};

/**
 * @brief for given polymorphic component type returns entt::type_list of direct parent types,
 * for non polymorphic type will return empty list
 * @tparam T type to get parents from
 */
template<typename T>
using poly_direct_parent_types_t = typename poly_direct_parent_types<T>::parent_types;

/**
 * @brief declares list of all parent types of polymorphic component type.
 * By default will concatenates list of all direct parent types and all lists of all parents for each parent type. All parent
 * types must be declared polymorphic.
 * @tparam T
 */
template<typename T, typename = void>
struct poly_parent_types {
private:
    template<typename, typename...>
    struct all_parent_types;

    template<typename... DirectParents>
    struct all_parent_types<type_list<DirectParents...>> {
        using types = type_list_cat_t<type_list<DirectParents...>, typename poly_parent_types<DirectParents>::parent_types...>;
    };

public:
    /** @brief entt::type_list of all parent types */
    using parent_types = typename all_parent_types<poly_direct_parent_types_t<T>>::types;

    /** @brief used to detect, if this template was specialized for a type */
    using not_redefined_tag = void;
};

/** @copydoc poly_parent_types */
template<typename T>
struct poly_parent_types<T, std::void_t<typename T::all_parent_types>> {
    using parent_types = typename T::all_parent_types;
};

/**
 * @brief for given polymorphic component type returns entt::type_list of all parent types,
 * for non polymorphic type will return empty list
 * @tparam T type to get parents from
 */
template<typename T>
using poly_parent_types_t = typename poly_parent_types<T>::parent_types;


/**
 * @brief for given type, detects, if it was declared polymorphic. Type considered polymorphic,
 * if it was either:
 *  - inherited from entt::inherit
 *  - declared types direct_parent_types or all_parent_types
 *  - for this type there is specialization of either entt::poly_direct_parent_types or entt::poly_parent_types
 * @tparam T type to check
 */
template<typename T, typename = void>
struct is_poly_type {
    static constexpr bool value = true;
};

/** @copydoc is_poly_type */
template<typename T>
struct is_poly_type<T, std::void_t<typename poly_direct_parent_types<T>::not_redefined_tag, typename poly_parent_types<T>::not_redefined_tag>> {
    static constexpr bool value = false;
};

/** @copydoc is_poly_type */
template<typename T>
static constexpr bool is_poly_type_v = is_poly_type<T>::value;

/**
 * @brief used to inherit from all given parent types and declare inheriting type polymorphic with given direct parents.
 * All parent types are required to be polymorphic
 * @code{.cpp}
 * struct A : public entt::inherit<> {}; // base polymorphic type with no parents
 * struct B : public entt::inherit<A> {}; // B inherits A, and declared as polymorphic component with direct parents {A}
 * struct C : public entt::inherit<B> {}; // C inherits B, and now has direct parents {B} and all parents {A, B}
 * @endcode
 * @tparam Parents list of parent types
 */
template<typename... Parents>
struct inherit : public Parents... {
    static_assert((is_poly_type_v<Parents> && ...), "entt::inherit must be used only with polymorphic types");
    using direct_parent_types = type_list<Parents...>;
};

/**
 * @brief TODO
 */
template<typename T>
struct poly_all {
    static_assert(is_poly_type_v<T>, "entt::poly_all must be used only with polymorphic types");
    using type = T;
    using poly_all_tag = void;
};

/**
 * @brief TODO
 */
template<typename T>
struct poly_any {
    static_assert(is_poly_type_v<T>, "entt::poly_any must be used only with polymorphic types");
    using type = T;
    using poly_any_tag = void;
};


/** @brief private base class for entt::poly_pool_holder */
template<typename Entity>
class poly_pool_holder_base {
public:
    inline poly_pool_holder_base(basic_sparse_set<Entity>* pool, void* (*getter)(void*, Entity) ENTT_NOEXCEPT) :
          pool_ptr(pool), getter_ptr(getter) {}

protected:
    basic_sparse_set<Entity>* pool_ptr;
    void* (*getter_ptr)(void*, Entity) ENTT_NOEXCEPT;
};

/** @brief private base class for entt::poly_type */
template<typename Entity>
class poly_type_base {
protected:
    using child_pools_container = std::vector<poly_pool_holder_base<Entity>>;

    child_pools_container child_pool_holders{};
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

    using entity_type = Entity;
    using value_type = Type;
    using pointer_type = std::remove_pointer_t<Type>*;

    /**
     * @brief gets child from a child pool as parent type reference by given entity
     * @param ptr pointer from the child type set
     * @return Returns reference to Type, converted from a given pointer. Will return pointer instead of reference, if parent type is a pointer.
     */
    inline pointer_type try_get(const Entity ent) ENTT_NOEXCEPT {
        return pointer_type(this->getter_ptr(this->pool_ptr, ent));
    }

    inline auto& pool() ENTT_NOEXCEPT {
        return *this->pool_ptr;
    }

    inline const auto& pool() const ENTT_NOEXCEPT {
        return *this->pool_ptr;
    }
};

/**
 * @brief Holds runtime information about one polymorphic component type.
 * @tparam Entity
 */
template<typename Entity, typename Type>
class poly_type : public poly_type_base<Entity> {
    struct cast_pool_holder {
        poly_pool_holder<Entity, Type>& operator()(poly_pool_holder_base<Entity>& holder) {
            return static_cast<poly_pool_holder<Entity, Type>&>(holder);
        }
    };

public:
    using entity_type = Entity;
    using value_type = Type;
    using pools_container = typename poly_type_base<Entity>::child_pools_container;
    using pools_iterator = internal::converting_iterator<cast_pool_holder, typename pools_container::iterator>;
    using component_iterator = internal::poly_components_iterator<value_type, pools_iterator>;

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

        return poly_pool_holder<Entity, Type>(pool_ptr, get);
    }

    template<typename Storage>
    void bind_child_storage(Storage* storage_ptr) {
        this->child_pool_holders.emplace_back(make_pool_holder(storage_ptr));
    }

    iterable_adaptor<pools_iterator> pools() {
        return { pools_iterator(this->child_pool_holders.begin()), pools_iterator(this->child_pool_holders.end()) };
    }

    iterable_adaptor<component_iterator> each(const entity_type ent) {
        auto begin = pools_iterator(this->child_pool_holders.begin());
        auto end = pools_iterator(this->child_pool_holders.end());
        return { component_iterator(ent, begin, end), component_iterator(ent, end, end) };
    }
};

/**
 * @brief Holds all data of all poly types in this registry
 * @tparam Entity
 */
template<typename Entity>
struct poly_type_data {
    /**
     * @param id type id of polymorphic type
     * @return poly_type for given type id
     */
    inline auto& assure(const id_type id) {
        return types[id];
    }

    /**
     * @tparam Type polymorphic type
     * @return poly_type for given type
     */
    template<typename Type>
    inline auto& assure() {
        static_assert(is_poly_type_v<Type>);
        return static_cast<poly_type<Entity, Type>&>(assure(type_id<Type>().hash()));
    }
private:
    dense_map<id_type, poly_type_base<Entity>, identity> types{};
};



/**
 * @brief Storage mixin for polymorphic component types
 * @tparam Storage
 */
template<typename Storage, typename = void>
class poly_storage_mixin;

/** @copydoc poly_storage_mixin */
template<typename Storage>
class poly_storage_mixin<Storage, void>: public Storage {
    using component_type = typename Storage::value_type;
    using entity_type = typename Storage::entity_type;

    using Storage::Storage;

    void bind(any value) ENTT_NOEXCEPT override {
        if(auto *reg = any_cast<basic_registry<entity_type>>(&value); reg) {
            register_for_all_parent_types(*reg, poly_parent_types_t<component_type>{});
        }
    }

    template<typename ParentType>
    void add_self_for_type(basic_registry<entity_type>& reg) {
        reg.poly_data().template assure<ParentType>().bind_child_storage(this);
    }

    template<typename... ParentTypes>
    void register_for_all_parent_types(basic_registry<entity_type>& reg, [[maybe_unused]] type_list<ParentTypes...>) {
        (add_self_for_type<ParentTypes>(reg), ...);
        add_self_for_type<component_type>(reg);
    }

    template<typename, typename>
    friend class poly_storage_mixin;
};

/**
 * @brief storage_traits template specialization for polymorphic component types
 * @tparam Entity entity type
 * @tparam Type polymorphic component type
 */
template<typename Entity, typename Type>
struct storage_traits<Entity, Type, std::enable_if_t<is_poly_type_v<Type>>> {
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
    using storage_type = poly_storage_mixin<basic_storage<Entity, guarded_type>>;
};

} // namespace entt

#endif // ENTT_ENTITY_POLYMORPHIC_HPP
