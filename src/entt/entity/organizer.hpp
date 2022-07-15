#ifndef ENTT_ENTITY_ORGANIZER_HPP
#define ENTT_ENTITY_ORGANIZER_HPP

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "../graph/adjacency_matrix.hpp"
#include "../graph/flow.hpp"
#include "fwd.hpp"
#include "helper.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename>
struct is_view: std::false_type {};

template<typename... Get, typename... Exclude>
struct is_view<basic_view<get_t<Get...>, exclude_t<Exclude...>>>: std::true_type {};

template<typename Type>
inline constexpr bool is_view_v = is_view<Type>::value;

template<typename Type, typename Override>
struct unpack_type {
    using ro = std::conditional_t<
        type_list_contains_v<Override, const Type> || (std::is_const_v<Type> && !type_list_contains_v<Override, std::remove_const_t<Type>>),
        type_list<std::remove_const_t<Type>>,
        type_list<>>;

    using rw = std::conditional_t<
        type_list_contains_v<Override, std::remove_const_t<Type>> || (!std::is_const_v<Type> && !type_list_contains_v<Override, const Type>),
        type_list<Type>,
        type_list<>>;
};

template<typename... Args, typename... Override>
struct unpack_type<basic_registry<Args...>, type_list<Override...>> {
    using ro = type_list<>;
    using rw = type_list<>;
};

template<typename... Args, typename... Override>
struct unpack_type<const basic_registry<Args...>, type_list<Override...>>
    : unpack_type<basic_registry<Args...>, type_list<Override...>> {};

template<typename... Get, typename... Exclude, typename... Override>
struct unpack_type<basic_view<get_t<Get...>, exclude_t<Exclude...>>, type_list<Override...>> {
    using ro = type_list_cat_t<type_list<typename Exclude::value_type...>, typename unpack_type<constness_as_t<typename Get::value_type, Get>, type_list<Override...>>::ro...>;
    using rw = type_list_cat_t<typename unpack_type<constness_as_t<typename Get::value_type, Get>, type_list<Override...>>::rw...>;
};

template<typename... Get, typename... Exclude, typename... Override>
struct unpack_type<const basic_view<get_t<Get...>, exclude_t<Exclude...>>, type_list<Override...>>
    : unpack_type<basic_view<get_t<Get...>, exclude_t<Exclude...>>, type_list<Override...>> {};

template<typename, typename>
struct resource_traits;

template<typename... Args, typename... Req>
struct resource_traits<type_list<Args...>, type_list<Req...>> {
    using args = type_list<std::remove_const_t<Args>...>;
    using ro = type_list_cat_t<typename unpack_type<Args, type_list<Req...>>::ro..., typename unpack_type<Req, type_list<>>::ro...>;
    using rw = type_list_cat_t<typename unpack_type<Args, type_list<Req...>>::rw..., typename unpack_type<Req, type_list<>>::rw...>;
};

template<typename... Req, typename Ret, typename... Args>
resource_traits<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> free_function_to_resource_traits(Ret (*)(Args...));

template<typename... Req, typename Ret, typename Type, typename... Args>
resource_traits<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> constrained_function_to_resource_traits(Ret (*)(Type &, Args...));

template<typename... Req, typename Ret, typename Class, typename... Args>
resource_traits<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> constrained_function_to_resource_traits(Ret (Class::*)(Args...));

template<typename... Req, typename Ret, typename Class, typename... Args>
resource_traits<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> constrained_function_to_resource_traits(Ret (Class::*)(Args...) const);

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Utility class for creating a static task graph.
 *
 * This class offers minimal support (but sufficient in many cases) for creating
 * an execution graph from functions and their requirements on resources.<br/>
 * Note that the resulting tasks aren't executed in any case. This isn't the
 * goal of the tool. Instead, they are returned to the user in the form of a
 * graph that allows for safe execution.
 *
 * @tparam Registry Basic registry type.
 */
template<typename Registry>
class basic_organizer final {
    using callback_type = void(const void *, Registry &);
    using prepare_type = void(Registry &);
    using dependency_type = std::size_t(const bool, const type_info **, const std::size_t);

    struct vertex_data final {
        std::size_t ro_count{};
        std::size_t rw_count{};
        const char *name{};
        const void *payload{};
        callback_type *callback{};
        dependency_type *dependency;
        prepare_type *prepare{};
        const type_info *info{};
    };

    template<typename Type>
    [[nodiscard]] static decltype(auto) extract(Registry &reg) {
        if constexpr(std::is_same_v<Type, Registry>) {
            return reg;
        } else if constexpr(internal::is_view_v<Type>) {
            return as_view{reg};
        } else {
            return reg.ctx().template emplace<std::remove_reference_t<Type>>();
        }
    }

