#ifndef ENTT_ENTITY_ATTACHEE_HPP
#define ENTT_ENTITY_ATTACHEE_HPP


#include <cassert>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "entity.hpp"


namespace entt {


/**
 * @brief Attachee.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class Attachee;


/**
 * @brief Basic attachee implementation.
 *
 * Convenience data structure used to store single instance components.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class Attachee<Entity> {
public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;

    /*! @brief Default constructor. */
    Attachee() ENTT_NOEXCEPT
        : owner{null}
    {}

    /*! @brief Default copy constructor. */
    Attachee(const Attachee &) = default;
    /*! @brief Default move constructor. */
    Attachee(Attachee &&) = default;

    /*! @brief Default copy assignment operator. @return This attachee. */
    Attachee & operator=(const Attachee &) = default;
    /*! @brief Default move assignment operator. @return This attachee. */
    Attachee & operator=(Attachee &&) = default;

    /*! @brief Default destructor. */
    virtual ~Attachee() ENTT_NOEXCEPT = default;

    /**
     * @brief Returns the owner of an attachee.
     * @return A valid entity identifier if an owner exists, the null entity
     * identifier otherwise.
     */
    inline entity_type get() const ENTT_NOEXCEPT {
        return owner;
    }

    /**
     * @brief Assigns an entity to an attachee.
     *
     * @warning
     * Attempting to assigns an entity to an attachee that already has an owner
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the attachee already has an owner.
     *
     * @param entity A valid entity identifier.
     */
    inline void construct(const entity_type entity) ENTT_NOEXCEPT {
        assert(owner == null);
        owner = entity;
    }

    /**
     * @brief Removes an entity from an attachee.
     *
     * @warning
     * Attempting to free an empty attachee results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * attachee is already empty.
     */
    virtual void destroy() ENTT_NOEXCEPT {
        assert(owner != null);
        owner = null;
    }

private:
    entity_type owner;
};


/**
 * @brief Extended attachee implementation.
 *
 * This specialization of an attachee associates an object to an entity. The
 * main purpose of this class is to use attachees to store tags in a Registry.
 * It guarantees fast access both to the element and to the entity.
 *
 * @sa Attachee<Entity>
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Type Type of object assigned to the entity.
 */
template<typename Entity, typename Type>
class Attachee<Entity, Type>: public Attachee<Entity> {
    using underlying_type = Attachee<Entity>;

public:
    /*! @brief Type of the object associated to the attachee. */
    using object_type = Type;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename underlying_type::entity_type;

    /*! @brief Default constructor. */
    Attachee() ENTT_NOEXCEPT = default;

    /*! @brief Copying an attachee isn't allowed. */
    Attachee(const Attachee &) = delete;
    /*! @brief Moving an attachee isn't allowed. */
    Attachee(Attachee &&) = delete;

    /*! @brief Copying an attachee isn't allowed. @return This attachee. */
    Attachee & operator=(const Attachee &) = delete;
    /*! @brief Moving an attachee isn't allowed. @return This attachee. */
    Attachee & operator=(Attachee &&) = delete;

    /*! @brief Default destructor. */
    ~Attachee() {
        if(underlying_type::get() != null) {
            reinterpret_cast<Type *>(&storage)->~Type();
        }
    }

    /**
     * @brief Returns the object associated to an attachee.
     *
     * @warning
     * Attempting to query an empty attachee results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * attachee is empty.
     *
     * @return The object associated to the attachee.
     */
    const Type & get() const ENTT_NOEXCEPT {
        assert(underlying_type::get() != null);
        return *reinterpret_cast<const Type *>(&storage);
    }

    /**
     * @brief Returns the object associated to an attachee.
     *
     * @warning
     * Attempting to query an empty attachee results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * attachee is empty.
     *
     * @return The object associated to the attachee.
     */
    Type & get() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const Attachee *>(this)->get());
    }

    /**
     * @brief Assigns an entity to an attachee and constructs its object.
     *
     * @warning
     * Attempting to assigns an entity to an attachee that already has an owner
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the attachee already has an owner.
     *
     * @tparam Args Types of arguments to use to construct the object.
     * @param entity A valid entity identifier.
     * @param args Parameters to use to construct an object for the entity.
     * @return The object associated to the attachee.
     */
    template<typename... Args>
    Type & construct(entity_type entity, Args &&... args) ENTT_NOEXCEPT {
        underlying_type::construct(entity);
        new (&storage) Type{std::forward<Args>(args)...};
        return *reinterpret_cast<Type *>(&storage);
    }

    /**
     * @brief Removes an entity from an attachee and destroies its object.
     *
     * @warning
     * Attempting to free an empty attachee results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * attachee is already empty.
     */
    void destroy() ENTT_NOEXCEPT override {
        reinterpret_cast<Type *>(&storage)->~Type();
        underlying_type::destroy();
    }

    /**
     * @brief Changes the owner of an attachee.
     *
     * The ownership of the attachee is transferred from one entity to another.
     *
     * @warning
     * Attempting to transfer the ownership of an attachee that hasn't an owner
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode in case
     * the attachee hasn't an owner yet.
     *
     * @param entity A valid entity identifier.
     */
    void move(const entity_type entity) ENTT_NOEXCEPT {
        underlying_type::destroy();
        underlying_type::construct(entity);
    }

private:
    std::aligned_storage_t<sizeof(Type), alignof(Type)> storage;
};


}


#endif // ENTT_ENTITY_ATTACHEE_HPP
