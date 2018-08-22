#ifndef ENTT_CORE_MONOSTATE_HPP
#define ENTT_CORE_MONOSTATE_HPP


#include <atomic>
#include <cassert>
#include "family.hpp"
#include "hashed_string.hpp"


namespace entt {


/**
 * @brief Minimal implementation of the monostate pattern.
 *
 * A minimal, yet complete configuration system built on top of the monostate
 * pattern. Thread safe by design, it works only with basic types like `int`s or
 * `bool`s.<br/>
 * Multiple types and therefore more than one value can be associated with a
 * single key. Because of this, users must pay attention to use the same type
 * both during an assignment and when they try to read back their data.
 * Otherwise, they can incur in unexpected results.
 */
template<HashedString::hash_type>
struct Monostate {
    /*! @brief Default constructor. Workaround for VS2015: fixes list initialization of this class. */
    Monostate() {}

    /**
     * @brief Assigns a value of a specific type to a given key.
     * @tparam Type Type of the value to assign.
     * @param val User data to assign to the given key.
     */
    template<typename Type>
    void operator=(Type val) const ENTT_NOEXCEPT {
        Monostate::value<Type> = val;
    }

    /**
     * @brief Gets a value of a specific type for a given key.
     * @tparam Type Type of the value to get.
     * @return Stored value, if any.
     */
    template<typename Type>
    operator Type() const ENTT_NOEXCEPT {
        return Monostate::value<Type>;
    }

private:
    template<typename Type>
    static std::atomic<Type> value;
};


template<HashedString::hash_type ID>
template<typename Type>
std::atomic<Type> Monostate<ID>::value = {}; // Workaround for VS2015: No list initialization


}


#endif // ENTT_CORE_MONOSTATE_HPP
