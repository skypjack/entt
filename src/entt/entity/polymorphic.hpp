#ifndef ENTT_ENTITY_POLYMORPHIC_HPP
#define ENTT_ENTITY_POLYMORPHIC_HPP

#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"

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

/**
 * @brief Holds runtime information about one polymorphic component type.
 * @tparam Entity
 */
template<typename Entity>
struct poly_type {
    /** @brief Holds pointer to child type storage and a function to convert it into parent type */
    struct poly_pool_holder {
        /**
         * @brief gets child from a child pool as parent type reference by given entity
         * @tparam Type parent type to convert into
         * @param ptr pointer from the child type set
         * @return Returns reference to Type, converted from a given pointer. Will return pointer instead of reference, if parent type is a pointer.
         */
        template<typename Type>
        inline decltype(auto) try_get(const Entity ent) ENTT_NOEXCEPT {
            return static_cast<std::remove_pointer_t<Type>*>(getter_ptr(pool_ptr, ent));
        }

        inline auto& pool() ENTT_NOEXCEPT {
            return *pool_ptr;
        }

        inline const auto& pool() const ENTT_NOEXCEPT {
            return *pool_ptr;
        }

    private:
        inline poly_pool_holder(basic_sparse_set<Entity>* pool, void* (*getter)(void*, Entity) ENTT_NOEXCEPT) :
              pool_ptr(pool), getter_ptr(getter) {};

        template<typename>
        friend struct poly_type;

    private:
        basic_sparse_set<Entity>* pool_ptr;
        void* (*getter_ptr)(void*, Entity) ENTT_NOEXCEPT;
    };

    /**
     * @brief From a given set, makes a poly_storage_holder to hold child type and convert to parent type.
     * @tparam SourceType source type to convert from = child type
     * @tparam TargetType target type to convert to = parent type.
     * @param pool_ptr child (source) type pool
     * @return poly_storage_holder to hold SourceType set and access it as TargetType
     */
    template<typename SourceType, typename TargetType, typename StorageType = typename storage_traits<Entity, SourceType>::storage_type>
    inline static poly_pool_holder make_pool_holder(StorageType* pool_ptr) ENTT_NOEXCEPT {
        static_assert(std::is_pointer_v<SourceType> == std::is_pointer_v<TargetType>);

        void* (*get)(void*, Entity entity) ENTT_NOEXCEPT = +[](void* pool, const Entity entity) ENTT_NOEXCEPT -> void* {
            if (static_cast<StorageType*>(pool)->contains(entity)) {
                // if entity is contained within the set
                if constexpr(std::is_base_of_v<std::remove_pointer_t<TargetType>, std::remove_pointer_t<SourceType>>) {
                    if constexpr(std::is_pointer_v<SourceType>) {
                        // TargetType and SourceType are pointers, dereference source pointer and do conversion
                        SourceType ptr = static_cast<StorageType*>(pool)->get(entity);
                        return static_cast<TargetType>(ptr);
                    } else {
                        // TargetType is base of SourceType, do pointer conversion
                        return static_cast<TargetType*>(std::addressof(static_cast<StorageType*>(pool)->get(entity)));
                    }
                } else {
                    // no inheritance - no conversion required
                    if constexpr(std::is_pointer_v<SourceType>) {
                        // in case of pointer type return it as it is
                        return static_cast<StorageType*>(pool)->get(entity);
                    } else {
                        //
                        return std::addressof(static_cast<StorageType*>(pool)->get(entity));
                    }
                }
            }
            // otherwise, return null
            return nullptr;
        };
        return poly_pool_holder(pool_ptr, get);
    }

    inline auto& child_pools() ENTT_NOEXCEPT {
        return child_pool_holders;
    }

    inline const auto& child_pools() const ENTT_NOEXCEPT {
        return child_pool_holders;
    }

private:
    std::vector<poly_pool_holder> child_pool_holders{};
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
        return assure(type_id<Type>().hash());
    }
private:
    dense_map<id_type, poly_type<Entity>, identity> types{};
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
        auto& type = reg.poly_data().template assure<ParentType>();
        type.child_pools().push_back(poly_type<entity_type>::template make_pool_holder<component_type, ParentType>(this));
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
