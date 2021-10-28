#ifndef ENTT_CORE_COMPRESSED_PAIR_HPP
#define ENTT_CORE_COMPRESSED_PAIR_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "type_traits.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Type, std::size_t, typename = void>
struct compressed_pair_element {
    using reference = Type &;
    using const_reference = const Type &;

    template<bool Dummy = true, typename = std::enable_if_t<Dummy && std::is_default_constructible_v<Type>>>
    compressed_pair_element()
        : value{} {}

    template<typename Args, typename = std::enable_if_t<!std::is_same_v<std::remove_const_t<std::remove_reference_t<Args>>, compressed_pair_element>>>
    compressed_pair_element(Args &&args)
        : value{std::forward<Args>(args)} {}

    template<typename... Args, std::size_t... Index>
    compressed_pair_element(std::tuple<Args...> args, std::index_sequence<Index...>)
        : value{std::get<Index>(args)...} {}

    [[nodiscard]] reference get() ENTT_NOEXCEPT {
        return value;
    }

    [[nodiscard]] const_reference get() const ENTT_NOEXCEPT {
        return value;
    }

private:
    Type value;
};

template<typename Type, std::size_t Tag>
struct compressed_pair_element<Type, Tag, std::enable_if_t<is_ebco_eligible_v<Type>>>: Type {
    using reference = Type &;
    using const_reference = const Type &;
    using base_type = Type;

    template<bool Dummy = true, typename = std::enable_if_t<Dummy && std::is_default_constructible_v<base_type>>>
    compressed_pair_element()
        : base_type{} {}

    template<typename Args, typename = std::enable_if_t<!std::is_same_v<std::remove_const_t<std::remove_reference_t<Args>>, compressed_pair_element>>>
    compressed_pair_element(Args &&args)
        : base_type{std::forward<Args>(args)} {}

    template<typename... Args, std::size_t... Index>
    compressed_pair_element(std::tuple<Args...> args, std::index_sequence<Index...>)
        : base_type{std::get<Index>(args)...} {}

    [[nodiscard]] reference get() ENTT_NOEXCEPT {
        return *this;
    }

