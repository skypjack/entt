#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Ret, typename... Args>
constexpr auto function_pointer(Ret (*)(Args...)) -> Ret (*)(Args...);

template<typename Ret, typename Type, typename... Args, typename Other>
constexpr auto function_pointer(Ret (*)(Type, Args...), Other &&) -> Ret (*)(Args...);

template<typename Class, typename Ret, typename... Args, typename... Other>
constexpr auto function_pointer(Ret (Class::*)(Args...), Other &&...) -> Ret (*)(Args...);

template<typename Class, typename Ret, typename... Args, typename... Other>
constexpr auto function_pointer(Ret (Class::*)(Args...) const, Other &&...) -> Ret (*)(Args...);

template<typename Class, typename Type, typename... Other, typename = std::enable_if_t<std::is_member_object_pointer_v<Type Class::*>>>
constexpr auto function_pointer(Type Class::*, Other &&...) -> Type (*)();

template<typename... Type>
using function_pointer_t = decltype(function_pointer(std::declval<Type>()...));

template<typename... Class, typename Ret, typename... Args>
[[nodiscard]] constexpr auto index_sequence_for(Ret (*)(Args...)) {
    return std::index_sequence_for<Class..., Args...>{};
}

} // namespace internal
/*! @endcond */

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
 * A delegate can be used as a general purpose invoker without memory overhead
 * for free functions possibly with payloads and bound or unbound members.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class delegate<Ret(Args...)> {
    using return_type = std::remove_const_t<Ret>;
    using delegate_type = return_type(const void *, Args...);

    template<auto Candidate, std::size_t... Index>
    [[nodiscard]] auto wrap(std::index_sequence<Index...>) noexcept {
        return [](const void *, Args... args) -> return_type {
            [[maybe_unused]] const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            [[maybe_unused]] constexpr auto offset = !std::is_invocable_r_v<Ret, decltype(Candidate), type_list_element_t<Index, type_list<Args...>>...> * (sizeof...(Args) - sizeof...(Index));
            return static_cast<Ret>(std::invoke(Candidate, std::forward<type_list_element_t<Index + offset, type_list<Args...>>>(std::get<Index + offset>(arguments))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    [[nodiscard]] auto wrap(Type &, std::index_sequence<Index...>) noexcept {
        return [](const void *payload, Args... args) -> return_type {
            Type *curr = static_cast<Type *>(const_cast<constness_as_t<void, Type> *>(payload));
            [[maybe_unused]] const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            [[maybe_unused]] constexpr auto offset = !std::is_invocable_r_v<Ret, decltype(Candidate), Type &, type_list_element_t<Index, type_list<Args...>>...> * (sizeof...(Args) - sizeof...(Index));
            return static_cast<Ret>(std::invoke(Candidate, *curr, std::forward<type_list_element_t<Index + offset, type_list<Args...>>>(std::get<Index + offset>(arguments))...));
        };
    }

    template<auto Candidate, typename Type, std::size_t... Index>
    [[nodiscard]] auto wrap(Type *, std::index_sequence<Index...>) noexcept {
        return [](const void *payload, Args... args) -> return_type {
            Type *curr = static_cast<Type *>(const_cast<constness_as_t<void, Type> *>(payload));
            [[maybe_unused]] const auto arguments = std::forward_as_tuple(std::forward<Args>(args)...);
            [[maybe_unused]] constexpr auto offset = !std::is_invocable_r_v<Ret, decltype(Candidate), Type *, type_list_element_t<Index, type_list<Args...>>...> * (sizeof...(Args) - sizeof...(Index));
            return static_cast<Ret>(std::invoke(Candidate, curr, std::forward<type_list_element_t<Index + offset, type_list<Args...>>>(std::get<Index + offset>(arguments))...));
        };
    }

public:
    /*! @brief Function type of the contained target. */
    using function_type = Ret(const void *, Args...);
    /*! @brief Function type of the delegate. */
    using type = Ret(Args...);
    /*! @brief Return type of the delegate. */
    using result_type = Ret;

    /*! @brief Default constructor. */
    delegate() noexcept = default;

    /**
     * @brief Constructs a delegate with a given object or payload, if any.
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload, if any.
     * @param value_or_instance Optional valid object that fits the purpose.
     */
    template<auto Candidate, typename... Type>
    delegate(connect_arg_t<Candidate>, Type &&...value_or_instance) noexcept {
        connect<Candidate>(std::forward<Type>(value_or_instance)...);
    }

    /**
     * @brief Constructs a delegate and connects an user defined function with
     * optional payload.
     * @param function Function to connect to the delegate.
     * @param payload User defined arbitrary data.
     */
    delegate(function_type *function, const void *payload = nullptr) noexcept {
        connect(function, payload);
    }

    /**
     * @brief Connects a free function or an unbound member to a delegate.
     * @tparam Candidate Function or member to connect to the delegate.
     */
    template<auto Candidate>
    void connect() noexcept {
        instance = nullptr;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Args...>) {
            fn = [](const void *, Args... args) -> return_type {
                return Ret(std::invoke(Candidate, std::forward<Args>(args)...));
            };
        } else if constexpr(std::is_member_pointer_v<decltype(Candidate)>) {
            fn = wrap<Candidate>(internal::index_sequence_for<type_list_element_t<0, type_list<Args...>>>(internal::function_pointer_t<decltype(Candidate)>{}));
        } else {
            fn = wrap<Candidate>(internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate)>{}));
        }
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * delegate.
     *
     * The delegate isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one of the delegate.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid reference that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type &value_or_instance) noexcept {
        instance = &value_or_instance;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Type &, Args...>) {
            fn = [](const void *payload, Args... args) -> return_type {
                Type *curr = static_cast<Type *>(const_cast<constness_as_t<void, Type> *>(payload));
                return Ret(std::invoke(Candidate, *curr, std::forward<Args>(args)...));
            };
        } else {
            fn = wrap<Candidate>(value_or_instance, internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate), Type>{}));
        }
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * delegate.
     *
     * @sa connect(Type &)
     *
     * @tparam Candidate Function or member to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) noexcept {
        instance = value_or_instance;

        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Type *, Args...>) {
            fn = [](const void *payload, Args... args) -> return_type {
                Type *curr = static_cast<Type *>(const_cast<constness_as_t<void, Type> *>(payload));
                return Ret(std::invoke(Candidate, curr, std::forward<Args>(args)...));
            };
        } else {
            fn = wrap<Candidate>(value_or_instance, internal::index_sequence_for(internal::function_pointer_t<decltype(Candidate), Type>{}));
        }
    }

    /**
     * @brief Connects an user defined function with optional payload to a
     * delegate.
     *
     * The delegate isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of an instance overcomes
     * the one of the delegate.<br/>
     * The payload is returned as the first argument to the target function in
     * all cases.
     *
     * @param function Function to connect to the delegate.
     * @param payload User defined arbitrary data.
     */
    void connect(function_type *function, const void *payload = nullptr) noexcept {
        ENTT_ASSERT(function != nullptr, "Uninitialized function pointer");
        instance = payload;
        fn = function;
    }

    /**
     * @brief Resets a delegate.
     *
     * After a reset, a delegate cannot be invoked anymore.
     */
    void reset() noexcept {
        instance = nullptr;
        fn = nullptr;
    }

    /**
     * @brief Returns a pointer to the stored callable function target, if any.
     * @return An opaque pointer to the stored callable function target.
     */
    [[nodiscard]] function_type *target() const noexcept {
        return fn;
    }

    /**
     * @brief Returns the instance or the payload linked to a delegate, if any.
     * @return An opaque pointer to the underlying data.
     */
    [[nodiscard]] const void *data() const noexcept {
        return instance;
    }

    /**
     * @brief Triggers a delegate.
     *
     * The delegate invokes the underlying function and returns the result.
     *
     * @warning
     * Attempting to trigger an invalid delegate results in undefined
     * behavior.
     *
     * @param args Arguments to use to invoke the underlying function.
     * @return The value returned by the underlying function.
     */
    Ret operator()(Args... args) const {
        ENTT_ASSERT(static_cast<bool>(*this), "Uninitialized delegate");
        return fn(instance, std::forward<Args>(args)...);
    }

    /**
     * @brief Checks whether a delegate actually stores a listener.
     * @return False if the delegate is empty, true otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        // no need to also test instance
        return !(fn == nullptr);
    }

    /**
     * @brief Compares the contents of two delegates.
     * @param other Delegate with which to compare.
     * @return False if the two contents differ, true otherwise.
     */
    [[nodiscard]] bool operator==(const delegate<Ret(Args...)> &other) const noexcept {
        return fn == other.fn && instance == other.instance;
    }

private:
    const void *instance{};
    delegate_type *fn{};
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
[[nodiscard]] bool operator!=(const delegate<Ret(Args...)> &lhs, const delegate<Ret(Args...)> &rhs) noexcept {
    return !(lhs == rhs);
}

/**
 * @brief Deduction guide.
 * @tparam Candidate Function or member to connect to the delegate.
 */
template<auto Candidate>
delegate(connect_arg_t<Candidate>) -> delegate<std::remove_pointer_t<internal::function_pointer_t<decltype(Candidate)>>>;

/**
 * @brief Deduction guide.
 * @tparam Candidate Function or member to connect to the delegate.
 * @tparam Type Type of class or type of payload.
 */
template<auto Candidate, typename Type>
delegate(connect_arg_t<Candidate>, Type &&) -> delegate<std::remove_pointer_t<internal::function_pointer_t<decltype(Candidate), Type>>>;

/**
 * @brief Deduction guide.
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
delegate(Ret (*)(const void *, Args...), const void * = nullptr) -> delegate<Ret(Args...)>;

} // namespace entt

#endif
