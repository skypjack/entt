#ifndef ENTT_META_UTILITY_HPP
#define ENTT_META_UTILITY_HPP

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include "../core/type_traits.hpp"
#include "../locator/locator.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "policy.hpp"

namespace entt {

/**
 * @brief Meta function descriptor traits.
 * @tparam Ret Function return type.
 * @tparam Args Function arguments.
 * @tparam Static Function staticness.
 * @tparam Const Function constness.
 */
template<typename Ret, typename Args, bool Static, bool Const>
struct meta_function_descriptor_traits {
    /*! @brief Meta function return type. */
    using return_type = Ret;
    /*! @brief Meta function arguments. */
    using args_type = Args;

    /*! @brief True if the meta function is static, false otherwise. */
    static constexpr bool is_static = Static;
    /*! @brief True if the meta function is const, false otherwise. */
    static constexpr bool is_const = Const;
};

/*! @brief Primary template isn't defined on purpose. */
template<typename, typename>
struct meta_function_descriptor;

/**
 * @brief Meta function descriptor.
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Ret Function return type.
 * @tparam Class Actual owner of the member function.
 * @tparam Args Function arguments.
 */
template<typename Type, typename Ret, typename Class, typename... Args>
struct meta_function_descriptor<Type, Ret (Class::*)(Args...) const>
    : meta_function_descriptor_traits<
          Ret,
          std::conditional_t<std::is_base_of_v<Class, Type>, type_list<Args...>, type_list<const Class &, Args...>>,
          !std::is_base_of_v<Class, Type>,
          true> {};

/**
 * @brief Meta function descriptor.
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Ret Function return type.
 * @tparam Class Actual owner of the member function.
 * @tparam Args Function arguments.
 */
template<typename Type, typename Ret, typename Class, typename... Args>
struct meta_function_descriptor<Type, Ret (Class::*)(Args...)>
    : meta_function_descriptor_traits<
          Ret,
          std::conditional_t<std::is_base_of_v<Class, Type>, type_list<Args...>, type_list<Class &, Args...>>,
          !std::is_base_of_v<Class, Type>,
          false> {};

/**
 * @brief Meta function descriptor.
 * @tparam Type Reflected type to which the meta data is associated.
 * @tparam Class Actual owner of the data member.
 * @tparam Ret Data member type.
 */
template<typename Type, typename Ret, typename Class>
struct meta_function_descriptor<Type, Ret Class::*>
    : meta_function_descriptor_traits<
          Ret &,
          std::conditional_t<std::is_base_of_v<Class, Type>, type_list<>, type_list<Class &>>,
          !std::is_base_of_v<Class, Type>,
          false> {};

/**
 * @brief Meta function descriptor.
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Ret Function return type.
 * @tparam MaybeType First function argument.
 * @tparam Args Other function arguments.
 */
template<typename Type, typename Ret, typename MaybeType, typename... Args>
struct meta_function_descriptor<Type, Ret (*)(MaybeType, Args...)>
    : meta_function_descriptor_traits<
          Ret,
          std::conditional_t<
              std::is_same_v<std::remove_cv_t<std::remove_reference_t<MaybeType>>, Type> || std::is_base_of_v<std::remove_cv_t<std::remove_reference_t<MaybeType>>, Type>,
              type_list<Args...>,
              type_list<MaybeType, Args...>>,
          !(std::is_same_v<std::remove_cv_t<std::remove_reference_t<MaybeType>>, Type> || std::is_base_of_v<std::remove_cv_t<std::remove_reference_t<MaybeType>>, Type>),
          std::is_const_v<std::remove_reference_t<MaybeType>> && (std::is_same_v<std::remove_cv_t<std::remove_reference_t<MaybeType>>, Type> || std::is_base_of_v<std::remove_cv_t<std::remove_reference_t<MaybeType>>, Type>)> {};

/**
 * @brief Meta function descriptor.
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Ret Function return type.
 */
template<typename Type, typename Ret>
struct meta_function_descriptor<Type, Ret (*)()>
    : meta_function_descriptor_traits<
          Ret,
          type_list<>,
          true,
          false> {};

/**
 * @brief Meta function helper.
 *
 * Converts a function type to be associated with a reflected type into its meta
 * function descriptor.
 *
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Candidate The actual function to associate with the reflected type.
 */
template<typename Type, typename Candidate>
class meta_function_helper {
    template<typename Ret, typename... Args, typename Class>
    static constexpr meta_function_descriptor<Type, Ret (Class::*)(Args...) const> get_rid_of_noexcept(Ret (Class::*)(Args...) const);