    [[nodiscard]] const_reference get() const ENTT_NOEXCEPT {
        return *this;
    }
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief A compressed pair.
 *
 * A pair that exploits the _Empty Base Class Optimization_ (or _EBCO_) to
 * reduce its final size to a minimum.
 *
 * @tparam First The type of the first element that the pair stores.
 * @tparam Second The type of the second element that the pair stores.
 */
template<typename First, typename Second>
class compressed_pair final
    : internal::compressed_pair_element<First, 0u>,
      internal::compressed_pair_element<Second, 1u> {
    using first_base = internal::compressed_pair_element<First, 0u>;
    using second_base = internal::compressed_pair_element<Second, 1u>;

public:
    /*! @brief The type of the first element that the pair stores. */
    using first_type = First;
    /*! @brief The type of the second element that the pair stores. */
    using second_type = Second;

    /**
     * @brief Default constructor, conditionally enabled.
     *
     * This constructor is only available when the types that the pair stores
     * are both at least default constructible.
     *
     * @tparam Dummy Dummy template parameter used for internal purposes.
     */
    template<bool Dummy = true, typename = std::enable_if_t<Dummy && std::is_default_constructible_v<first_type> && std::is_default_constructible_v<second_type>>>
    constexpr compressed_pair()
        : first_base{},
          second_base{} {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    constexpr compressed_pair(const compressed_pair &other) = default;

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    constexpr compressed_pair(compressed_pair &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Constructs a pair from its values.
     * @tparam Arg Type of value to use to initialize the first element.
     * @tparam Other Type of value to use to initialize the second element.
     * @param arg Value to use to initialize the first element.
     * @param other Value to use to initialize the second element.
     */
    template<typename Arg, typename Other>
    constexpr compressed_pair(Arg &&arg, Other &&other)
        : first_base{std::forward<Arg>(arg)},
          second_base{std::forward<Other>(other)} {}

    /**
     * @brief Constructs a pair by forwarding the arguments to its parts.
     * @tparam Args Types of arguments to use to initialize the first element.
     * @tparam Other Types of arguments to use to initialize the second element.
     * @param args Arguments to use to initialize the first element.
     * @param other Arguments to use to initialize the second element.
     */
    template<typename... Args, typename... Other>
    constexpr compressed_pair(std::piecewise_construct_t, std::tuple<Args...> args, std::tuple<Other...> other)
        : first_base{std::move(args), std::index_sequence_for<Args...>{}},
          second_base{std::move(other), std::index_sequence_for<Other...>{}} {}

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This compressed pair object.
     */
    constexpr compressed_pair &operator=(const compressed_pair &other) = default;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This compressed pair object.
     */
    constexpr compressed_pair &operator=(compressed_pair &&other) ENTT_NOEXCEPT = default;

    /**
     * @brief Returns the first element that a pair stores.
     * @return The first element that a pair stores.
     */
    [[nodiscard]] first_type &first() ENTT_NOEXCEPT {
        return static_cast<first_base &>(*this).get();
    }

    /*! @copydoc first */
    [[nodiscard]] const first_type &first() const ENTT_NOEXCEPT {
        return static_cast<const first_base &>(*this).get();
    }

    /**
     * @brief Returns the second element that a pair stores.
     * @return The second element that a pair stores.
     */
    [[nodiscard]] second_type &second() ENTT_NOEXCEPT {
        return static_cast<second_base &>(*this).get();
    }

    /*! @copydoc second */
    [[nodiscard]] const second_type &second() const ENTT_NOEXCEPT {
        return static_cast<const second_base &>(*this).get();
    }

    /**
     * @brief Swaps two compressed pair objects.
     * @param other The compressed pair to swap with.
     */
    void swap(compressed_pair &other) {
        using std::swap;
        swap(first(), other.first());
        swap(second(), other.second());
    }

    /**
     * @brief Extracts an element from the compressed pair.
     * @tparam Index An integer value that is either 0 or 1.
     * @return Returns a reference to the first element if `Index` is 0 and a
     * reference to the second element if `Index` is 1.
     */
    template<std::size_t Index>
    decltype(auto) get() ENTT_NOEXCEPT {
        if constexpr(Index == 0u) {
            return first();
        } else {
            static_assert(Index == 1u, "Index out of bounds");
            return second();
        }
    }

    /*! @copydoc get */
    template<std::size_t Index>
    decltype(auto) get() const ENTT_NOEXCEPT {
        if constexpr(Index == 0u) {
            return first();
        } else {
            static_assert(Index == 1u, "Index out of bounds");
            return second();
        }
    }
};

/**
 * @brief Deduction guide.
 * @tparam Type Type of value to use to initialize the first element.
 * @tparam Other Type of value to use to initialize the second element.
 */
template<typename Type, typename Other>
compressed_pair(Type &&, Other &&) -> compressed_pair<std::decay_t<Type>, std::decay_t<Other>>;

/**
 * @brief Swaps two compressed pair objects.
 * @tparam First The type of the first element that the pairs store.
 * @tparam Second The type of the second element that the pairs store.
 * @param lhs A valid compressed pair object.
 * @param rhs A valid compressed pair object.
 */
template<typename First, typename Second>
inline void swap(compressed_pair<First, Second> &lhs, compressed_pair<First, Second> &rhs) {
    lhs.swap(rhs);
}

} // namespace entt

// disable structured binding support for clang 6, it messes when specializing tuple_size
#if !defined __clang_major__ || __clang_major__ > 6
namespace std {

template<typename First, typename Second>
struct tuple_size<entt::compressed_pair<First, Second>>: integral_constant<size_t, 2u> {};

template<size_t Index, typename First, typename Second>
struct tuple_element<Index, entt::compressed_pair<First, Second>>: conditional<Index == 0u, First, Second> {
    static_assert(Index < 2u, "Index out of bounds");
};

} // namespace std
#endif

#endif
