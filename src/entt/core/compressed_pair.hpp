#ifndef ENTT_CORE_COMPRESSED_PAIR_HPP
#define ENTT_CORE_COMPRESSED_PAIR_HPP

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "fwd.hpp"
#include "type_traits.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Type, std::size_t, typename = void>
struct compressed_pair_element {
    using reference = Type &;
    using const_reference = const Type &;

    template<typename Dummy = Type, typename = std::enable_if_t<std::is_default_constructible_v<Dummy>>>
    // NOLINTNEXTLINE(modernize-use-equals-default)
    constexpr compressed_pair_element() noexcept(std::is_nothrow_default_constructible_v<Type>) {}

    template<typename Arg, typename = std::enable_if_t<!std::is_same_v<std::remove_const_t<std::remove_reference_t<Arg>>, compressed_pair_element>>>
    constexpr compressed_pair_element(Arg &&arg) noexcept(std::is_nothrow_constructible_v<Type, Arg>)
        : value{std::forward<Arg>(arg)} {}

    template<typename... Args, std::size_t... Index>
    constexpr compressed_pair_element(std::tuple<Args...> args, std::index_sequence<Index...>) noexcept(std::is_nothrow_constructible_v<Type, Args...>)
        : value{std::forward<Args>(std::get<Index>(args))...} {}

    [[nodiscard]] constexpr reference get() noexcept {
        return value;
    }

    [[nodiscard]] constexpr const_reference get() const noexcept {
        return value;
    }

private:
    Type value{};
};

template<typename Type, std::size_t Tag>
struct compressed_pair_element<Type, Tag, std::enable_if_t<is_ebco_eligible_v<Type>>>: Type {
    using reference = Type &;
    using const_reference = const Type &;
    using base_type = Type;

    template<typename Dummy = Type, typename = std::enable_if_t<std::is_default_constructible_v<Dummy>>>
    constexpr compressed_pair_element() noexcept(std::is_nothrow_default_constructible_v<base_type>)
        : base_type{} {}

    template<typename Arg, typename = std::enable_if_t<!std::is_same_v<std::remove_const_t<std::remove_reference_t<Arg>>, compressed_pair_element>>>
    constexpr compressed_pair_element(Arg &&arg) noexcept(std::is_nothrow_constructible_v<base_type, Arg>)
        : base_type{std::forward<Arg>(arg)} {}

    template<typename... Args, std::size_t... Index>
    constexpr compressed_pair_element(std::tuple<Args...> args, std::index_sequence<Index...>) noexcept(std::is_nothrow_constructible_v<base_type, Args...>)
        : base_type{std::forward<Args>(std::get<Index>(args))...} {}

    [[nodiscard]] constexpr reference get() noexcept {
        return *this;
    }

    [[nodiscard]] constexpr const_reference get() const noexcept {
        return *this;
    }
};

} // namespace internal
/*! @endcond */

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
    constexpr compressed_pair() noexcept(std::is_nothrow_default_constructible_v<first_base> && std::is_nothrow_default_constructible_v<second_base>)
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
    constexpr compressed_pair(compressed_pair &&other) noexcept = default;

    /**
     * @brief Constructs a pair from its values.
     * @tparam Arg Type of value to use to initialize the first element.
     * @tparam Other Type of value to use to initialize the second element.
     * @param arg Value to use to initialize the first element.
     * @param other Value to use to initialize the second element.
     */
    template<typename Arg, typename Other>
    constexpr compressed_pair(Arg &&arg, Other &&other) noexcept(std::is_nothrow_constructible_v<first_base, Arg> && std::is_nothrow_constructible_v<second_base, Other>)
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
    constexpr compressed_pair(std::piecewise_construct_t, std::tuple<Args...> args, std::tuple<Other...> other) noexcept(std::is_nothrow_constructible_v<first_base, Args...> && std::is_nothrow_constructible_v<second_base, Other...>)
        : first_base{std::move(args), std::index_sequence_for<Args...>{}},
          second_base{std::move(other), std::index_sequence_for<Other...>{}} {}

    /*! @brief Default destructor. */
    ~compressed_pair() = default;

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
    constexpr compressed_pair &operator=(compressed_pair &&other) noexcept = default;

    /**
     * @brief Returns the first element that a pair stores.
     * @return The first element that a pair stores.
     */
    [[nodiscard]] constexpr first_type &first() noexcept {
        return static_cast<first_base &>(*this).get();
    }

    /*! @copydoc first */
    [[nodiscard]] constexpr const first_type &first() const noexcept {
        return static_cast<const first_base &>(*this).get();
    }

    /**
     * @brief Returns the second element that a pair stores.
     * @return The second element that a pair stores.
     */
    [[nodiscard]] constexpr second_type &second() noexcept {
        return static_cast<second_base &>(*this).get();
    }

    /*! @copydoc second */
    [[nodiscard]] constexpr const second_type &second() const noexcept {
        return static_cast<const second_base &>(*this).get();
    }

    /**
     * @brief Swaps two compressed pair objects.
     * @param other The compressed pair to swap with.
     */
    constexpr void swap(compressed_pair &other) noexcept {
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
    [[nodiscard]] constexpr decltype(auto) get() noexcept {
        if constexpr(Index == 0u) {
            return first();
        } else {
            static_assert(Index == 1u, "Index out of bounds");
            return second();
        }
    }

    /*! @copydoc get */
    template<std::size_t Index>
    [[nodiscard]] constexpr decltype(auto) get() const noexcept {
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
constexpr void swap(compressed_pair<First, Second> &lhs, compressed_pair<First, Second> &rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace entt

namespace std {

/**
 * @brief `std::tuple_size` specialization for `compressed_pair`s.
 * @tparam First The type of the first element that the pair stores.
 * @tparam Second The type of the second element that the pair stores.
 */
template<typename First, typename Second>
struct tuple_size<entt::compressed_pair<First, Second>>: integral_constant<size_t, 2u> {};

/**
 * @brief `std::tuple_element` specialization for `compressed_pair`s.
 * @tparam Index The index of the type to return.
 * @tparam First The type of the first element that the pair stores.
 * @tparam Second The type of the second element that the pair stores.
 */
template<size_t Index, typename First, typename Second>
struct tuple_element<Index, entt::compressed_pair<First, Second>>: conditional<Index == 0u, First, Second> {
    static_assert(Index < 2u, "Index out of bounds");
};

} // namespace std

#endif
