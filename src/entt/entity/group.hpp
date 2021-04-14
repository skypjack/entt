#ifndef ENTT_ENTITY_GROUP_HPP
#define ENTT_ENTITY_GROUP_HPP


#include <tuple>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"
#include "utility.hpp"


namespace entt {


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
 * A non-owning group returns all entities and only the entities that have at
 * least the given components. Moreover, it's guaranteed that the entity list
 * is tightly packed in memory for fast iterations.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Groups share references to the underlying data structures of the registry
 * that generated them. Therefore any change to the entities and to the
 * components made by means of the registry are immediately reflected by all the
 * groups.<br/>
 * Moreover, sorting a non-owning group affects all the instances of the same
 * group (it means that users don't have to call `sort` on each instance to sort
 * all of them because they _share_ entities and components).
 *
 * @warning
 * Lifetime of a group must not overcome that of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the group.
 * @tparam Get Type of components observed by the group.
 */
template<typename Entity, typename... Exclude, typename... Get>
class basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>> final {
    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type, Component>;

    class iterable_group final {
        friend class basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>>;

        template<typename It>
        class iterable_group_iterator final {
            friend class iterable_group;

            template<typename... Args>
            iterable_group_iterator(It from, const std::tuple<storage_type<Get> *...> &args) ENTT_NOEXCEPT
                : it{from},
                  pools{args}
            {}

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = decltype(std::tuple_cat(std::tuple<Entity>{}, std::declval<basic_group>().get({})));
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            iterable_group_iterator & operator++() ENTT_NOEXCEPT {
                return ++it, *this;
            }

            iterable_group_iterator operator++(int) ENTT_NOEXCEPT {
                iterable_group_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
                const auto entt = *it;
                return std::tuple_cat(std::make_tuple(entt), get_as_tuple(*std::get<storage_type<Get> *>(pools), entt)...);
            }

            [[nodiscard]] bool operator==(const iterable_group_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const iterable_group_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            It it;
            std::tuple<storage_type<Get> *...> pools;
        };

        iterable_group(basic_sparse_set<Entity> * const ref, const std::tuple<storage_type<Get> *...> &cpools)
            : handler{ref},
              pools{cpools}
        {}

    public:
        using iterator = iterable_group_iterator<typename basic_sparse_set<Entity>::iterator>;
        using reverse_iterator = iterable_group_iterator<typename basic_sparse_set<Entity>::reverse_iterator>;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return handler ? iterator{handler->begin(), pools} : iterator{{}, pools};
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return handler ? iterator{handler->end(), pools} : iterator{{}, pools};
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return handler ? reverse_iterator{handler->rbegin(), pools} : reverse_iterator{{}, pools};
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return handler ? reverse_iterator{handler->rend(), pools} : reverse_iterator{{}, pools};
        }

    private:
        basic_sparse_set<Entity> * const handler;
        const std::tuple<storage_type<Get> *...> pools;
    };

    basic_group(basic_sparse_set<Entity> &ref, storage_type<Get> &... gpool) ENTT_NOEXCEPT
        : handler{&ref},
          pools{&gpool...}
    {}

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = typename basic_sparse_set<Entity>::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename basic_sparse_set<Entity>::reverse_iterator;

    /*! @brief Default constructor to use to create empty, invalid groups. */
    basic_group() ENTT_NOEXCEPT
        : handler{}
    {}

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return *this ? handler->size() : size_type{};
    }

    /**
     * @brief Returns the number of elements that a group has currently
     * allocated space for.
     * @return Capacity of the group.
     */
    [[nodiscard]] size_type capacity() const ENTT_NOEXCEPT {
        return *this ? handler->capacity() : size_type{};
    }

    /*! @brief Requests the removal of unused capacity. */
    void shrink_to_fit() {
        if(*this) {
            handler->shrink_to_fit();
        }
    }

    /**
     * @brief Checks whether a group is empty.
     * @return True if the group is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return !*this || handler->empty();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return *this ? handler->data() : nullptr;
    }

    /**
     * @brief Returns an iterator to the first entity of the group.
     *
     * The returned iterator points to the first entity of the group. If the
     * group is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the group.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return *this ? handler->begin() : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the group.
     *
     * The returned iterator points to the entity following the last entity of
     * the group. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * group.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return *this ? handler->end() : iterator{};
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed group.
     *
     * The returned iterator points to the first entity of the reversed group.
     * If the group is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed group.
     */
    [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return *this ? handler->rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * group.
     *
     * The returned iterator points to the entity following the last entity of
     * the reversed group. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * reversed group.
     */
    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return *this ? handler->rend() : reverse_iterator{};
    }