    template<typename Ret, typename... Args, typename Class>
    static constexpr meta_function_descriptor<Type, Ret (Class::*)(Args...)> get_rid_of_noexcept(Ret (Class::*)(Args...));

    template<typename Ret, typename Class, typename = std::enable_if_t<std::is_member_object_pointer_v<Ret Class::*>>>
    static constexpr meta_function_descriptor<Type, Ret Class::*> get_rid_of_noexcept(Ret Class::*);

    template<typename Ret, typename... Args>
    static constexpr meta_function_descriptor<Type, Ret (*)(Args...)> get_rid_of_noexcept(Ret (*)(Args...));

    template<typename Class>
    static constexpr meta_function_descriptor<Class, decltype(&Class::operator())> get_rid_of_noexcept(Class);

public:
    /*! @brief The meta function descriptor of the given function. */
    using type = decltype(get_rid_of_noexcept(std::declval<Candidate>()));
};

/**
 * @brief Helper type.
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Candidate The actual function to associate with the reflected type.
 */
template<typename Type, typename Candidate>
using meta_function_helper_t = typename meta_function_helper<Type, Candidate>::type;

/**
 * @brief Wraps a value depending on the given policy.
 *
 * This function always returns a wrapped value in the requested context.<br/>
 * Therefore, if the passed value is itself a wrapped object with a different
 * context, it undergoes a rebinding to the requested context.
 *
 * @tparam Policy Optional policy (no policy set by default).
 * @tparam Type Type of value to wrap.
 * @param ctx The context from which to search for meta types.
 * @param value Value to wrap.
 * @return A meta any containing the returned value, if any.
 */
template<typename Policy = as_is_t, typename Type>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_dispatch(const meta_ctx &ctx, [[maybe_unused]] Type &&value) {
    if constexpr(std::is_same_v<Policy, as_void_t>) {
        return meta_any{ctx, std::in_place_type<void>};
    } else if constexpr(std::is_same_v<Policy, as_ref_t>) {
        return meta_any{ctx, std::in_place_type<Type>, value};
    } else if constexpr(std::is_same_v<Policy, as_cref_t>) {
        static_assert(std::is_lvalue_reference_v<Type>, "Invalid type");
        return meta_any{ctx, std::in_place_type<const std::remove_reference_t<Type> &>, std::as_const(value)};
    } else {
        return meta_any{ctx, std::forward<Type>(value)};
    }
}

/**
 * @brief Wraps a value depending on the given policy.
 * @tparam Policy Optional policy (no policy set by default).
 * @tparam Type Type of value to wrap.
 * @param value Value to wrap.
 * @return A meta any containing the returned value, if any.
 */
template<typename Policy = as_is_t, typename Type>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_dispatch(Type &&value) {
    return meta_dispatch<Policy, Type>(locator<meta_ctx>::value_or(), std::forward<Type>(value));
}

/**
 * @brief Returns the meta type of the i-th element of a list of arguments.
 * @tparam Type Type list of the actual types of arguments.
 * @param ctx The context from which to search for meta types.
 * @param index The index of the element for which to return the meta type.
 * @return The meta type of the i-th element of the list of arguments.
 */
template<typename Type>
[[nodiscard]] static meta_type meta_arg(const meta_ctx &ctx, const std::size_t index) noexcept {
    auto &&context = internal::meta_context::from(ctx);
    return {ctx, internal::meta_arg_node(context, Type{}, index)};
}

/**
 * @brief Returns the meta type of the i-th element of a list of arguments.
 * @tparam Type Type list of the actual types of arguments.
 * @param index The index of the element for which to return the meta type.
 * @return The meta type of the i-th element of the list of arguments.
 */
template<typename Type>
[[nodiscard]] static meta_type meta_arg(const std::size_t index) noexcept {
    return meta_arg<Type>(locator<meta_ctx>::value_or(), index);
}

/**
 * @brief Sets the value of a given variable.
 * @tparam Type Reflected type to which the variable is associated.
 * @tparam Data The actual variable to set.
 * @param instance An opaque instance of the underlying type, if required.
 * @param value Parameter to use to set the variable.
 * @return True in case of success, false otherwise.
 */
template<typename Type, auto Data>
[[nodiscard]] bool meta_setter([[maybe_unused]] meta_handle instance, [[maybe_unused]] meta_any value) {
    if constexpr(std::is_member_function_pointer_v<decltype(Data)> || std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>>) {
        using descriptor = meta_function_helper_t<Type, decltype(Data)>;
        using data_type = type_list_element_t<descriptor::is_static, typename descriptor::args_type>;

        if(auto *const clazz = instance->try_cast<Type>(); clazz && value.allow_cast<data_type>()) {
            std::invoke(Data, *clazz, value.cast<data_type>());
            return true;
        }
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_reference_t<typename meta_function_helper_t<Type, decltype(Data)>::return_type>;

        if constexpr(!std::is_array_v<data_type> && !std::is_const_v<data_type>) {
            if(auto *const clazz = instance->try_cast<Type>(); clazz && value.allow_cast<data_type>()) {
                std::invoke(Data, *clazz) = value.cast<data_type>();
                return true;
            }
        }
    } else if constexpr(std::is_pointer_v<decltype(Data)>) {
        using data_type = std::remove_reference_t<decltype(*Data)>;

        if constexpr(!std::is_array_v<data_type> && !std::is_const_v<data_type>) {
            if(value.allow_cast<data_type>()) {
                *Data = value.cast<data_type>();
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Gets the value of a given variable.
 *
 * @warning
 * The context provided is used only for the return type.<br/>
 * It's up to the caller to bind the arguments to the right context(s).
 *
 * @tparam Type Reflected type to which the variable is associated.
 * @tparam Data The actual variable to get.
 * @tparam Policy Optional policy (no policy set by default).
 * @param ctx The context from which to search for meta types.
 * @param instance An opaque instance of the underlying type, if required.
 * @return A meta any containing the value of the underlying variable.
 */
template<typename Type, auto Data, typename Policy = as_is_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_getter(const meta_ctx &ctx, [[maybe_unused]] meta_handle instance) {
    if constexpr(std::is_member_pointer_v<decltype(Data)> || std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>>) {
        if constexpr(!std::is_array_v<std::remove_cv_t<std::remove_reference_t<std::invoke_result_t<decltype(Data), Type &>>>>) {
            if constexpr(std::is_invocable_v<decltype(Data), Type &>) {
                if(auto *clazz = instance->try_cast<Type>(); clazz) {
                    return meta_dispatch<Policy>(ctx, std::invoke(Data, *clazz));
                }
            }

            if constexpr(std::is_invocable_v<decltype(Data), const Type &>) {
                if(auto *fallback = instance->try_cast<const Type>(); fallback) {
                    return meta_dispatch<Policy>(ctx, std::invoke(Data, *fallback));
                }
            }
        }

        return meta_any{meta_ctx_arg, ctx};
    } else if constexpr(std::is_pointer_v<decltype(Data)>) {
        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            return meta_any{meta_ctx_arg, ctx};
        } else {
            return meta_dispatch<Policy>(ctx, *Data);
        }
    } else {
        return meta_dispatch<Policy>(ctx, Data);
    }
}

/**
 * @brief Gets the value of a given variable.
 * @tparam Type Reflected type to which the variable is associated.
 * @tparam Data The actual variable to get.
 * @tparam Policy Optional policy (no policy set by default).
 * @param instance An opaque instance of the underlying type, if required.
 * @return A meta any containing the value of the underlying variable.
 */
template<typename Type, auto Data, typename Policy = as_is_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_getter(meta_handle instance) {
    return meta_getter<Type, Data, Policy>(locator<meta_ctx>::value_or(), std::move(instance));
}

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Policy, typename Candidate, typename... Args>
[[nodiscard]] meta_any meta_invoke_with_args(const meta_ctx &ctx, Candidate &&candidate, Args &&...args) {
    if constexpr(std::is_void_v<decltype(std::invoke(std::forward<Candidate>(candidate), args...))>) {
        std::invoke(std::forward<Candidate>(candidate), args...);
        return meta_any{ctx, std::in_place_type<void>};
    } else {
        return meta_dispatch<Policy>(ctx, std::invoke(std::forward<Candidate>(candidate), args...));
    }
}

template<typename Type, typename Policy, typename Candidate, std::size_t... Index>
[[nodiscard]] meta_any meta_invoke(const meta_ctx &ctx, [[maybe_unused]] meta_handle instance, Candidate &&candidate, [[maybe_unused]] meta_any *const args, std::index_sequence<Index...>) {
    using descriptor = meta_function_helper_t<Type, std::remove_reference_t<Candidate>>;

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic) - waiting for C++20 (and std::span)
    if constexpr(std::is_invocable_v<std::remove_reference_t<Candidate>, const Type &, type_list_element_t<Index, typename descriptor::args_type>...>) {
        if(const auto *const clazz = instance->try_cast<const Type>(); clazz && ((args + Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return meta_invoke_with_args<Policy>(ctx, std::forward<Candidate>(candidate), *clazz, (args + Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    } else if constexpr(std::is_invocable_v<std::remove_reference_t<Candidate>, Type &, type_list_element_t<Index, typename descriptor::args_type>...>) {
        if(auto *const clazz = instance->try_cast<Type>(); clazz && ((args + Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return meta_invoke_with_args<Policy>(ctx, std::forward<Candidate>(candidate), *clazz, (args + Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    } else {
        if(((args + Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return meta_invoke_with_args<Policy>(ctx, std::forward<Candidate>(candidate), (args + Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    return meta_any{meta_ctx_arg, ctx};
}

template<typename Type, typename... Args, std::size_t... Index>
[[nodiscard]] meta_any meta_construct(const meta_ctx &ctx, meta_any *const args, std::index_sequence<Index...>) {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic) - waiting for C++20 (and std::span)
    if(((args + Index)->allow_cast<Args>() && ...)) {
        return meta_any{ctx, std::in_place_type<Type>, (args + Index)->cast<Args>()...};
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

    return meta_any{meta_ctx_arg, ctx};
}

} // namespace internal
/*! @endcond */

/**
 * @brief Tries to _invoke_ an object given a list of erased parameters.
 *
 * @warning
 * The context provided is used only for the return type.<br/>
 * It's up to the caller to bind the arguments to the right context(s).
 *
 * @tparam Type Reflected type to which the object to _invoke_ is associated.
 * @tparam Policy Optional policy (no policy set by default).
 * @param ctx The context from which to search for meta types.
 * @tparam Candidate The type of the actual object to _invoke_.
 * @param instance An opaque instance of the underlying type, if required.
 * @param candidate The actual object to _invoke_.
 * @param args Parameters to use to _invoke_ the object.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, typename Policy = as_is_t, typename Candidate>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_invoke(const meta_ctx &ctx, meta_handle instance, Candidate &&candidate, meta_any *const args) {
    return internal::meta_invoke<Type, Policy>(ctx, std::move(instance), std::forward<Candidate>(candidate), args, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<Candidate>>::args_type::size>{});
}

/**
 * @brief Tries to _invoke_ an object given a list of erased parameters.
 * @tparam Type Reflected type to which the object to _invoke_ is associated.
 * @tparam Policy Optional policy (no policy set by default).
 * @tparam Candidate The type of the actual object to _invoke_.
 * @param instance An opaque instance of the underlying type, if required.
 * @param candidate The actual object to _invoke_.
 * @param args Parameters to use to _invoke_ the object.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, typename Policy = as_is_t, typename Candidate>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_invoke(meta_handle instance, Candidate &&candidate, meta_any *const args) {
    return meta_invoke<Type, Policy>(locator<meta_ctx>::value_or(), std::move(instance), std::forward<Candidate>(candidate), args);
}

/**
 * @brief Tries to invoke a function given a list of erased parameters.
 *
 * @warning
 * The context provided is used only for the return type.<br/>
 * It's up to the caller to bind the arguments to the right context(s).
 *
 * @tparam Type Reflected type to which the function is associated.
 * @tparam Candidate The actual function to invoke.
 * @tparam Policy Optional policy (no policy set by default).
 * @param ctx The context from which to search for meta types.
 * @param instance An opaque instance of the underlying type, if required.
 * @param args Parameters to use to invoke the function.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, auto Candidate, typename Policy = as_is_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_invoke(const meta_ctx &ctx, meta_handle instance, meta_any *const args) {
    return internal::meta_invoke<Type, Policy>(ctx, std::move(instance), Candidate, args, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<decltype(Candidate)>>::args_type::size>{});
}

/**
 * @brief Tries to invoke a function given a list of erased parameters.
 * @tparam Type Reflected type to which the function is associated.
 * @tparam Candidate The actual function to invoke.
 * @tparam Policy Optional policy (no policy set by default).
 * @param instance An opaque instance of the underlying type, if required.
 * @param args Parameters to use to invoke the function.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, auto Candidate, typename Policy = as_is_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_invoke(meta_handle instance, meta_any *const args) {
    return meta_invoke<Type, Candidate, Policy>(locator<meta_ctx>::value_or(), std::move(instance), args);
}

/**
 * @brief Tries to construct an instance given a list of erased parameters.
 *
 * @warning
 * The context provided is used only for the return type.<br/>
 * It's up to the caller to bind the arguments to the right context(s).
 *
 * @tparam Type Actual type of the instance to construct.
 * @tparam Args Types of arguments expected.
 * @param ctx The context from which to search for meta types.
 * @param args Parameters to use to construct the instance.
 * @return A meta any containing the new instance, if any.
 */
template<typename Type, typename... Args>
[[nodiscard]] meta_any meta_construct(const meta_ctx &ctx, meta_any *const args) {
    return internal::meta_construct<Type, Args...>(ctx, args, std::index_sequence_for<Args...>{});
}

/**
 * @brief Tries to construct an instance given a list of erased parameters.
 * @tparam Type Actual type of the instance to construct.
 * @tparam Args Types of arguments expected.
 * @param args Parameters to use to construct the instance.
 * @return A meta any containing the new instance, if any.
 */
template<typename Type, typename... Args>
[[nodiscard]] meta_any meta_construct(meta_any *const args) {
    return meta_construct<Type, Args...>(locator<meta_ctx>::value_or(), args);
}

/**
 * @brief Tries to construct an instance given a list of erased parameters.
 *
 * @warning
 * The context provided is used only for the return type.<br/>
 * It's up to the caller to bind the arguments to the right context(s).
 *
 * @tparam Type Reflected type to which the object to _invoke_ is associated.
 * @tparam Policy Optional policy (no policy set by default).
 * @tparam Candidate The type of the actual object to _invoke_.
 * @param ctx The context from which to search for meta types.
 * @param candidate The actual object to _invoke_.
 * @param args Parameters to use to _invoke_ the object.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, typename Policy = as_is_t, typename Candidate>
[[nodiscard]] meta_any meta_construct(const meta_ctx &ctx, Candidate &&candidate, meta_any *const args) {
    if constexpr(meta_function_helper_t<Type, Candidate>::is_static || std::is_class_v<std::remove_cv_t<std::remove_reference_t<Candidate>>>) {
        return internal::meta_invoke<Type, Policy>(ctx, {}, std::forward<Candidate>(candidate), args, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<Candidate>>::args_type::size>{});
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) - waiting for C++20 (and std::span)
        return internal::meta_invoke<Type, Policy>(ctx, *args, std::forward<Candidate>(candidate), args + 1u, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<Candidate>>::args_type::size>{});
    }
}

/**
 * @brief Tries to construct an instance given a list of erased parameters.
 * @tparam Type Reflected type to which the object to _invoke_ is associated.
 * @tparam Policy Optional policy (no policy set by default).
 * @tparam Candidate The type of the actual object to _invoke_.
 * @param candidate The actual object to _invoke_.
 * @param args Parameters to use to _invoke_ the object.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, typename Policy = as_is_t, typename Candidate>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_construct(Candidate &&candidate, meta_any *const args) {
    return meta_construct<Type, Policy>(locator<meta_ctx>::value_or(), std::forward<Candidate>(candidate), args);
}

/**
 * @brief Tries to construct an instance given a list of erased parameters.
 *
 * @warning
 * The context provided is used only for the return type.<br/>
 * It's up to the caller to bind the arguments to the right context(s).
 *
 * @tparam Type Reflected type to which the function is associated.
 * @tparam Candidate The actual function to invoke.
 * @tparam Policy Optional policy (no policy set by default).
 * @param ctx The context from which to search for meta types.
 * @param args Parameters to use to invoke the function.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, auto Candidate, typename Policy = as_is_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_construct(const meta_ctx &ctx, meta_any *const args) {
    return meta_construct<Type, Policy>(ctx, Candidate, args);
}

/**
 * @brief Tries to construct an instance given a list of erased parameters.
 * @tparam Type Reflected type to which the function is associated.
 * @tparam Candidate The actual function to invoke.
 * @tparam Policy Optional policy (no policy set by default).
 * @param args Parameters to use to invoke the function.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, auto Candidate, typename Policy = as_is_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_construct(meta_any *const args) {
    return meta_construct<Type, Candidate, Policy>(locator<meta_ctx>::value_or(), args);
}

} // namespace entt

#endif
