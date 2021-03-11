#ifndef ENTT_META_UTILITY_HPP
#define ENTT_META_UTILITY_HPP


#include <array>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "policy.hpp"


namespace entt {


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
struct meta_function_descriptor<Type, Ret(Class:: *)(Args...) const> {
    /*! @brief Meta function return type. */
    using return_type = Ret;
    /*! @brief Meta function arguments. */
    using args_type = std::conditional_t<std::is_same_v<Type, Class>, type_list<Args...>, type_list<const Class &, Args...>>;

    /*! @brief True if the meta function is const, false otherwise. */
    static constexpr auto is_const = true;
    /*! @brief True if the meta function is static, false otherwise. */
    static constexpr auto is_static = !std::is_same_v<Type, Class>;
};


/**
 * @brief Meta function descriptor.
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Ret Function return type.
 * @tparam Class Actual owner of the member function.
 * @tparam Args Function arguments.
 */
template<typename Type, typename Ret, typename Class, typename... Args>
struct meta_function_descriptor<Type, Ret(Class:: *)(Args...)> {
    /*! @brief Meta function return type. */
    using return_type = Ret;
    /*! @brief Meta function arguments. */
    using args_type = std::conditional_t<std::is_same_v<Type, Class>, type_list<Args...>, type_list<Class &, Args...>>;

    /*! @brief True if the meta function is const, false otherwise. */
    static constexpr auto is_const = false;
    /*! @brief True if the meta function is static, false otherwise. */
    static constexpr auto is_static = !std::is_same_v<Type, Class>;
};


/**
 * @brief Meta function descriptor.
 * @tparam Type Reflected type to which the meta function is associated.
 * @tparam Ret Function return type.
 * @tparam Args Function arguments.
 */
template<typename Type, typename Ret, typename... Args>
struct meta_function_descriptor<Type, Ret(*)(Args...)> {
    /*! @brief Meta function return type. */
    using return_type = Ret;
    /*! @brief Meta function arguments. */
    using args_type = type_list<Args...>;

    /*! @brief True if the meta function is const, false otherwise. */
    static constexpr auto is_const = false;
    /*! @brief True if the meta function is static, false otherwise. */
    static constexpr auto is_static = true;
};


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
    static constexpr meta_function_descriptor<Type, Ret(Class:: *)(Args...) const> get_rid_of_noexcept(Ret(Class:: *)(Args...) const);

    template<typename Ret, typename... Args, typename Class>
    static constexpr meta_function_descriptor<Type, Ret(Class:: *)(Args...)> get_rid_of_noexcept(Ret(Class:: *)(Args...));

