#ifndef ENTT_ENTITY_GROUP_HPP
#define ENTT_ENTITY_GROUP_HPP


#include <tuple>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Alias for lists of observed components.
 * @tparam Type List of types.
 */
template<typename... Type>
struct get_t: type_list<Type...> {};


/**
 * @brief Variable template for lists of observed components.
 * @tparam Type List of types.
 */
template<typename... Type>
constexpr get_t<Type...> get{};


/**
 * @brief Group.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class basic_group;


/**
 * @brief Non-owning group.
 *
 * A non-owning group returns all the entities and only the entities that have
 * at least the given components. Moreover, it's guaranteed that the entity list
 * is tightly packed in memory for fast iterations.<br/>
 * In general, non-owning groups don't stay true to the order of any set of
 * components unless users explicitly sort them.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @note
 * Groups share references to the underlying data structures of the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by all the
 * groups.<br/>
 * Moreover, sorting a non-owning group affects all the instance of the same
 * group (it means that users don't have to call `sort` on each instance to sort
 * all of them because they share the set of entities).
 *
 * @warning
 * Lifetime of a group must overcome the one of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Get Types of components observed by the group.
 */
template<typename Entity, typename... Get>
class basic_group<Entity, get_t<Get...>> {
    static_assert(sizeof...(Get) > 0);

    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    // we could use pool_type<Get> *..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    basic_group(sparse_set<Entity> *ref, storage<Entity, std::remove_const_t<Get>> *... get) ENTT_NOEXCEPT
        : handler{ref},
          pools{get...}
    {}

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename sparse_set<Entity>::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->size();
    }

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return handler->size();
    }

    /**
     * @brief Returns the number of elements that a group has currently
     * allocated space for.
     * @return Capacity of the group.
     */
    size_type capacity() const ENTT_NOEXCEPT {
        return handler->capacity();
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        handler->shrink_to_fit();
    }

    /**
     * @brief Checks whether the pool of a given component is empty.
     * @tparam Component Type of component in which one is interested.
     * @return True if the pool of the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->empty();
    }

    /**
     * @brief Checks whether the group is empty.
     * @return True if the group is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return handler->empty();
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Component>
    Component * raw() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->data();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return handler->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the group is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        return handler->begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return handler->end();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        const auto it = handler->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return find(entt) != end();
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * group doesn't contain the given entity.
     *
     * @tparam Component Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 1) {
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Component>(entt))...>{get<Component>(entt)...};
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Get &...);
     * void(Get &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void each(Func func) const {
        for(const auto entt: *handler) {
            if constexpr(std::is_invocable_v<Func, decltype(get<Get>({}))...>) {
                func(std::get<pool_type<Get> *>(pools)->get(entt)...);
            } else {
                func(entt, std::get<pool_type<Get> *>(pools)->get(entt)...);
            }
        };
    }

    /**
     * @brief Sort the shared pool of entities according to the given component.
     *
     * Non-owning groups of the same type share with the registry a pool of
     * entities with  its own order that doesn't depend on the order of any pool
     * of components. Users can order the underlying data structure so that it
     * respects the order of the pool of the given component.
     *
     * @note
     * The shared pool of entities and thus its order is affected by the changes
     * to each and every pool that it tracks. Therefore changes to those pools
     * can quickly ruin the order imposed to the pool of entities shared between
     * the non-owning groups.
     *
     * @tparam Component Type of component to use to impose the order.
     */
    template<typename Component>
    void sort() const {
        handler->respect(*std::get<pool_type<Component> *>(pools));
    }

private:
    sparse_set<entity_type> *handler;
    const std::tuple<pool_type<Get> *...> pools;
};


