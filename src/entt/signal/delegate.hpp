#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <utility>
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
 * @brief Utility class to send around functions and member functions.
 *
 * Unmanaged delegate for function pointers and member functions. Users of this
 * class are in charge of disconnecting instances before deleting them.
 *
 * A delegate can be used as general purpose invoker with no memory overhead for
 * free functions and member functions provided along with an instance on which
 * to invoke them.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> final {
    using proto_fn_type = Ret(void *, Args...);
    using stub_type = std::pair<void *, proto_fn_type *>;

    template<auto Function>
    static Ret proto(void *, Args... args) {
        return std::invoke(Function, args...);
    }

    template<typename Class, auto Member>
    static Ret proto(void *instance, Args... args) {
        return std::invoke(Member, static_cast<Class *>(instance), args...);
    }

public:
    /*! @brief Function type of the delegate. */
    using function_type = Ret(Args...);

    /*! @brief Default constructor. */
    delegate() ENTT_NOEXCEPT
        : stub{}
    {}

    /**
     * @brief Constructs a delegate and binds a free function to it.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
        : stub{}
    {
        connect<Function>();
    }

    /**
     * @brief Constructs a delegate and binds a member function to it.
     * @tparam Member Member function to connect to the delegate.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<auto Member, typename Class>
    delegate(connect_arg_t<Member>, Class *instance) ENTT_NOEXCEPT
        : stub{}
    {
        connect<Member>(instance);
    }

    /**
     * @brief Binds a free function to a delegate.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), Args...>);
        stub = std::make_pair(nullptr, &proto<Function>);
    }

    /**
     * @brief Connects a member function for a given instance to a delegate.
     *
     * The delegate isn't responsible for the connected object. Users must
     * guarantee that the lifetime of the instance overcomes the one of the
     * delegate.
     *
     * @tparam Member Member function to connect to the delegate.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<auto Member, typename Class>
    void connect(Class *instance) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Member), Class, Args...>);
        stub = std::make_pair(instance, &proto<Class, Member>);
    }

    /**
     * @brief Resets a delegate.
     *
     * After a reset, a delegate can be safely invoked with no effect.
     */
    void reset() ENTT_NOEXCEPT {
        stub.second = nullptr;
    }

    /**
     * @brief Returns the instance bound to a delegate, if any.
     * @return An opaque pointer to the instance bound to the delegate, if any.
     */
    const void * instance() const ENTT_NOEXCEPT {
        return stub.first;
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
        return stub.second(stub.first, args...);
    }

    /**
     * @brief Checks whether a delegate actually stores a listener.
     * @return False if the delegate is empty, true otherwise.
     */
    explicit operator bool() const ENTT_NOEXCEPT {
        // no need to test also stub.first
        return stub.second;
    }

    /**
     * @brief Checks if the contents of the two delegates are different.
     *
     * Two delegates are identical if they contain the same listener.
     *
     * @param other Delegate with which to compare.
     * @return True if the two delegates are identical, false otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        return stub.first == other.stub.first && stub.second == other.stub.second;
    }

private:
    stub_type stub{};
};


/**
 * @brief Checks if the contents of the two delegates are different.
 *
 * Two delegates are identical if they contain the same listener.
 *
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