    /**
     * @brief Returns the first entity of the group, if any.
     * @return The first entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the group, if any.
     * @return The last entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        const auto it = *this ? handler->find(entt) : iterator{};
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group is properly initialized.
     * @return True if the group is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return handler != nullptr;
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return *this && handler->contains(entt);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.
     *
     * @tparam Component Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 0) {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Get> *>(pools), entt)...);
        } else if constexpr(sizeof...(Component) == 1) {
            return (std::get<storage_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Component> *>(pools), entt)...);
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        for(const auto entt: *this) {
            if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_group>().get({})))>) {
                std::apply(func, std::tuple_cat(std::make_tuple(entt), get(entt)));
            } else {
                std::apply(func, get(entt));
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ the group.
     *
     * The iterable object returns tuples that contain the current entity and a
     * set of references to its non-empty components. The _constness_ of the
     * components is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the group.
     */
    [[nodiscard]] iterable_group each() const ENTT_NOEXCEPT {
        return iterable_group{handler, pools};
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
     * bool(std::tuple<Component &...>, std::tuple<Component &...>);
     * bool(const Component &..., const Component &...);
     * bool(const Entity, const Entity);
     * @endcode
     *
     * Where `Component` are such that they are iterated by the group.<br/>
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
        if(*this) {
            if constexpr(sizeof...(Component) == 0) {
                static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
                handler->sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
            }  else if constexpr(sizeof...(Component) == 1) {
                handler->sort([this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                    return compare((std::get<storage_type<Component> *>(pools)->get(lhs), ...), (std::get<storage_type<Component> *>(pools)->get(rhs), ...));
                }, std::move(algo), std::forward<Args>(args)...);
            } else {
                handler->sort([this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                    return compare(std::forward_as_tuple(std::get<storage_type<Component> *>(pools)->get(lhs)...), std::forward_as_tuple(std::get<storage_type<Component> *>(pools)->get(rhs)...));
                }, std::move(algo), std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Sort the shared pool of entities according to the given component.
     *
     * Non-owning groups of the same type share with the registry a pool of
     * entities with its own order that doesn't depend on the order of any pool
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
        if(*this) {
            handler->respect(*std::get<storage_type<Component> *>(pools));
        }
    }

private:
    basic_sparse_set<entity_type> * const handler;
    const std::tuple<storage_type<Get> *...> pools;
};


/**
 * @brief Owning group.
 *
 * Owning groups return all entities and only the entities that have at least
 * the given components. Moreover:
 *
 * * It's guaranteed that the entity list is tightly packed in memory for fast
 *   iterations.
 * * It's guaranteed that the lists of owned components are tightly packed in
 *   memory for even faster iterations and to allow direct access.
 * * They stay true to the order of the owned components and all instances have
 *   the same order in memory.
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
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the group in any way
 * invalidates all the iterators and using them results in undefined behavior.
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
 * Lifetime of a group must not overcome that of the registry that generated it.
 * In any other case, attempting to use a group results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the group.
 * @tparam Get Types of components observed by the group.
 * @tparam Owned Types of components owned by the group.
 */
template<typename Entity, typename... Exclude, typename... Get, typename... Owned>
class basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...> final {
    /*! @brief A registry is allowed to create groups. */
    friend class basic_registry<Entity>;

    template<typename Component>
    using storage_type = constness_as_t<typename storage_traits<Entity, std::remove_const_t<Component>>::storage_type, Component>;

    class iterable_group final {
        friend class basic_group<Entity, exclude_t<Exclude...>, get_t<Get...>, Owned...>;

        template<typename, typename>
        class iterable_group_iterator;

        template<typename It, typename... OIt>
        class iterable_group_iterator<It, type_list<OIt...>> final {
            friend class iterable_group;

            template<typename... Other>
            iterable_group_iterator(It from, const std::tuple<Other...> &other, const std::tuple<storage_type<Get> *...> &cpools) ENTT_NOEXCEPT
                : it{from},
                  owned{std::get<OIt>(other)...},
                  get{cpools}
            {}

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = decltype(std::tuple_cat(std::tuple<Entity>{}, std::declval<basic_group>().get({})));
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            iterable_group_iterator & operator++() ENTT_NOEXCEPT {
                return ++it, (++std::get<OIt>(owned), ...), *this;
            }

            iterable_group_iterator operator++(int) ENTT_NOEXCEPT {
                iterable_group_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const ENTT_NOEXCEPT {
                return std::tuple_cat(
                    std::make_tuple(*it),
                    std::forward_as_tuple(*std::get<OIt>(owned)...),
                    get_as_tuple(*std::get<storage_type<Get> *>(get), *it)...
                );
            }

            [[nodiscard]] bool operator==(const iterable_group_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const iterable_group_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            It it;
            std::tuple<OIt...> owned;
            std::tuple<storage_type<Get> *...> get;
        };

        iterable_group(std::tuple<storage_type<Owned> *..., storage_type<Get> *...> cpools, const std::size_t * const extent)
            : pools{cpools},
              length{extent}
        {}

    public:
        using iterator = iterable_group_iterator<
            typename basic_sparse_set<Entity>::iterator,
            type_list_cat_t<std::conditional_t<std::is_void_v<decltype(std::declval<storage_type<Owned>>().get({}))>, type_list<>, type_list<decltype(std::declval<storage_type<Owned>>().end())>>...>
        >;
        using reverse_iterator = iterable_group_iterator<
            typename basic_sparse_set<Entity>::reverse_iterator,
            type_list_cat_t<std::conditional_t<std::is_void_v<decltype(std::declval<storage_type<Owned>>().get({}))>, type_list<>, type_list<decltype(std::declval<storage_type<Owned>>().rbegin())>>...>
        >;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return length ? iterator{
                std::get<0>(pools)->basic_sparse_set<Entity>::end() - *length,
                std::make_tuple((std::get<storage_type<Owned> *>(pools)->end() - *length)...),
                std::make_tuple(std::get<storage_type<Get> *>(pools)...)
            } : iterator{{}, std::make_tuple(decltype(std::get<storage_type<Owned> *>(pools)->end()){}...), std::make_tuple(std::get<storage_type<Get> *>(pools)...)};
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return length ? iterator{
                std::get<0>(pools)->basic_sparse_set<Entity>::end(),
                std::make_tuple((std::get<storage_type<Owned> *>(pools)->end())...),
                std::make_tuple(std::get<storage_type<Get> *>(pools)...)
            } : iterator{{}, std::make_tuple(decltype(std::get<storage_type<Owned> *>(pools)->end()){}...), std::make_tuple(std::get<storage_type<Get> *>(pools)...)};
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return length ? reverse_iterator{
                std::get<0>(pools)->basic_sparse_set<Entity>::rbegin(),
                std::make_tuple((std::get<storage_type<Owned> *>(pools)->rbegin())...),
                std::make_tuple(std::get<storage_type<Get> *>(pools)...)
            } : reverse_iterator{{}, std::make_tuple(decltype(std::get<storage_type<Owned> *>(pools)->rbegin()){}...), std::make_tuple(std::get<storage_type<Get> *>(pools)...)};
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return length ? reverse_iterator{
                std::get<0>(pools)->basic_sparse_set<Entity>::rbegin() + *length,
                std::make_tuple((std::get<storage_type<Owned> *>(pools)->rbegin() + *length)...),
                std::make_tuple(std::get<storage_type<Get> *>(pools)...)
            } : reverse_iterator{{}, std::make_tuple(decltype(std::get<storage_type<Owned> *>(pools)->rbegin()){}...), std::make_tuple(std::get<storage_type<Get> *>(pools)...)};
        }

    private:
        const std::tuple<storage_type<Owned> *..., storage_type<Get> *...> pools;
        const std::size_t * const length;
    };

    basic_group(const std::size_t &extent, storage_type<Owned> &... opool, storage_type<Get> &... gpool) ENTT_NOEXCEPT
        : pools{&opool..., &gpool...},
          length{&extent}
    {}

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = typename basic_sparse_set<Entity>::iterator;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = typename basic_sparse_set<Entity>::reverse_iterator;

    /*! @brief Default constructor to use to create empty, invalid groups. */
    basic_group() ENTT_NOEXCEPT
        : length{}
    {}

    /**
     * @brief Returns the number of entities that have the given components.
     * @return Number of entities that have the given components.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return *this ? *length : size_type{};
    }

    /**
     * @brief Checks whether a group is empty.
     * @return True if the group is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return !*this || !*length;
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Component>(), raw<Component>() + size())` is always a valid range,
     * even if the container is empty.<br/>
     *
     * @warning
     * This function is only available for owned types.
     *
     * @tparam Component Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Component>
    [[nodiscard]] Component * raw() const ENTT_NOEXCEPT {
        static_assert((std::is_same_v<Component, Owned> || ...), "Non-owned type");
        auto *cpool = std::get<storage_type<Component> *>(pools);
        return cpool ? cpool->raw() : nullptr;
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type * data() const ENTT_NOEXCEPT {
        return *this ? std::get<0>(pools)->data() : nullptr;
    }

    /**
     * @brief Returns an iterator to the first entity of the group.
     *
     * The returned iterator points to the first entity of the group. If the
     * group is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the group.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return *this ? (std::get<0>(pools)->basic_sparse_set<entity_type>::end() - *length) : iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the group.
     *
     * The returned iterator points to the entity following the last entity of
     * the group. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * group.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return *this ? std::get<0>(pools)->basic_sparse_set<entity_type>::end() : iterator{};
    }

    /**
     * @brief Returns an iterator to the first entity of the reversed group.
     *
     * The returned iterator points to the first entity of the reversed group.
     * If the group is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the reversed group.
     */
    [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
        return *this ? std::get<0>(pools)->basic_sparse_set<entity_type>::rbegin() : reverse_iterator{};
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * group.
     *
     * The returned iterator points to the entity following the last entity of
     * the reversed group. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * reversed group.
     */
    [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
        return *this ? (std::get<0>(pools)->basic_sparse_set<entity_type>::rbegin() + *length) : reverse_iterator{};
    }

    /**
     * @brief Returns the first entity of the group, if any.
     * @return The first entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the group, if any.
     * @return The last entity of the group if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        const auto it = *this ? std::get<0>(pools)->find(entt) : iterator{};
        return it != end() && it >= begin() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a group is properly initialized.
     * @return True if the group is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
        return length != nullptr;
    }

    /**
     * @brief Checks if a group contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the group contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return *this && std::get<0>(pools)->contains(entt) && (std::get<0>(pools)->index(entt) < (*length));
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the group
     * results in undefined behavior.
     *
     * @tparam Component Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Component>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Component) == 0) {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Owned> *>(pools), entt)..., get_as_tuple(*std::get<storage_type<Get> *>(pools), entt)...);
        } else if constexpr(sizeof...(Component) == 1) {
            return (std::get<storage_type<Component> *>(pools)->get(entt), ...);
        } else {
            return std::tuple_cat(get_as_tuple(*std::get<storage_type<Component> *>(pools), entt)...);
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        for(auto args: each()) {
            if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_group>().get({})))>) {
                std::apply(func, args);
            } else {
                std::apply([&func](auto, auto &&... less) { func(std::forward<decltype(less)>(less)...); }, args);
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ the group.
     *
     * The iterable object returns tuples that contain the current entity and a
     * set of references to its non-empty components. The _constness_ of the
     * components is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the group.
     */
    [[nodiscard]] iterable_group each() const ENTT_NOEXCEPT {
        return iterable_group{pools, length};
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
     * bool(std::tuple<Component &...>, std::tuple<Component &...>);
     * bool(const Component &, const Component &);
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
     * @tparam Component Optional types of components to compare.
     * @tparam Compare Type of comparison function object.
     * @tparam Sort Type of sort function object.
     * @tparam Args Types of arguments to forward to the sort function object.
     * @param compare A valid comparison function object.
     * @param algo A valid sort function object.
     * @param args Arguments to forward to the sort function object, if any.
     */
    template<typename... Component, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&... args) const {
        auto *cpool = std::get<0>(pools);
        
        if constexpr(sizeof...(Component) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
            cpool->sort_n(*length, std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else if constexpr(sizeof...(Component) == 1) {
            cpool->sort_n(*length, [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                return compare((std::get<storage_type<Component> *>(pools)->get(lhs), ...), (std::get<storage_type<Component> *>(pools)->get(rhs), ...));
            }, std::move(algo), std::forward<Args>(args)...);
        } else {
            cpool->sort_n(*length, [this, compare = std::move(compare)](const entity_type lhs, const entity_type rhs) {
                return compare(std::forward_as_tuple(std::get<storage_type<Component> *>(pools)->get(lhs)...), std::forward_as_tuple(std::get<storage_type<Component> *>(pools)->get(rhs)...));
            }, std::move(algo), std::forward<Args>(args)...);
        }
        
        [this](auto *head, auto *... other) {
            for(auto next = *length; next; --next) {
                const auto pos = next - 1;
                [[maybe_unused]] const auto entt = head->data()[pos];
                (other->swap(other->data()[pos], entt), ...);
            }
        }(std::get<storage_type<Owned> *>(pools)...);
    }

private:
    const std::tuple<storage_type<Owned> *..., storage_type<Get> *...> pools;
    const size_type * const length;
};


}


#endif
