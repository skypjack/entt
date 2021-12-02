#ifndef ENTT_SIGNAL_FWD_HPP
#define ENTT_SIGNAL_FWD_HPP

#include <memory>

namespace entt {

template<typename>
class delegate;

class dispatcher;

template<typename>
class emitter;

class connection;

struct scoped_connection;

template<typename>
class sink;

template<typename Type, typename = std::allocator<delegate<Type>>>
class sigh;

} // namespace entt

#endif
