#ifndef ENTT_META_TEMPLATE_HPP
#define ENTT_META_TEMPLATE_HPP

#include "../core/type_traits.hpp"

namespace entt {

/*! @brief Utility class to disambiguate class templates. */
template<template<typename...> typename>
struct meta_class_template_tag {};

/**
 * @brief General purpose traits class for generating meta template information.
 * @tparam Clazz Type of class template.
 * @tparam Args Types of template arguments.
 */
template<template<typename...> typename Clazz, typename... Args>
struct meta_template_traits<Clazz<Args...>> {
    /*! @brief Wrapped class template. */
    using class_type = meta_class_template_tag<Clazz>;
    /*! @brief List of template arguments. */
    using args_type = type_list<Args...>;
};

} // namespace entt

#endif
