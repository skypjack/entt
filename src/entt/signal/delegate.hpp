#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <cassert>
#include <algorithm>
#include <functional>
#include <type_traits>
#include "../config/config.h"


namespace entt {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename Ret, typename... Args>
auto to_function_pointer(Ret(*)(Args...)) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...)) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...) const) -> Ret(*)(Args...);


template<auto Func>
using function_type = std::remove_pointer_t<decltype(to_function_pointer(Func))>;


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/*! @brief Used to wrap a function or a member function of a specified type. */
template<auto>
struct connect_arg_t {};


/*! @brief Constant of type connect_arg_t used to disambiguate calls. */
template<auto Func>
inline static connect_arg_t<Func> connect_arg{};


/**
 * @brief Basic delegate implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename>
class delegate;


/**
 * @brief Utility class to use to send around functions and member functions.
 *
 * Unmanaged delegate for function pointers and member functions. Users of this
 * class are in charge of disconnecting instances before deleting them.
 *
 * A delegate can be used as general purpose invoker with no memory overhead for
 * free functions and member functions provided along with an instance on which
 * to invoke them. It comes also with limited support for curried functions.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> final {
    using storage_type = std::aligned_storage_t<sizeof(void *), alignof(void *)>;
    using proto_fn_type = Ret(storage_type &, Args...);

public:
    /*! @brief Function type of the delegate. */
    using function_type = Ret(Args...);

    /*! @brief Default constructor. */
    delegate() ENTT_NOEXCEPT
        : storage{}, fn{nullptr}
    {
        new (&storage) void *{nullptr};
    }

    /**
     * @brief Constructs a delegate and connects a free function to it.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Function>();
    }

    /**
     * @brief Constructs a delegate and connects a member function to it.
     * @tparam Member Member function to connect to the delegate.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<auto Member, typename Class, typename = std::enable_if_t<std::is_member_function_pointer_v<decltype(Member)>>>
    delegate(connect_arg_t<Member>, Class *instance) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Member>(instance);
    }

    /**
     * @brief Connects a free function to a delegate.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), Args...>);
        new (&storage) void *{nullptr};

        fn = [](storage_type &, Args... args) -> Ret {
            return std::invoke(Function, args...);
        };
    }

    /**
     * @brief Connects a member function for a given instance or a curried free
     * function to a delegate.
     *
     * When used to connect a member function, the delegate isn't responsible
     * for the connected object. Users must guarantee that the lifetime of the
     * instance overcomes the one of the delegate.<br/>
     * When used to connect a curried free function, the linked value must be at
     * least copyable and such that its size is lower than or equal to the one
     * of a `void *`. It means that all primitive types are accepted as well as
     * pointers. Moreover, the signature of the free function must be such that
     * the value is the first argument before the ones used to define the
     * delegate itself.
     *
     * @tparam Candidate Member function or curried free function to connect to
     * the delegate.
     * @tparam Type Type of class to which the member function belongs or type
     * of value used for currying.
     * @param value_or_instance A valid pointer to an instance of class type or
     * the value to use for currying.
     */
    template<auto Candidate, typename Type>
    void connect(Type value_or_instance) ENTT_NOEXCEPT {
        static_assert(sizeof(Type) <= sizeof(void *));
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type, Args...>);
        new (&storage) Type{value_or_instance};

        fn = [](storage_type &storage, Args... args) -> Ret {
            Type value_or_instance = *reinterpret_cast<Type *>(&storage);
            return std::invoke(Candidate, value_or_instance, args...);
        };
    }

    /**
     * @brief Resets a delegate.
     *
     * After a reset, a delegate cannot be invoked anymore.
     */
    void reset() ENTT_NOEXCEPT {
        new (&storage) void *{nullptr};
        fn = nullptr;
    }

    /**
     * @brief Returns the instance linked to a delegate, if any.
     *
     * @warning
     * Attempting to use an instance returned by a delegate that doesn't contain
     * a pointer to a member function results in undefined behavior.
     *
     * @return An opaque pointer to the instance linked to the delegate, if any.
     */
    const void * instance() const ENTT_NOEXCEPT {
        return *reinterpret_cast<const void **>(&storage);
    }

    /**
     * @brief Triggers a delegate.
     *
     * The delegate invokes the underlying function and returns the result.
     *
     * @warning
     * Attempting to trigger an invalid delegate results in undefined
     * behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * delegate has not yet been set.
     *
     * @param args Arguments to use to invoke the underlying function.
     * @return The value returned by the underlying function.
     */
    Ret operator()(Args... args) const {
        assert(fn);
        return fn(storage, args...);
    }

    /**
     * @brief Checks whether a delegate actually stores a listener.
     * @return False if the delegate is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        // no need to test also data
        return fn;
    }

    /**
     * @brief Checks if the contents of the two delegates are different.
     * @param other Delegate with which to compare.
     * @return True if the two delegates are identical, false otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        const char *lhs = reinterpret_cast<const char *>(&storage);
        const char *rhs = reinterpret_cast<const char *>(&other.storage);
        return fn == other.fn && std::equal(lhs, lhs + sizeof(storage_type), rhs);
    }

private:
    mutable storage_type storage;
    proto_fn_type *fn;
};


/**
 * @brief Checks if the contents of the two delegates are different.
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the two delegates are different, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Deduction guideline.
 *
 * It allows to deduce the function type of the delegate directly from the
 * function provided to the constructor.
 *
 * @tparam Function A valid free function pointer.
 */
template<auto Function>
delegate(connect_arg_t<Function>) ENTT_NOEXCEPT -> delegate<internal::function_type<Function>>;


/**

 * @brief Deduction guideline.
 *
 * It allows to deduce the function type of the delegate directly from the
 * member function provided to the constructor.
 *
 * @tparam Member Member function to connect to the delegate.
 * @tparam Class Type of class to which the member function belongs.
 */
template<auto Member, typename Class>
delegate(connect_arg_t<Member>, Class *) ENTT_NOEXCEPT -> delegate<internal::function_type<Member>>;


}


#endif // ENTT_SIGNAL_DELEGATE_HPP
