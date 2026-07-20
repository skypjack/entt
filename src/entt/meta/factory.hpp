#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP

#include "../config/config.h"
#include "../core/bit.hpp"
#include "../core/fwd.hpp"
#include "../core/hashed_string.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../locator/locator.hpp"
#include "../stl/algorithm.hpp"
#include "../stl/concepts.hpp"
#include "../stl/cstddef.hpp"
#include "../stl/cstdint.hpp"
#include "../stl/functional.hpp"
#include "../stl/memory.hpp"
#include "../stl/type_traits.hpp"
#include "../stl/utility.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "policy.hpp"
#include "range.hpp"
#include "utility.hpp"

namespace entt {

/*! @cond ENTT_INTERNAL */
namespace internal {

class basic_meta_factory {
    using invoke_type = stl::remove_pointer_t<decltype(meta_func_node::invoke)>;

    enum class mode {
        type,
        data,
        func
    };

    [[nodiscard]] auto *find_member_or_assert() {
        auto *member = find_member(parent->details->data, bucket);
        ENTT_ASSERT(member != nullptr, "Cannot find member");
        return member;
    }

    [[nodiscard]] auto *find_overload_or_assert() {
        ENTT_ASSERT(invoke != nullptr, "Invoke function not available");
        auto *overload = find_overload(find_member(parent->details->func, bucket), invoke);
        ENTT_ASSERT(overload != nullptr, "Cannot find overload");
        return overload;
    }

    bool unique_alias(const id_type alias) const noexcept {
        return (ctx->bucket.find(alias) == ctx->bucket.cend()) && (stl::find_if(ctx->bucket.cbegin(), ctx->bucket.cend(), [alias](const auto &value) { return value.second->alias == alias; }) == ctx->bucket.cend());
    }

protected:
    void type(const id_type alias, const char *name) noexcept {
        state = mode::type;
        ENTT_ASSERT((parent->alias == alias) || unique_alias(alias), "Duplicate identifier");
        parent->alias = alias;
        parent->name = name;
    }

    template<typename Type>
    void insert_or_assign(Type node) {
        state = mode::type;

        if constexpr(stl::is_same_v<Type, meta_base_node>) {
            auto *member = find_member(parent->details->base, node.id);
            member ? (*member = node) : parent->details->base.emplace_back(node);
        } else if constexpr(stl::is_same_v<Type, meta_conv_node>) {
            auto *member = find_member(parent->details->conv, node.id);
            member ? (*member = node) : parent->details->conv.emplace_back(node);
        } else {
            static_assert(stl::is_same_v<Type, meta_ctor_node>, "Unexpected type");
            auto *member = find_member(parent->details->ctor, node.id);
            member ? (*member = node) : parent->details->ctor.emplace_back(node);
        }
    }

    void data(meta_data_node node) {
        state = mode::data;
        bucket = node.id;

        if(auto *member = find_member(parent->details->data, node.id); member == nullptr) {
            parent->details->data.emplace_back(stl::move(node));
        } else if(member->set != node.set || member->get != node.get) {
            *member = stl::move(node);
        }
    }

    void func(meta_func_node node) {
        state = mode::func;
        bucket = node.id;
        invoke = node.invoke;

        if(auto *member = find_member(parent->details->func, node.id); member == nullptr) {
            parent->details->func.emplace_back(stl::move(node));
        } else if(auto *overload = find_overload(member, node.invoke); overload == nullptr) {
            while(member->next != nullptr) { member = member->next.get(); }
            member->next = stl::make_unique<meta_func_node>(stl::move(node));
        }
    }

    void traits(const meta_traits value, const bool unset) {
        const auto set_or_unset_on = [=](auto &node) {
            node.traits = (unset ? (node.traits & ~value) : (node.traits | value));
        };

        switch(state) {
        case mode::type:
            set_or_unset_on(*parent);
            break;
        case mode::data:
            set_or_unset_on(*find_member_or_assert());
            break;
        case mode::func:
            set_or_unset_on(*find_overload_or_assert());
            break;
        }
    }

