#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <cassert>
#include <cstring>
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


template<typename Ret, typename... Args, typename Type>
auto to_function_pointer(Ret(*)(Type *, Args...), Type *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...), Class *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...) const, Class *) -> Ret(*)(Args...);


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/*! @brief Used to wrap a function or a member of a specified type. */
template<auto>
struct connect_arg_t {};


/*! @brief Constant of type connect_arg_t used to disambiguate calls. */
template<auto Func>
constexpr connect_arg_t<Func> connect_arg{};


/**
 * @brief Basic delegate implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename>
class delegate;


/**
 * @brief Utility class to use to send around functions and members.
 *
 * Unmanaged delegate for function pointers and members. Users of this class are
 * in charge of disconnecting instances before deleting them.
 *
 * A delegate can be used as general purpose invoker with no memory overhead for
 * free functions (with or without payload) and members provided along with an
 * instance on which to invoke them.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> {
    using proto_fn_type = Ret(const void *, Args...);

public:
    /*! @brief Function type of the delegate. */
    using function_type = Ret(Args...);

    /*! @brief Default constructor. */
    delegate() ENTT_NOEXCEPT
        : fn{nullptr}, data{nullptr}
    {}

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
     * @brief Constructs a delegate and connects a member for a given instance
     * or a free function with payload.
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type *value_or_instance) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>(value_or_instance);
    }

    /**
     * @brief Connects a free function to a delegate.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), Args...>);
        data = nullptr;

        fn = [](const void *, Args... args) -> Ret {
            // this allows void(...) to eat return values and avoid errors
            return Ret(std::invoke(Function, args...));
        };
    }

    /**
     * @brief Connects a member function for a given instance or a free function
     * with payload to a delegate.
     *
     * The delegate isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type *, Args...>);
        data = value_or_instance;

        fn = [](const void *payload, Args... args) -> Ret {
            Type *curr = nullptr;

            if constexpr(std::is_const_v<Type>) {
                curr = static_cast<Type *>(payload);
            } else {
                curr = static_cast<Type *>(const_cast<void *>(payload));
            }

            // this allows void(...) to eat return values and avoid errors
            return Ret(std::invoke(Candidate, curr, args...));
        };
    }

    /**
     * @brief Resets a delegate.
     *
     * After a reset, a delegate cannot be invoked anymore.
     */
    void reset() ENTT_NOEXCEPT {
        fn = nullptr;
        data = nullptr;
    }

    /**
     * @brief Returns the instance linked to a delegate, if any.
     * @return An opaque pointer to the instance linked to the delegate, if any.
     */
    const void * instance() const ENTT_NOEXCEPT {
        return data;
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
        return fn(data, args...);
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
     * @brief Checks if the connected functions differ.
     *
     * Instances connected to delegates are ignored by this operator. Use the
     * `instance` member function instead.
     *
     * @param other Delegate with which to compare.
     * @return False if the connected functions differ, true otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        return fn == other.fn;
    }

private:
    proto_fn_type *fn;
    const void *data;
};


/**
 * @brief Checks if the connected functions differ.
 *
 * Instances connected to delegates are ignored by this operator. Use the
 * `instance` member function instead.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the connected functions differ, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Deduction guideline.
 *
 * It allows to deduce the function type of the delegate directly from a
 * function provided to the constructor.
 *
 * @tparam Function A valid free function pointer.
 */
template<auto Function>
delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Function))>>;


/**
 * @brief Deduction guideline.
 *
 * It allows to deduce the function type of the delegate directly from a member
 * or a free function with payload provided to the constructor.
 *
 * @tparam Candidate Member or free function to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type *) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<decltype(internal::to_function_pointer(Candidate, std::declval<Type *>()))>>;


}


#endif // ENTT_SIGNAL_DELEGATE_HPP
