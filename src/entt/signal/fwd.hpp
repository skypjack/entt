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

/*! @brief Disambiguation tag for constructors and the like. */
template<auto>
struct connect_arg_t {
    /*! @brief Default constructor. */
    explicit connect_arg_t() = default;
};

/**
 * @brief Constant of type connect_arg_t used to disambiguate calls.
 * @tparam Candidate Element to connect (likely a free or member function).
 */
template<auto Candidate>
inline constexpr connect_arg_t<Candidate> connect_arg{};

} // namespace entt

#endif
