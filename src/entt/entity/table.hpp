#ifndef ENTT_ENTITY_TABLE_HPP
#define ENTT_ENTITY_TABLE_HPP

#include "fwd.hpp"

namespace entt {

template<typename... Type, typename Allocator>
struct basic_table<type_list<Type...>, Allocator> {
};

} // namespace entt

#endif