    template<typename... Args>
    [[nodiscard]] static auto to_args(Registry &reg, type_list<Args...>) {
        return std::tuple<decltype(extract<Args>(reg))...>(extract<Args>(reg)...);
    }

    template<typename... Type>
    static std::size_t fill_dependencies(type_list<Type...>, [[maybe_unused]] const type_info **buffer, [[maybe_unused]] const std::size_t count) {
        if constexpr(sizeof...(Type) == 0u) {
            return {};
        } else {
            const type_info *info[sizeof...(Type)]{&type_id<Type>()...};
            const auto length = count < sizeof...(Type) ? count : sizeof...(Type);

            for(std::size_t pos{}; pos < length; ++pos) {
                buffer[pos] = info[pos];
            }

            return length;
        }
    }

    template<typename... RO, typename... RW>
    void track_dependencies(std::size_t index, const bool requires_registry, type_list<RO...>, type_list<RW...>) {
        builder.bind(static_cast<id_type>(index));
        builder.set(type_hash<Registry>::value(), requires_registry || (sizeof...(RO) + sizeof...(RW) == 0u));
        (builder.ro(type_hash<RO>::value()), ...);
        (builder.rw(type_hash<RW>::value()), ...);
    }

public:
    /*! Basic registry type. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Raw task function type. */
    using function_type = callback_type;

    /*! @brief Vertex type of a task graph defined as an adjacency list. */
    struct vertex {
        /**
         * @brief Constructs a vertex of the task graph.
         * @param vtype True if the vertex is a top-level one, false otherwise.
         * @param data The data associated with the vertex.
         * @param edges The indices of the children in the adjacency list.
         */
        vertex(const bool vtype, vertex_data data, std::vector<std::size_t> edges)
            : is_top_level{vtype},
              node{std::move(data)},
              reachable{std::move(edges)} {}

        /**
         * @brief Fills a buffer with the type info objects for the writable
         * resources of a vertex.
         * @param buffer A buffer pre-allocated by the user.
         * @param length The length of the user-supplied buffer.
         * @return The number of type info objects written to the buffer.
         */
        size_type ro_dependency(const type_info **buffer, const std::size_t length) const noexcept {
            return node.dependency(false, buffer, length);
        }

        /**
         * @brief Fills a buffer with the type info objects for the read-only
         * resources of a vertex.
         * @param buffer A buffer pre-allocated by the user.
         * @param length The length of the user-supplied buffer.
         * @return The number of type info objects written to the buffer.
         */
        size_type rw_dependency(const type_info **buffer, const std::size_t length) const noexcept {
            return node.dependency(true, buffer, length);
        }

        /**
         * @brief Returns the number of read-only resources of a vertex.
         * @return The number of read-only resources of the vertex.
         */
        size_type ro_count() const noexcept {
            return node.ro_count;
        }

        /**
         * @brief Returns the number of writable resources of a vertex.
         * @return The number of writable resources of the vertex.
         */
        size_type rw_count() const noexcept {
            return node.rw_count;
        }

        /**
         * @brief Checks if a vertex is also a top-level one.
         * @return True if the vertex is a top-level one, false otherwise.
         */
        bool top_level() const noexcept {
            return is_top_level;
        }

        /**
         * @brief Returns a type info object associated with a vertex.
         * @return A properly initialized type info object.
         */
        const type_info &info() const noexcept {
            return *node.info;
        }

        /**
         * @brief Returns a user defined name associated with a vertex, if any.
         * @return The user defined name associated with the vertex, if any.
         */
        const char *name() const noexcept {
            return node.name;
        }

        /**
         * @brief Returns the function associated with a vertex.
         * @return The function associated with the vertex.
         */
        function_type *callback() const noexcept {
            return node.callback;
        }

        /**
         * @brief Returns the payload associated with a vertex, if any.
         * @return The payload associated with the vertex, if any.
         */
        const void *data() const noexcept {
            return node.payload;
        }

        /**
         * @brief Returns the list of nodes reachable from a given vertex.
         * @return The list of nodes reachable from the vertex.
         */
        const std::vector<std::size_t> &children() const noexcept {
            return reachable;
        }

        /**
         * @brief Prepares a registry and assures that all required resources
         * are properly instantiated before using them.
         * @param reg A valid registry.
         */
        void prepare(registry_type &reg) const {
            node.prepare ? node.prepare(reg) : void();
        }

    private:
        bool is_top_level;
        vertex_data node;
        std::vector<std::size_t> reachable;
    };

