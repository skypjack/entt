#ifndef ENTT_CORE_IDENT_HPP
#define ENTT_CORE_IDENT_HPP

#include "../stl/cstddef.hpp"
#include "../stl/type_traits.hpp"
#include "../stl/utility.hpp"
#include "fwd.hpp"
#include "type_traits.hpp"

namespace entt {

/**
 * @brief Type integral identifiers.
 * @tparam Type List of types for which to generate identifiers.
 */
template<typename... Type>
class ident {
    template<typename Curr, stl::size_t... Index>
    [[nodiscard]] static ENTT_CONSTEVAL id_type get(std::index_sequence<Index...>) noexcept {
        return (0 + ... + (stl::is_same_v<Curr, type_list_element_t<Index, type_list<stl::decay_t<Type>...>>> ? id_type{Index} : id_type{}));
    }

public:
    /*! @brief Unsigned integer type. */
    using value_type = id_type;

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename Curr>
    requires (stl::is_same_v<stl::remove_cvref_t<Curr>, Type> || ...)
    static constexpr value_type value = get<stl::remove_cvref_t<Curr>>(stl::index_sequence_for<Type...>{});
};

} // namespace entt

#endif