    void custom(meta_custom_node node) {
        switch(state) {
        case mode::type:
            parent->custom = stl::move(node);
            break;
        case mode::data:
            find_member_or_assert()->custom = stl::move(node);
            break;
        case mode::func:
            find_overload_or_assert()->custom = stl::move(node);
            break;
        }
    }

public:
    basic_meta_factory(meta_ctx &area, meta_type_node node, const id_type id)
        : ctx{&meta_context::from(area)},
          bucket{},
          state{mode::type} {
        if(const auto it = ctx->bucket.find(id); it == ctx->bucket.cend()) {
            ENTT_ASSERT(unique_alias(id), "Duplicate identifier");
            parent = ctx->bucket.emplace(id, stl::make_unique<meta_type_node>(stl::move(node))).first->second.get();
            parent->details = stl::make_unique<meta_type_descriptor>();
            parent->alias = id;
        } else {
            parent = it->second.get();
        }
    }

private:
    meta_context *ctx{};
    invoke_type *invoke{};
    meta_type_node *parent{};
    id_type bucket{};
    mode state{};
};

} // namespace internal
/*! @endcond */

/**
 * @brief Meta factory to be used for reflection purposes.
 * @tparam Type Type for which the factory was created.
 */
template<typename Type>
class meta_factory: private internal::basic_meta_factory {
    using base_type = internal::basic_meta_factory;

public:
    /*! @brief Type of object for which this factory builds a meta type. */
    using element_type = Type;

    /*! @brief Default constructor. */
    meta_factory() noexcept
        : meta_factory{locator<meta_ctx>::value_or()} {}

    /**
     * @brief Context aware constructor.
     * @param area The context into which to construct meta types.
     */
    meta_factory(meta_ctx &area) noexcept
        : base_type{area, internal::setup_node_for<element_type>(), type_hash<Type>::value()} {}

    /**
     * @brief Constructs an unconstrained type assigned to a given identifier.
     * @param id A custom unique identifier.
     */
    meta_factory(const id_type id) noexcept
        : meta_factory{locator<meta_ctx>::value_or(), id} {}

    /**
     * @brief Context aware constructor.
     * @param id A custom unique identifier.
     * @param area The context into which to construct meta types.
     */
    meta_factory(meta_ctx &area, const id_type id) noexcept
        : base_type{area, internal::setup_node_for<element_type>(), id} {}

    /**
     * @brief Assigns a custom unique identifier to a meta type.
     * @param name A custom unique identifier as a **string literal**.
     * @return A meta factory for the given type.
     */
    meta_factory type(const char *name) noexcept {
        return type(hashed_string::value(name), name);
    }

    /**
     * @brief Assigns a custom unique identifier to a meta type.
     * @param alias A custom unique identifier.
     * @param name An optional name for the type as a **string literal**.
     * @return A meta factory for the given type.
     */
    meta_factory type(const id_type alias, const char *name = nullptr) noexcept {
        base_type::type(alias, name);
        return *this;
    }

    /**
     * @brief Assigns a meta base to a meta type.
     *
     * A reflected base class must be a real base class of the reflected type.
     *
     * @tparam Base Type of the base class to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename Base>
    requires stl::derived_from<element_type, Base>
    meta_factory base() noexcept {
        if constexpr(!stl::same_as<element_type, Base>) {
            auto *const op = +[](const void *instance) noexcept { return static_cast<const void *>(static_cast<const Base *>(static_cast<const element_type *>(instance))); };

            base_type::insert_or_assign(
                internal::meta_base_node{
                    type_id<Base>().hash(),
                    &internal::resolve<Base>,
                    op});
        }

        return *this;
    }

    /**
     * @brief Assigns a meta conversion function to a meta type.
     *
     * Conversion functions can be either free functions or member
     * functions.<br/>
     * In case of free functions, they must accept a const reference to an
     * instance of the parent type as an argument. In case of member functions,
     * they should have no arguments at all.
     *
     * @tparam Candidate The actual function to use for the conversion.
     * @return A meta factory for the parent type.
     */
    template<auto Candidate>
    auto conv() noexcept {
        using conv_type = stl::remove_cvref_t<stl::invoke_result_t<decltype(Candidate), element_type &>>;
        auto *const op = +[](const meta_ctx &area, const void *instance) { return forward_as_meta(area, stl::invoke(Candidate, *static_cast<const element_type *>(instance))); };

        base_type::insert_or_assign(
            internal::meta_conv_node{
                type_id<conv_type>().hash(),
                op});

        return *this;
    }

