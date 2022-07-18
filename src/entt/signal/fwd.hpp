#ifndef ENTT_SIGNAL_FWD_HPP
#define ENTT_SIGNAL_FWD_HPP

#include <memory>

namespace entt {

template<typename>
class delegate;

template<typename = std::allocator<void>>
class basic_dispatcher;

template<typename, typename = std::allocator<void>>
class emitter;

class connection;

struct scoped_connection;

template<typename>
class sink;

template<typename Type, typename = std::allocator<void>>
class sigh;

/*! @brief Alias declaration for the most common use case. */
using dispatcher = basic_dispatcher<>;

} // namespace entt

#endif
