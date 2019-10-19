#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <tuple>
#include <cstring>
#include <utility>
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


template<typename Ret, typename... Args, typename Type, typename Payload, typename = std::enable_if_t<std::is_convertible_v<const Payload *, const Type *>>>
auto to_function_pointer(Ret(*)(Type &, Args...), const Payload *) -> Ret(*)(Args...);


template<typename Ret, typename... Args, typename Type, typename Payload, typename = std::enable_if_t<std::is_convertible_v<const Payload *, const Type *>>>
auto to_function_pointer(Ret(*)(Type *, Args...), const Payload *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...), const Class *) -> Ret(*)(Args...);


template<typename Class, typename Ret, typename... Args>
auto to_function_pointer(Ret(Class:: *)(Args...) const, const Class *) -> Ret(*)(Args...);


template<typename Class, typename Type>
auto to_function_pointer(Type Class:: *, const Class *) -> Type(*)();


template<typename... Type>
using to_function_pointer_t = decltype(internal::to_function_pointer(std::declval<Type>()...));


template<typename Ret, typename... Args>
constexpr auto index_sequence_for(Ret(*)(Args...)) {
    return std::index_sequence_for<Args...>{};
}


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
    using proto_fn_type = Ret(const void *, std::tuple<Args &&...>);

    template<auto Function, std::size_t... Index>
    void connect(std::index_sequence<Index...>) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Function), std::tuple_element_t<Index, std::tuple<Args...>>...>);
        data = nullptr;

        fn = [](const void *, std::tuple<Args &&...> args) -> Ret {
            // Ret(...) makes void(...) eat the return values to avoid errors
            return Ret(std::invoke(Function, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(args))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    void connect(Type &value_or_instance, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type &, std::tuple_element_t<Index, std::tuple<Args...>>...>);
        data = &value_or_instance;

        fn = [](const void *payload, std::tuple<Args &&...> args) -> Ret {
            Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
            // Ret(...) makes void(...) eat the return values to avoid errors
            return Ret(std::invoke(Candidate, *curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(args))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    void connect(Type *value_or_instance, std::index_sequence<Index...>) ENTT_NOEXCEPT {
        static_assert(std::is_invocable_r_v<Ret, decltype(Candidate), Type *, std::tuple_element_t<Index, std::tuple<Args...>>...>);
        data = value_or_instance;

        fn = [](const void *payload, std::tuple<Args &&...> args) -> Ret {
            Type *curr = static_cast<Type *>(const_cast<std::conditional_t<std::is_const_v<Type>, const void *, void *>>(payload));
            // Ret(...) makes void(...) eat the return values to avoid errors
            return Ret(std::invoke(Candidate, curr, std::forward<std::tuple_element_t<Index, std::tuple<Args...>>>(std::get<Index>(args))...));
        };
    }

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
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    delegate(connect_arg_t<Candidate>, Type &value_or_instance) ENTT_NOEXCEPT
        : delegate{}
    {
        connect<Candidate>(value_or_instance);
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
        connect<Function>(internal::index_sequence_for(internal::to_function_pointer_t<decltype(Function)>{}));
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
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type &value_or_instance) ENTT_NOEXCEPT {
        connect<Candidate>(value_or_instance, internal::index_sequence_for(internal::to_function_pointer_t<decltype(Candidate), Type *>{}));
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
        connect<Candidate>(value_or_instance, internal::index_sequence_for(internal::to_function_pointer_t<decltype(Candidate), Type *>{}));
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
     * @brief Returns the instance or the payload linked to a delegate, if any.
     * @return An opaque pointer to the underlying data.
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
        ENTT_ASSERT(fn);
        return fn(data, std::forward_as_tuple(std::forward<Args>(args)...));
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
     * @brief Compares the contents of two delegates.
     * @param other Delegate with which to compare.
     * @return False if the two contents differ, true otherwise.
     */
    bool operator==(const delegate<Ret(Args...)> &other) const ENTT_NOEXCEPT {
        return fn == other.fn && data == other.data;
    }

private:
    proto_fn_type *fn;
    const void *data;
};


/**
 * @brief Compares the contents of two delegates.
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the two contents differ, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of the delegate directly from a
 * function provided to the constructor.
 *
 * @tparam Function A valid free function pointer.
 */
template<auto Function>
delegate(connect_arg_t<Function>) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<internal::to_function_pointer_t<decltype(Function)>>>;


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of the delegate directly from a member
 * or a free function with payload provided to the constructor.
 *
 * @tparam Candidate Member or free function to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type &) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<internal::to_function_pointer_t<decltype(Candidate), Type *>>>;


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of the delegate directly from a member
 * or a free function with payload provided to the constructor.
 *
 * @tparam Candidate Member or free function to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type *) ENTT_NOEXCEPT
-> delegate<std::remove_pointer_t<internal::to_function_pointer_t<decltype(Candidate), Type *>>>;


}


#endif // ENTT_SIGNAL_DELEGATE_HPP
