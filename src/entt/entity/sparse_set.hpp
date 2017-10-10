#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP


#include <algorithm>
#include <utility>
#include <vector>
#include <cstddef>
#include <cassert>
#include "traits.hpp"


namespace entt {


/**
 * @brief Sparse set.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class SparseSet;


/**
 * @brief Basic sparse set implementation.
 *
 * Sparse set or packed array or whatever is the name users give it.<br/>
 * Two arrays: an _external_ one and an _internal_ one; a _sparse_ one and a
 * _packed_ one; one used for direct access through contiguous memory, the other
 * one used to get the data through an extra level of indirection.<br/>
 * This is largely used by the Registry to offer users the fastest access ever
 * to the components. View and PersistentView are entirely designed around
 * sparse sets.
 *
 * This type of data structure is widely documented in the literature and on the
 * web. This is nothing more than a customized implementation suitable for the
 * purpose of the framework.
 *
 * @note
 * There are no guarantees that entities are returned in the insertion order
 * when iterate a sparse set. Do not make assumption on the order in any case.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `data` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class SparseSet<Entity> {
    using traits_type = entt_traits<Entity>;

    struct Iterator {
        using value_type = Entity;

        Iterator(const std::vector<Entity> *direct, std::size_t pos)
            : direct{direct}, pos{pos}
        {}

        Iterator & operator++() noexcept {
            return --pos, *this;
        }

        Iterator operator++(int) noexcept {
            Iterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const Iterator &other) const noexcept {
            return other.pos == pos && other.direct == direct;
        }

        bool operator!=(const Iterator &other) const noexcept {
            return !(*this == other);
        }

        value_type operator*() const noexcept {
            return (*direct)[pos-1];
        }

    private:
        const std::vector<Entity> *direct;
        std::size_t pos;
    };

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Entity dependent position type. */
    using pos_type = entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator_type = Iterator;

    /*! @brief Default constructor, explicit on purpose. */
    explicit SparseSet() noexcept = default;

    /*! @brief Copying a sparse set isn't allowed. */
    SparseSet(const SparseSet &) = delete;
    /*! @brief Default move constructor. */
    SparseSet(SparseSet &&) = default;

    /*! @brief Default destructor. */
    virtual ~SparseSet() noexcept = default;

    /*! @brief Copying a sparse set isn't allowed. @return This sparse set. */
    SparseSet & operator=(const SparseSet &) = delete;
    /*! @brief Default move operator. @return This sparse set. */
    SparseSet & operator=(SparseSet &&) = default;

    /**
     * @brief Returns the number of elements in the sparse set.
     *
     * The number of elements is also the size of the internal packed array.
     * There is no guarantee that the internal sparse array has the same size.
     * Usually the size of the internal sparse array is equal or greater than
     * the one of the internal packed array.
     *
     * @return The number of elements.
     */
    size_type size() const noexcept {
        return direct.size();
    }

    /**
     * @brief Checks whether the sparse set is empty.
     * @return True is the sparse set is empty, false otherwise.
     */
    bool empty() const noexcept {
        return direct.empty();
    }

    /**
     * @brief Direct access to the internal packed array.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order, even though `sort` has been
     * previously invoked. Internal data structures arrange elements to maximize
     * performance. Accessing them directly gives a performance boost but less
     * guarantees. Use `begin` and `end` if you want to iterate the sparse set
     * in the expected order.
     *
     * @return A pointer to the internal packed array.
     */
    const entity_type * data() const noexcept {
        return direct.data();
    }

    /**
     * @brief Returns an iterator to the beginning.
     *
     * The returned iterator points to the first element of the internal packed
     * array. If the sparse set is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to `sort`.
     *
     * @return An iterator to the first element of the internal packed array.
     */
    iterator_type begin() const noexcept {
        return Iterator{&direct, direct.size()};
    }

    /**
     * @brief Returns an iterator to the end.
     *
     * The returned iterator points to the element following the last element in
     * the internal packed array. Attempting to dereference the returned
     * iterator results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed by a call to `sort`.
     *
     * @return An iterator to the element following the last element of the
     * internal packed array.
     */
    iterator_type end() const noexcept {
        return Iterator{&direct, 0};
    }

    /**
     * @brief Checks if the sparse set contains the given entity.
     * @param entity A valid entity identifier.
     * @return True if the sparse set contains the entity, false otherwise.
     */
    bool has(entity_type entity) const noexcept {
        const auto entt = entity & traits_type::entity_mask;
        return entt < reverse.size() && reverse[entt] < direct.size() && direct[reverse[entt]] == entity;
    }