/**
 * @brief Owning group.
 *
 * Owning groups return all the entities and only the entities that have at
 * least the given components. Moreover:
 *
 * * It's guaranteed that the entity list is tightly packed in memory for fast
 *   iterations.
 * * It's guaranteed that the lists of owned components are tightly packed in
 *   memory for even faster iterations and to allow direct access.
 * * They stay true to the order of the owned components and all the owned
 *   components have the same order in memory.
 *
 * The more types of components are owned by a group, the faster it is to
 * iterate them.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @note
 * Groups share references to the underlying data structures of the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by all the
 * groups.
 * Moreover, sorting an owning group affects all the instance of the same group
 * (it means that users don't have to call `sort` on each instance to sort all
 * of them because they share the underlying data structure).
 *
 * @warning
 * Lifetime of a group must overcome the one of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Get Types of components observed by the group.
 * @tparam Owned Types of components owned by the group.
 */
template<typename Entity, typename... Get, typename... Owned>
class basic_group<Entity, get_t<Get...>, Owned...> {
    static_assert(sizeof...(Get) + sizeof...(Owned) > 0);

    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    template<typename Component>
    using component_iterator_type = decltype(std::declval<pool_type<Component>>().begin());

    template<typename Component>
    decltype(auto) from_index(const typename sparse_set<Entity>::size_type index) {
        static_assert(!std::is_empty_v<Component>);

        if constexpr(std::disjunction_v<std::is_same<Component, Owned>...>) {
            return std::as_const(*std::get<pool_type<Component> *>(pools)).raw()[index];
        } else {
            return std::as_const(*std::get<pool_type<Component> *>(pools)).get(data()[index]);
        }
    }

    template<typename Component>
    inline auto swap(int, pool_type<Component> *cpool, const std::size_t lhs, const std::size_t rhs)
    -> decltype(cpool->raw(), void()) {
        std::swap(cpool->raw()[lhs], cpool->raw()[rhs]);
        cpool->swap(lhs, rhs);
    }

    template<typename Component>
    inline void swap(char, pool_type<Component> *cpool, const std::size_t lhs, const std::size_t rhs) {
        cpool->swap(lhs, rhs);
    }