    /**
     * @brief Assigns a meta conversion function to a meta type.
     *
     * The given type must be such that an instance of the reflected type can be
     * converted to it.
     *
     * @tparam To Type of the conversion function to assign to the meta type.
     * @return A meta factory for the parent type.
     */
    template<typename To>
    meta_factory conv() noexcept {
        using conv_type = stl::remove_cvref_t<To>;
        auto *const op = +[](const meta_ctx &area, const void *instance) { return forward_as_meta(area, static_cast<To>(*static_cast<const element_type *>(instance))); };

        base_type::insert_or_assign(
            internal::meta_conv_node{
                type_id<conv_type>().hash(),
                op});

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * Both member functions and free function can be assigned to meta types in
     * the role of constructors. All that is required is that they return an
     * instance of the underlying type.<br/>
     * From a client's point of view, nothing changes if a constructor of a meta
     * type is a built-in one or not.
     *
     * @tparam Candidate The actual function to use as a constructor.
     * @tparam Policy Optional policy (no policy set by default).
     * @return A meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_value_t>
    meta_factory ctor() noexcept {
        using descriptor = meta_function_helper_t<element_type, decltype(Candidate)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");
        static_assert(stl::is_same_v<stl::remove_cvref_t<typename descriptor::return_type>, element_type>, "The function doesn't return an object of the required type");

        base_type::insert_or_assign(
            internal::meta_ctor_node{
                type_id<typename descriptor::args_type>().hash(),
                descriptor::args_type::size,
                &meta_arg<typename descriptor::args_type>,
                &meta_construct<element_type, Candidate, Policy>});

        return *this;
    }

    /**
     * @brief Assigns a meta constructor to a meta type.
     *
     * A meta constructor is uniquely identified by the types of its arguments
     * and is such that there exists an actual constructor of the underlying
     * type that can be invoked with parameters whose types are those given.
     *
     * @tparam Args Types of arguments to use to construct an instance.
     * @return A meta factory for the parent type.
     */
    template<typename... Args>
    meta_factory ctor() noexcept {
        // default constructor is already implicitly generated, no need for redundancy
        if constexpr(sizeof...(Args) != 0u) {
            using descriptor = meta_function_helper_t<element_type, element_type (*)(Args...)>;

            base_type::insert_or_assign(
                internal::meta_ctor_node{
                    type_id<typename descriptor::args_type>().hash(),
                    descriptor::args_type::size,
                    &meta_arg<typename descriptor::args_type>,
                    &meta_construct<element_type, Args...>});
        }

        return *this;
    }

    /**
     * @brief Assigns a meta data to a meta type.
     * @tparam Data The actual variable to attach to the meta type.
     * @tparam Policy Optional policy (no policy set by default).
     * @param name A custom unique identifier as a **string literal**.
     * @return A meta factory for the given type.
     */
    template<auto Data, typename Policy = as_value_t>
    meta_factory data(const char *name) noexcept {
        return data<Data, Policy>(hashed_string::value(name), name);
    }

    /**
     * @brief Assigns a meta data to a meta type.
     *
     * Both data members and static and global variables, as well as constants
     * of any kind, can be assigned to a meta type.<br/>
     * From a client's point of view, all the variables associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Data The actual variable to attach to the meta type.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @param name An optional name for the meta data as a **string literal**.
     * @return A meta factory for the parent type.
     */
    template<auto Data, typename Policy = as_value_t>
    meta_factory data(const id_type id, const char *name = nullptr) noexcept {
        if constexpr(stl::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = stl::invoke_result_t<decltype(Data), element_type &>;
            static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");

            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    /* this is never static */
                    stl::is_const_v<stl::remove_reference_t<data_type>> ? internal::meta_traits::is_const : internal::meta_traits::is_none,
                    1u,
                    0u,
                    &meta_arg<type_list<stl::remove_cvref_t<data_type>>>,
                    &meta_arg<type_list<>>,
                    &internal::resolve<stl::remove_cvref_t<data_type>>,
                    &meta_setter<element_type, Data>,
                    &meta_getter<element_type, Data, Policy>});
        } else {
            using data_type = stl::remove_pointer_t<decltype(Data)>;

            if constexpr(stl::is_pointer_v<decltype(Data)>) {
                static_assert(Policy::template value<decltype(*Data)>, "Invalid return type for the given policy");
            } else {
                static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");
            }

            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    ((!stl::is_pointer_v<decltype(Data)> || stl::is_const_v<data_type>) ? internal::meta_traits::is_const : internal::meta_traits::is_none) | internal::meta_traits::is_static,
                    1u,
                    0u,
                    &meta_arg<type_list<stl::remove_cvref_t<data_type>>>,
                    &meta_arg<type_list<>>,
                    &internal::resolve<stl::remove_cvref_t<data_type>>,
                    &meta_setter<element_type, Data>,
                    &meta_getter<element_type, Data, Policy>});
        }

        return *this;
    }

    /**
     * @brief Assigns a meta data to a meta type by means of its setter and
     * getter.
     * @tparam Setter The actual function to use as a setter.
     * @tparam Getter The actual function to use as a getter.
     * @tparam Policy Optional policy (no policy set by default).
     * @param name A custom unique identifier as a **string literal**.
     * @return A meta factory for the given type.
     */
    template<auto Setter, auto Getter, typename Policy = as_value_t>
    meta_factory data(const char *name) noexcept {
        return data<Setter, Getter, Policy>(hashed_string::value(name), name);
    }

    /**
     * @brief Assigns a meta data to a meta type by means of its setter and
     * getter.
     *
     * Setters and getters can be either free functions, member functions or a
     * mix of them.<br/>
     * In case of free functions, setters and getters must accept a reference to
     * an instance of the parent type as their first argument. A setter has then
     * an extra argument of a type convertible to that of the parameter to
     * set.<br/>
     * In case of member functions, getters have no arguments at all, while
     * setters has an argument of a type convertible to that of the parameter to
     * set.
     *
     * @tparam Setter The actual function to use as a setter.
     * @tparam Getter The actual function to use as a getter.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @param name An optional name for the meta data as a **string literal**.
     * @return A meta factory for the parent type.
     */
    template<auto Setter, auto Getter, typename Policy = as_value_t>
    meta_factory data(const id_type id, const char *name = nullptr) noexcept {
        using getter = meta_function_helper_t<element_type, decltype(Getter)>;
        static_assert(Policy::template value<typename getter::return_type>, "Invalid return type for the given policy");

        if constexpr(stl::is_same_v<decltype(Setter), stl::nullptr_t>) {
            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    /* this is never static */
                    internal::meta_traits::is_const,
                    0u,
                    getter::args_type::size,
                    &meta_arg<type_list<>>,
                    &meta_arg<typename getter::args_type>,
                    &internal::resolve<stl::remove_cvref_t<typename getter::return_type>>,
                    &meta_setter<element_type, Setter>,
                    &meta_getter<element_type, Getter, Policy>});
        } else {
            using setter = meta_function_helper_t<element_type, decltype(Setter)>;

            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    /* this is never static nor const */
                    internal::meta_traits::is_none,
                    setter::args_type::size,
                    getter::args_type::size,
                    &meta_arg<typename setter::args_type>,
                    &meta_arg<typename getter::args_type>,
                    &internal::resolve<stl::remove_cvref_t<typename getter::return_type>>,
                    &meta_setter<element_type, Setter>,
                    &meta_getter<element_type, Getter, Policy>});
        }

        return *this;
    }

    /**
     * @brief Assigns a meta function to a meta type.
     * @tparam Candidate The actual function to attach to the meta function.
     * @tparam Policy Optional policy (no policy set by default).
     * @param name A custom unique identifier as a **string literal**.
     * @return A meta factory for the given type.
     */
    template<auto Candidate, typename Policy = as_value_t>
    meta_factory func(const char *name) noexcept {
        return func<Candidate, Policy>(hashed_string::value(name), name);
    }

    /**
     * @brief Assigns a meta function to a meta type.
     *
     * Both member functions and free functions can be assigned to a meta
     * type.<br/>
     * From a client's point of view, all the functions associated with the
     * reflected object will appear as if they were part of the type itself.
     *
     * @tparam Candidate The actual function to attach to the meta type.
     * @tparam Policy Optional policy (no policy set by default).
     * @param id Unique identifier.
     * @param name An optional name for the function as a **string literal**.
     * @return A meta factory for the parent type.
     */
    template<auto Candidate, typename Policy = as_value_t>
    meta_factory func(const id_type id, const char *name = nullptr) noexcept {
        using descriptor = meta_function_helper_t<element_type, decltype(Candidate)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");

        base_type::func(
            internal::meta_func_node{
                id,
                name,
                (descriptor::is_const ? internal::meta_traits::is_const : internal::meta_traits::is_none) | (descriptor::is_static ? internal::meta_traits::is_static : internal::meta_traits::is_none),
                descriptor::args_type::size,
                &internal::resolve<stl::conditional_t<stl::is_same_v<Policy, as_void_t>, void, stl::remove_cvref_t<typename descriptor::return_type>>>,
                &meta_arg<typename descriptor::args_type>,
                &meta_invoke<element_type, Candidate, Policy>});

        return *this;
    }

    /**
     * @brief Sets traits on the last created meta object.
     *
     * The assigned value must be an enum and intended as a bitmask.
     *
     * @tparam Value Type of the traits value.
     * @param value Traits value.
     * @param unset True to unset the given traits, false otherwise.
     * @return A meta factory for the parent type.
     */
    template<typename Value>
    meta_factory traits(const Value value, const bool unset = false) {
        static_assert(stl::is_enum_v<Value>, "Invalid enum type");
        base_type::traits(internal::user_to_meta_traits(value), unset);
        return *this;
    }

    /**
     * @brief Sets user defined data that will never be used by the library.
     * @tparam Value Type of user defined data to store.
     * @tparam Args Types of arguments to use to construct the user data.
     * @param args Parameters to use to initialize the user data.
     * @return A meta factory for the parent type.
     */
    template<typename Value, typename... Args>
    meta_factory custom(Args &&...args) {
        base_type::custom(internal::meta_custom_node{type_id<Value>().hash(), stl::make_shared<Value>(stl::forward<Args>(args)...)});
        return *this;
    }
};