    /**
     * @brief Returns the position of the entity in the sparse set.
     *
     * @warning
     * Attempting to get the position of an entity that doesn't belong to the
     * sparse set results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The position of the entity in the sparse set.
     */
    pos_type get(entity_type entity) const noexcept {
        assert(has(entity));
        return reverse[entity & traits_type::entity_mask];
    }

    /**
     * @brief Assigns an entity to the sparse set.
     *
     * @warning
     * Attempting to assign an entity that already belongs to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set already contains the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The position of the entity in the internal packed array.
     */
    pos_type construct(entity_type entity) {
        assert(!has(entity));
        const auto entt = entity & traits_type::entity_mask;

        if(!(entt < reverse.size())) {
            reverse.resize(entt+1);
        }

        const auto pos = pos_type(direct.size());
        reverse[entt] = pos;
        direct.emplace_back(entity);

        return pos;
    }

    /**
     * @brief Removes the given entity from the sparse set.
     *
     * @warning
     * Attempting to remove an entity that doesn't belong to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     */
    virtual void destroy(entity_type entity) {
        assert(has(entity));
        const auto entt = entity & traits_type::entity_mask;
        const auto back = direct.back() & traits_type::entity_mask;
        const auto pos = reverse[entt];
        reverse[back] = pos;
        direct[pos] = direct.back();
        direct.pop_back();
    }

    /**
     * @brief Swaps the position of the entities in the internal packed array.
     *
     * For what it's worth, this function affects both the internal sparse array
     * and the internal packed array. Users should not care of that anyway.
     *
     * @warning
     * Attempting to swap entities that don't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entities.
     *
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    virtual void swap(entity_type lhs, entity_type rhs) {
        assert(has(lhs));
        assert(has(rhs));
        const auto le = lhs & traits_type::entity_mask;
        const auto re = rhs & traits_type::entity_mask;
        std::swap(direct[reverse[le]], direct[reverse[re]]);
        std::swap(reverse[le], reverse[re]);
    }

    /**
     * @brief Sort entities according to the given comparison function.
     *
     * Sort the elements so that iterating the sparse set with a couple of
     * iterators returns them in the expected order. See `begin` and `end` for
     * more details.
     *
     * @note
     * Attempting to iterate elements using the raw pointer returned by `data`
     * gives no guarantees on the order, even though `sort` has been invoked.
     *
     * @tparam Compare The type of the comparison function.
     * @param compare A comparison function whose signature shall be equivalent
     * to: `bool(Entity, Entity)`.
     */
    template<typename Compare>
    void sort(Compare compare) {
        std::vector<pos_type> copy{direct.cbegin(), direct.cend()};
        std::sort(copy.begin(), copy.end(), [compare = std::move(compare)](auto... args) {
            return !compare(args...);
        });

        for(pos_type i = 0; i < copy.size(); ++i) {
            if(direct[i] != copy[i]) {
                swap(direct[i], copy[i]);
            }
        }
    }

    /**
     * @brief Sort entities according to their order in the given sparse set.
     *
     * Entities that are part of both the sparse sets are ordered internally
     * according to the order they have in `other`. All the other entities goes
     * to the end of the list and there are no guarantess on their order.<br/>
     * In other terms, this function can be used to impose the same order on two
     * sets by using one of them as a master and the other one as a slave.
     *
     * Iterating the sparse set with a couple of iterators returns elements in
     * the expected order after a call to `sort`. See `begin` and `end` for more
     * details.
     *
     * @note
     * Attempting to iterate elements using the raw pointer returned by `data`
     * gives no guarantees on the order, even though `sort` has been invoked.
     *
     * @param other The sparse sets that imposes the order of the entities.
     */
    void respect(const SparseSet<Entity> &other) {
        struct Bool { bool value{false}; };
        std::vector<Bool> check(std::max(other.reverse.size(), reverse.size()));

        for(auto entity: other.direct) {
            check[entity & traits_type::entity_mask].value = true;
        }

        sort([this, &other, &check](auto lhs, auto rhs) {
            const auto le = lhs & traits_type::entity_mask;
            const auto re = rhs & traits_type::entity_mask;

            const bool bLhs = check[le].value;
            const bool bRhs = check[re].value;
            bool compare = false;

            if(bLhs && bRhs) {
                compare = other.get(rhs) < other.get(lhs);
            } else if(!bLhs && !bRhs) {
                compare = re < le;
            } else {
                compare = bLhs;
            }

            return compare;
        });
    }

    /**
     * @brief Resets the sparse set.
     */
    virtual void reset() {
        reverse.clear();
        direct.clear();
    }

private:
    std::vector<entity_type> reverse;
    std::vector<entity_type> direct;
};


