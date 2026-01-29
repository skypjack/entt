#ifndef ENTT_STL_ITERATOR_HPP
#define ENTT_STL_ITERATOR_HPP

#include "../config/config.h"

/*! @cond ENTT_INTERNAL */
namespace entt::stl {

#ifndef ENTT_FORCE_STL
#    if __has_include(<version>)
#        include <version>
#
#        if defined(__cpp_lib_ranges)
#            define ENTT_HAS_ITERATOR_CONCEPTS
#            include <iterator>
using std::bidirectional_iterator;
using std::forward_iterator;
using std::input_iterator;
using std::input_or_output_iterator;
using std::output_iterator;
using std::random_access_iterator;
using std::sentinel_for;
#        endif
#    endif
#endif

#ifndef ENTT_HAS_ITERATOR_CONCEPTS
#    include <concepts>
#    include <iterator>
#    include <utility>

namespace internal {

template<typename It>
requires requires { typename std::iterator_traits<It>::iterator_category; }
struct iterator_tag {
    using type = typename std::iterator_traits<It>::iterator_category;
};

template<typename It>
requires requires { typename It::iterator_concept; }
struct iterator_tag<It> {
    using type = typename It::iterator_concept;
};

template<typename It, typename Tag>
concept has_iterator_tag = std::derived_from<typename iterator_tag<It>::type, Tag>;

} // namespace internal

// Bare minimum definitions to support broken platforms like PS4.
// EnTT does not provide full featured definitions for iterator concepts.

template<typename It>
concept input_or_output_iterator = requires(It it) {
    *it;
    { ++it } -> std::same_as<It &>;
    it++;
};

template<typename It>
concept input_iterator = input_or_output_iterator<It> && internal::has_iterator_tag<It, std::input_iterator_tag>;

template<typename It, typename Type>
concept output_iterator = input_or_output_iterator<It> && requires(It it, Type &&value) {
    *it++ = std::forward<Type>(value);
};

template<typename It>
concept forward_iterator = input_iterator<It> && internal::has_iterator_tag<It, std::forward_iterator_tag>;

template<typename It>
concept bidirectional_iterator = forward_iterator<It> && internal::has_iterator_tag<It, std::bidirectional_iterator_tag>;

template<typename It>
concept random_access_iterator = bidirectional_iterator<It> && internal::has_iterator_tag<It, std::random_access_iterator_tag>;

template<class Sentinel, typename It>
concept sentinel_for = input_or_output_iterator<It> && requires(Sentinel sentinel, It it) {
    { it == sentinel } -> std::same_as<bool>;
};
#endif

} // namespace entt::stl
/*! @endcond */

#endif