/**
 * @brief Resets a type and all its parts.
 *
 * Resets a type and all its data members, member functions and properties, as
 * well as its constructors, destructors and conversion functions if any.<br/>
 * Base classes aren't reset but the link between the two types is removed.
 *
 * The type is also removed from the set of searchable types.
 *
 * @param alias Unique identifier.
 * @param ctx The context from which to reset meta types.
 */
inline void meta_reset(meta_ctx &ctx, const id_type alias) noexcept {
    auto &bucket = internal::meta_context::from(ctx).bucket;

    // fast path for unsearchable and overloaded types
    if(bucket.erase(alias) == 0u) {
        if(const auto it = stl::find_if(bucket.cbegin(), bucket.cend(), [alias](const auto &value) { return value.second->alias == alias; }); it != bucket.cend()) {
            bucket.erase(it);
        }
    }
}

/**
 * @brief Resets a type and all its parts.
 *
 * Resets a type and all its data members, member functions and properties, as
 * well as its constructors, destructors and conversion functions if any.<br/>
 * Base classes aren't reset but the link between the two types is removed.
 *
 * The type is also removed from the set of searchable types.
 *
 * @param alias Unique identifier.
 */
inline void meta_reset(const id_type alias) noexcept {
    meta_reset(locator<meta_ctx>::value_or(), alias);
}

/**
 * @brief Resets a type and all its parts.
 *
 * @sa meta_reset
 *
 * @tparam Type Type to reset.
 * @param ctx The context from which to reset meta types.
 */
template<typename Type>
void meta_reset(meta_ctx &ctx) noexcept {
    internal::meta_context::from(ctx).bucket.erase(type_id<Type>().hash());
}

/**
 * @brief Resets a type and all its parts.
 *
 * @sa meta_reset
 *
 * @tparam Type Type to reset.
 */
template<typename Type>
void meta_reset() noexcept {
    meta_reset<Type>(locator<meta_ctx>::value_or());
}

/**
 * @brief Resets all meta types.
 *
 * @sa meta_reset
 *
 * @param ctx The context from which to reset meta types.
 */
inline void meta_reset(meta_ctx &ctx) noexcept {
    internal::meta_context::from(ctx).bucket.clear();
}

/**
 * @brief Resets all meta types.
 *
 * @sa meta_reset
 */
inline void meta_reset() noexcept {
    meta_reset(locator<meta_ctx>::value_or());
}

} // namespace entt

#endif