/**
 * @brief Extended sparse set implementation.
 *
 * This specialization of a sparse set associates an object to an entity. The
 * main purpose of this class is to use sparse sets to store components in a
 * Registry. It guarantees fast access both to the elements and to the entities.
 *
 * @note
 * Entities and objects have the same order. It's guaranteed both in case of raw
 * access (either to entities or objects) and when using input iterators.
 *
 * @note
 * Internal data structures arrange elements to maximize performance. Because of
 * that, there are no guarantees that elements have the expected order when
 * iterate directly the internal packed array (see `raw` and `size` member
 * functions for that). Use `begin` and `end` instead.
 *
 * @sa SparseSet<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type The type of the objects assigned to the entities.
 */
template<typename Entity, typename Type>
class SparseSet<Entity, Type>: public SparseSet<Entity> {
    using underlying_type = SparseSet<Entity>;

public:
    /*! @brief Type of the objects associated to the entities. */
    using type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;
    /*! @brief Entity dependent position type. */
    using pos_type = typename underlying_type::pos_type;
    /*! @brief Unsigned integer type. */
    using size_type = typename underlying_type::size_type;
    /*! @brief Input iterator type. */
    using iterator_type = typename underlying_type::iterator_type;

    /*! @brief Default constructor, explicit on purpose. */
    explicit SparseSet() noexcept = default;

    /*! @brief Copying a sparse set isn't allowed. */
    SparseSet(const SparseSet &) = delete;
    /*! @brief Default move constructor. */
    SparseSet(SparseSet &&) = default;

    /*! @brief Copying a sparse set isn't allowed. @return This sparse set. */
    SparseSet & operator=(const SparseSet &) = delete;
    /*! @brief Default move operator. @return This sparse set. */
    SparseSet & operator=(SparseSet &&) = default;

    /**
     * @brief Direct access to the array of objects.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order, even though `sort` has been
     * previously invoked. Internal data structures arrange elements to maximize
     * performance. Accessing them directly gives a performance boost but less
     * guarantees. Use `begin` and `end` if you want to iterate the sparse set
     * in the expected order.
     *
     * @return A pointer to the array of objects.
     */
    const type * raw() const noexcept {
        return instances.data();
    }

    /**
     * @brief Direct access to the array of objects.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order, even though `sort` has been
     * previously invoked. Internal data structures arrange elements to maximize
     * performance. Accessing them directly gives a performance boost but less
     * guarantees. Use `begin` and `end` if you want to iterate the sparse set
     * in the expected order.
     *
     * @return A pointer to the array of objects.
     */
    type * raw() noexcept {
        return instances.data();
    }

    /**
     * @brief Returns the object associated to the given entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The object associated to the entity.
     */
    const type & get(entity_type entity) const noexcept {
        return instances[underlying_type::get(entity)];
    }

    /**
     * @brief Returns the object associated to the given entity.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     * @return The object associated to the entity.
     */
    type & get(entity_type entity) noexcept {
        return const_cast<type &>(const_cast<const SparseSet *>(this)->get(entity));
    }

    /**
     * @brief Assigns an entity to the sparse set and constructs its object.
     *
     * @warning
     * Attempting to use an entity that already belongs to the sparse set
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set already contains the given entity.
     *
     * @tparam Args The type of the params used to construct the object.
     * @param entity A valid entity identifier.
     * @param args The params to use to construct an object for the entity.
     * @return The object associated to the entity.
     */
    template<typename... Args>
    type & construct(entity_type entity, Args&&... args) {
        underlying_type::construct(entity);
        instances.push_back({ std::forward<Args>(args)... });
        return instances.back();
    }

    /**
     * @brief Removes an entity from the sparse set and destroies its object.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entity.
     *
     * @param entity A valid entity identifier.
     */
    void destroy(entity_type entity) override {
        instances[underlying_type::get(entity)] = std::move(instances.back());
        instances.pop_back();
        underlying_type::destroy(entity);
    }

    /**
     * @brief Swaps the two entities and their objects.
     *
     * @note
     * This function doesn't swap objects between entities. It exchanges entity
     * and object positions in the sparse set. It's used mainly for sorting.
     *
     * @warning
     * Attempting to use entities that don't belong to the sparse set results
     * in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * sparse set doesn't contain the given entities.
     *
     * @param lhs A valid entity identifier.
     * @param rhs A valid entity identifier.
     */
    void swap(entity_type lhs, entity_type rhs) override {
        std::swap(instances[underlying_type::get(lhs)], instances[underlying_type::get(rhs)]);
        underlying_type::swap(lhs, rhs);
    }

    /**
     * @brief Resets the sparse set.
     */
    void reset() override {
        underlying_type::reset();
        instances.clear();
    }

private:
    std::vector<type> instances;
};


}


#endif // ENTT_ENTITY_SPARSE_SET_HPP