    /**
     * @brief Adds a free function to the task list.
     * @tparam Candidate Function to add to the task list.
     * @tparam Req Additional requirements and/or override resource access mode.
     * @param name Optional name to associate with the task.
     */
    template<auto Candidate, typename... Req>
    void emplace(const char *name = nullptr) {
        using resource_type = decltype(internal::free_function_to_resource_traits<Req...>(Candidate));
        constexpr auto requires_registry = type_list_contains_v<typename resource_type::args, registry_type>;

        callback_type *callback = +[](const void *, registry_type &reg) {
            std::apply(Candidate, to_args(reg, typename resource_type::args{}));
        };

        vertex_data vdata{
            resource_type::ro::size,
            resource_type::rw::size,
            name,
            nullptr,
            callback,
            +[](const bool rw, const type_info **buffer, const std::size_t length) { return rw ? fill_dependencies(typename resource_type::rw{}, buffer, length) : fill_dependencies(typename resource_type::ro{}, buffer, length); },
            +[](registry_type &reg) { void(to_args(reg, typename resource_type::args{})); },
            &type_id<std::integral_constant<decltype(Candidate), Candidate>>()};

        track_dependencies(vertices.size(), requires_registry, typename resource_type::ro{}, typename resource_type::rw{});
        vertices.push_back(std::move(vdata));
    }

    /**
     * @brief Adds a free function with payload or a member function with an
     * instance to the task list.
     * @tparam Candidate Function or member to add to the task list.
     * @tparam Req Additional requirements and/or override resource access mode.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @param name Optional name to associate with the task.
     */
    template<auto Candidate, typename... Req, typename Type>
    void emplace(Type &value_or_instance, const char *name = nullptr) {
        using resource_type = decltype(internal::constrained_function_to_resource_traits<Req...>(Candidate));
        constexpr auto requires_registry = type_list_contains_v<typename resource_type::args, registry_type>;

        callback_type *callback = +[](const void *payload, registry_type &reg) {
            Type *curr = static_cast<Type *>(const_cast<constness_as_t<void, Type> *>(payload));
            std::apply(Candidate, std::tuple_cat(std::forward_as_tuple(*curr), to_args(reg, typename resource_type::args{})));
        };

        vertex_data vdata{
            resource_type::ro::size,
            resource_type::rw::size,
            name,
            &value_or_instance,
            callback,
            +[](const bool rw, const type_info **buffer, const std::size_t length) { return rw ? fill_dependencies(typename resource_type::rw{}, buffer, length) : fill_dependencies(typename resource_type::ro{}, buffer, length); },
            +[](registry_type &reg) { void(to_args(reg, typename resource_type::args{})); },
            &type_id<std::integral_constant<decltype(Candidate), Candidate>>()};

        track_dependencies(vertices.size(), requires_registry, typename resource_type::ro{}, typename resource_type::rw{});
        vertices.push_back(std::move(vdata));
    }

    /**
     * @brief Adds an user defined function with optional payload to the task
     * list.
     * @tparam Req Additional requirements and/or override resource access mode.
     * @param func Function to add to the task list.
     * @param payload User defined arbitrary data.
     * @param name Optional name to associate with the task.
     */
    template<typename... Req>
    void emplace(function_type *func, const void *payload = nullptr, const char *name = nullptr) {
        using resource_type = internal::resource_traits<type_list<>, type_list<Req...>>;
        track_dependencies(vertices.size(), true, typename resource_type::ro{}, typename resource_type::rw{});

        vertex_data vdata{
            resource_type::ro::size,
            resource_type::rw::size,
            name,
            payload,
            func,
            +[](const bool rw, const type_info **buffer, const std::size_t length) { return rw ? fill_dependencies(typename resource_type::rw{}, buffer, length) : fill_dependencies(typename resource_type::ro{}, buffer, length); },
            nullptr,
            &type_id<void>()};

        vertices.push_back(std::move(vdata));
    }

    /**
     * @brief Generates a task graph for the current content.
     * @return The adjacency list of the task graph.
     */
    std::vector<vertex> graph() {
        std::vector<vertex> adjacency_list{};
        adjacency_list.reserve(vertices.size());
        auto adjacency_matrix = builder.graph();

        for(auto curr: adjacency_matrix.vertices()) {
            const auto iterable = adjacency_matrix.in_edges(curr);
            std::vector<std::size_t> reachable{};

            for(auto &&edge: adjacency_matrix.out_edges(curr)) {
                reachable.push_back(edge.second);
            }

            adjacency_list.emplace_back(iterable.cbegin() == iterable.cend(), vertices[curr], std::move(reachable));
        }

        return adjacency_list;
    }

    /*! @brief Erases all elements from a container. */
    void clear() {
        builder.clear();
        vertices.clear();
    }

private:
    std::vector<vertex_data> vertices;
    flow builder;
};

} // namespace entt

#endif