    // we could use pool_type<Type> *..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    basic_group(const typename basic_registry<Entity>::size_type *sz, storage<Entity, std::remove_const_t<Owned>> *... owned, storage<Entity, std::remove_const_t<Get>> *... get) ENTT_NOEXCEPT
        : length{sz},
          pools{owned..., get...}
    {}

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = typename sparse_set<Entity>::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename sparse_set<Entity>::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /**
     * @brief Returns the number of existing components of the given type.
     * @tparam Component Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Component>
    size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->size();
    }

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    size_type size() const ENTT_NOEXCEPT {
        return *length;
    }

    /**
     * @brief Checks whether the pool of a given component is empty.
     * @tparam Component Type of component in which one is interested.
     * @return True if the pool of the given component is empty, false
     * otherwise.
     */
    template<typename Component>
    bool empty() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->empty();
    }

    /**
     * @brief Checks whether the group is empty.
     * @return True if the group is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return !*length;
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.<br/>
     * Moreover, in case the group owns the given component, the range
     * `[raw<Component>(), raw<Component>() + size()]` is such that it contains
     * the instances that are part of the group itself.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Component>
    Component * raw() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Component>(), data<Component>() + size<Component>()]` is always a
     * valid range, even if the container is empty.<br/>
     * Moreover, in case the group owns the given component, the range
     * `[data<Component>(), data<Component>() + size()]` is such that it
     * contains the entities that are part of the group itself.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Component>
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<pool_type<Component> *>(pools)->data();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the group in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the group is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->sparse_set<entity_type>::end() - *length;
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return std::get<0>(pools)->sparse_set<entity_type>::end();
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const ENTT_NOEXCEPT {
        const auto it = std::get<0>(pools)->find(entt);
        return it != end() && it >= begin() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const ENTT_NOEXCEPT {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const ENTT_NOEXCEPT {
        return find(entt) != end();
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its companion function.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * group doesn't contain the given entity.
     *
     * @tparam Component Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Component>
    decltype(auto) get([[maybe_unused]] const entity_type entt) const ENTT_NOEXCEPT {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 1) {
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Component>(entt))...>{get<Component>(entt)...};
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Owned &..., Get &...);
     * void(Owned &..., Get &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    inline void each(Func func) const {
        auto raw = std::make_tuple((std::get<pool_type<Owned> *>(pools)->end() - *length)...);
        [[maybe_unused]] auto data = std::get<0>(pools)->sparse_set<entity_type>::end() - *length;

        for(auto next = *length; next; --next) {
            if constexpr(std::is_invocable_v<Func, decltype(get<Owned>({}))..., decltype(get<Get>({}))...>) {
                if constexpr(sizeof...(Get) == 0) {
                    func(*(std::get<component_iterator_type<Owned>>(raw)++)...);
                } else {
                    const auto entt = *(data++);
                    func(*(std::get<component_iterator_type<Owned>>(raw)++)..., std::get<pool_type<Get> *>(pools)->get(entt)...);
                }
            } else {
                const auto entt = *(data++);
                func(entt, *(std::get<component_iterator_type<Owned>>(raw)++)..., std::get<pool_type<Get> *>(pools)->get(entt)...);
            }
        }
    }

    /**
     * @brief Sort a group according to the given comparison function.
     *
     * Sort the group so that iterating it with a couple of iterators returns
     * entities and components in the expected order. See `begin` and `end` for
     * more details.
     *
     * The comparison function object must return `true` if the first element
     * is _less_ than the second one, `false` otherwise. The signature of the
     * comparison function should be equivalent to one of the following:
     *
     * @code{.cpp}
     * bool(const Component &..., const Component &...);
     * bool(const Entity, const Entity);
     * @endcode
     *
     * Where `Component` are either owned types or not but still such that they
     * are iterated by the group.<br/>
     * Moreover, the comparison function object shall induce a
     * _strict weak ordering_ on the values.
     *
     * The sort function oject must offer a member function template
     * `operator()` that accepts three arguments:
     *
     * * An iterator to the first element of the range to sort.
     * * An iterator past the last element of the range to sort.
     * * A comparison function to use to compare the elements.
     *
     * The comparison function object received by the sort function object
     * hasn't necessarily the type of the one passed along with the other
     * parameters to this member function.
     *
     * @note
     * Attempting to iterate elements using a raw pointer returned by a call to
     * either `data` or `raw` gives no guarantees on the order, even though
     * `sort` has been invoked.
     *
     * @tparam Component Optional types of components to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename... Component, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) {
        std::vector<size_type> copy(*length);
        std::iota(copy.begin(), copy.end(), 0);

        if constexpr(sizeof...(Component) == 0) {
            algo(copy.rbegin(), copy.rend(), [compare = std::move(compare), data = data()](const auto lhs, const auto rhs) {
                return compare(data[lhs], data[rhs]);
            }, std::forward<Args>(args)...);
        } else {
            algo(copy.rbegin(), copy.rend(), [compare = std::move(compare), this](const auto lhs, const auto rhs) {
                // useless this-> used to suppress a warning with clang
                return compare(this->from_index<Component>(lhs)..., this->from_index<Component>(rhs)...);
            }, std::forward<Args>(args)...);
        }

        for(size_type pos = 0, last = copy.size(); pos < last; ++pos) {
            auto curr = pos;
            auto next = copy[curr];

            while(curr != next) {
                const auto lhs = copy[curr];
                const auto rhs = copy[next];
                (swap<Owned>(0, std::get<pool_type<Owned> *>(pools), lhs, rhs), ...);
                copy[curr] = curr;
                curr = next;
                next = copy[curr];
            }
        }
    }

private:
    const typename basic_registry<Entity>::size_type *length;
    const std::tuple<pool_type<Owned> *..., pool_type<Get> *...> pools;
};


}


#endif // ENTT_ENTITY_GROUP_HPP
