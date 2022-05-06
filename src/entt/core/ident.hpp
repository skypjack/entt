#ifndef ENTT_CORE_IDENT_HPP
#define ENTT_CORE_IDENT_HPP

#include <cstddef>
#include <type_traits>
#include <utility>
#include "fwd.hpp"
#include "type_traits.hpp"

namespace entt {

/**
 * @brief Type integral identifiers.
 * @tparam Type List of types for which to generate identifiers.
 */
template<typename... Type>
class ident {
    template<typename Curr, std::size_t... Index>
    [[nodiscard]] static constexpr id_type get(std::index_sequence<Index...>) noexcept {
        static_assert((std::is_same_v<Curr, Type> || ...), "Invalid type");
        return (0 + ... + (std::is_same_v<Curr, type_list_element_t<Index, type_list<std::decay_t<Type>...>>> ? id_type{Index} : id_type{}));
    }

public:
    /*! @brief Unsigned integer type. */
    using value_type = id_type;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename Curr>
    static constexpr value_type value = get<std::decay_t<Curr>>(std::index_sequence_for<Type...>{});
};

} // namespace entt

#endif