    template<typename Ret, typename... Args>
    static constexpr meta_function_descriptor<Type, Ret(*)(Args...)> get_rid_of_noexcept(Ret(*)(Args...));

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
 * @brief Returns the meta type of the i-th element of a list of arguments.
 * @tparam Args Actual types of arguments.
 * @return The meta type of the i-th element of the list of arguments.
 */
template<typename... Args>
[[nodiscard]] static meta_type meta_arg(type_list<Args...>, const std::size_t index) ENTT_NOEXCEPT {
    return internal::meta_arg_node(type_list<Args...>{}, index);
}


/**
 * @brief Constructs an instance given a list of erased parameters, if possible.
 * @tparam Type Actual type of the instance to construct.
 * @tparam Args Types of arguments expected.
 * @tparam Index Indexes to use to extract erased arguments from their list.
 * @param args Parameters to use to construct the instance.
 * @return A meta any containing the new instance, if any.
 */
template<typename Type, typename... Args, std::size_t... Index>
[[nodiscard]] meta_any meta_construct(meta_any * const args, std::index_sequence<Index...>) {
    if(((args+Index)->allow_cast<Args>() && ...)) {
        return Type{(args+Index)->cast<Args>()...};
    }

    return {};
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
    if constexpr(!std::is_same_v<decltype(Data), Type> && !std::is_same_v<decltype(Data), std::nullptr_t>) {
        if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
            using descriptor = meta_function_helper_t<Type, decltype(Data)>;
            using data_type = type_list_element_t<!std::is_member_function_pointer_v<decltype(Data)>, typename descriptor::args_type>;

            if(auto * const clazz = instance->try_cast<Type>(); clazz) {
                if(value.allow_cast<data_type>()) {
                    std::invoke(Data, *clazz, value.cast<data_type>());
                    return true;
                }
            }
        } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::remove_reference_t<decltype(std::declval<Type>().*Data)>;

            if constexpr(!std::is_array_v<data_type> && !std::is_const_v<data_type>) {
                if(auto * const clazz = instance->try_cast<Type>(); clazz) {
                    if(value.allow_cast<data_type>()) {
                        std::invoke(Data, clazz) = value.cast<data_type>();
                        return true;
                    }
                }
            }
        } else {
            using data_type = std::remove_reference_t<decltype(*Data)>;

            if constexpr(!std::is_array_v<data_type> && !std::is_const_v<data_type>) {
                if(value.allow_cast<data_type>()) {
                    *Data = value.cast<data_type>();
                    return true;
                }
            }
        }
    }

    return false;
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
[[nodiscard]] meta_any meta_getter([[maybe_unused]] meta_handle instance) {
    [[maybe_unused]] auto dispatch = [](auto &&value) {
        if constexpr(std::is_same_v<Policy, as_void_t>) {
            return meta_any{std::in_place_type<void>, std::forward<decltype(value)>(value)};
        } else if constexpr(std::is_same_v<Policy, as_ref_t>) {
            return meta_any{std::reference_wrapper{std::forward<decltype(value)>(value)}};
        } else if constexpr(std::is_same_v<Policy, as_cref_t>) {
            return meta_any{std::cref(std::forward<decltype(value)>(value))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>, "Policy not supported");
            return meta_any{std::forward<decltype(value)>(value)};
        }
    };

    if constexpr(std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>> || std::is_member_function_pointer_v<decltype(Data)>) {
        auto * const clazz = instance->try_cast<std::conditional_t<std::is_invocable_v<decltype(Data), const Type &>, const Type, Type>>();
        return clazz ? dispatch(std::invoke(Data, *clazz)) : meta_any{};
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        if constexpr(std::is_array_v<std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Type>().*Data)>>>) {
            return meta_any{};
        } else {
            if(auto * clazz = instance->try_cast<Type>(); clazz) {
                return dispatch(std::invoke(Data, *clazz));
            } else {
                auto * fallback = instance->try_cast<const Type>();
                return fallback ? dispatch(std::invoke(Data, *fallback)) : meta_any{};
            }
        }
    } else if constexpr(std::is_pointer_v<decltype(Data)>) {
        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            return meta_any{};
        } else {
            return dispatch(*Data);
        }
    } else {
        return dispatch(Data);
    }
}


/**
 * @brief Invokes a function given a list of erased parameters, if possible.
 * @tparam Type Reflected type to which the function is associated.
 * @tparam Candidate The actual function to invoke.
 * @tparam Policy Optional policy (no policy set by default).
 * @tparam Index Indexes to use to extract erased arguments from their list.
 * @param instance An opaque instance of the underlying type, if required.
 * @param args Parameters to use to invoke the function.
 * @return A meta any containing the returned value, if any.
 */
template<typename Type, auto Candidate, typename Policy = as_is_t, std::size_t... Index>
[[nodiscard]] meta_any meta_invoke([[maybe_unused]] meta_handle instance, meta_any *args, std::index_sequence<Index...>) {
    using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;

    auto dispatch = [](auto &&... params) {
        if constexpr(std::is_void_v<std::remove_cv_t<typename descriptor::return_type>> || std::is_same_v<Policy, as_void_t>) {
            std::invoke(Candidate, std::forward<decltype(params)>(params)...);
            return meta_any{std::in_place_type<void>};
        } else if constexpr(std::is_same_v<Policy, as_ref_t>) {
            return meta_any{std::reference_wrapper{std::invoke(Candidate, std::forward<decltype(params)>(params)...)}};
        } else if constexpr(std::is_same_v<Policy, as_cref_t>) {
            return meta_any{std::cref(std::invoke(Candidate, std::forward<decltype(params)>(params)...))};
        } else {
            static_assert(std::is_same_v<Policy, as_is_t>, "Policy not supported");
            return meta_any{std::invoke(Candidate, std::forward<decltype(params)>(params)...)};
        }
    };

    if constexpr(std::is_invocable_v<decltype(Candidate), const Type &, type_list_element_t<Index, typename descriptor::args_type>...>) {
        if(const auto * const clazz = instance->try_cast<const Type>(); clazz && ((args+Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return dispatch(*clazz, (args+Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    } else if constexpr(std::is_invocable_v<decltype(Candidate), Type &, type_list_element_t<Index, typename descriptor::args_type>...>) {
        if(auto * const clazz = instance->try_cast<Type>(); clazz && ((args+Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return dispatch(*clazz, (args+Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    } else {
        if(((args+Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return dispatch((args+Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    }

    return meta_any{};
}


}


#endif
