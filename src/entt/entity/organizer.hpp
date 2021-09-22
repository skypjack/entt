#ifndef ENTT_ENTITY_ORGANIZER_HPP
#define ENTT_ENTITY_ORGANIZER_HPP

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
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

template<typename Entity, typename... Component, typename... Exclude>
struct is_view<basic_view<Entity, get_t<Component...>, exclude_t<Exclude...>>>: std::true_type {};

template<typename Type>
inline constexpr bool is_view_v = is_view<Type>::value;

template<typename Type, typename Override>
struct unpack_type {
    using ro = std::conditional_t<
        type_list_contains_v<Override, std::add_const_t<Type>> || (std::is_const_v<Type> && !type_list_contains_v<Override, std::remove_const_t<Type>>),
        type_list<std::remove_const_t<Type>>,
        type_list<>>;

    using rw = std::conditional_t<
        type_list_contains_v<Override, std::remove_const_t<Type>> || (!std::is_const_v<Type> && !type_list_contains_v<Override, std::add_const_t<Type>>),
        type_list<Type>,
        type_list<>>;
};

template<typename Entity, typename... Override>
struct unpack_type<basic_registry<Entity>, type_list<Override...>> {
    using ro = type_list<>;
    using rw = type_list<>;
};

template<typename Entity, typename... Override>
struct unpack_type<const basic_registry<Entity>, type_list<Override...>>
    : unpack_type<basic_registry<Entity>, type_list<Override...>> {};

template<typename Entity, typename... Component, typename... Exclude, typename... Override>
struct unpack_type<basic_view<Entity, get_t<Component...>, exclude_t<Exclude...>>, type_list<Override...>> {
    using ro = type_list_cat_t<type_list<Exclude...>, typename unpack_type<Component, type_list<Override...>>::ro...>;
    using rw = type_list_cat_t<typename unpack_type<Component, type_list<Override...>>::rw...>;
};

template<typename Entity, typename... Component, typename... Exclude, typename... Override>
struct unpack_type<const basic_view<Entity, get_t<Component...>, exclude_t<Exclude...>>, type_list<Override...>>
    : unpack_type<basic_view<Entity, get_t<Component...>, exclude_t<Exclude...>>, type_list<Override...>> {};

template<typename, typename>
struct resource;

template<typename... Args, typename... Req>
struct resource<type_list<Args...>, type_list<Req...>> {
    using args = type_list<std::remove_const_t<Args>...>;
    using ro = type_list_cat_t<typename unpack_type<Args, type_list<Req...>>::ro..., typename unpack_type<Req, type_list<>>::ro...>;
    using rw = type_list_cat_t<typename unpack_type<Args, type_list<Req...>>::rw..., typename unpack_type<Req, type_list<>>::rw...>;
};

template<typename... Req, typename Ret, typename... Args>
resource<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> free_function_to_resource(Ret (*)(Args...));

template<typename... Req, typename Ret, typename Type, typename... Args>
resource<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> constrained_function_to_resource(Ret (*)(Type &, Args...));

template<typename... Req, typename Ret, typename Class, typename... Args>
resource<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> constrained_function_to_resource(Ret (Class::*)(Args...));

template<typename... Req, typename Ret, typename Class, typename... Args>
resource<type_list<std::remove_reference_t<Args>...>, type_list<Req...>> constrained_function_to_resource(Ret (Class::*)(Args...) const);

template<typename... Req>
resource<type_list<>, type_list<Req...>> to_resource();

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
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_organizer final {
    using callback_type = void(const void *, basic_registry<Entity> &);
    using prepare_type = void(basic_registry<Entity> &);
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
    [[nodiscard]] static decltype(auto) extract(basic_registry<Entity> &reg) {
        if constexpr(std::is_same_v<Type, basic_registry<Entity>>) {
            return reg;
        } else if constexpr(internal::is_view_v<Type>) {
            return as_view{reg};
        } else {
            return reg.template ctx_or_set<std::remove_reference_t<Type>>();
        }
    }

    template<typename... Args>
    [[nodiscard]] static auto to_args(basic_registry<Entity> &reg, type_list<Args...>) {
        return std::tuple<decltype(extract<Args>(reg))...>(extract<Args>(reg)...);
    }

    template<typename... Type>
    static std::size_t fill_dependencies(type_list<Type...>, [[maybe_unused]] const type_info **buffer, [[maybe_unused]] const std::size_t count) {
        if constexpr(sizeof...(Type) == 0u) {
            return {};
        } else {
            const type_info *info[sizeof...(Type)]{&type_id<Type>()...};
            const auto length = (std::min)(count, sizeof...(Type));
            std::copy_n(info, length, buffer);
            return length;
        }
    }

    template<typename... RO, typename... RW>
    void track_dependencies(std::size_t index, const bool requires_registry, type_list<RO...>, type_list<RW...>) {
        dependencies[type_hash<basic_registry<Entity>>::value()].emplace_back(index, requires_registry || (sizeof...(RO) + sizeof...(RW) == 0u));
        (dependencies[type_hash<RO>::value()].emplace_back(index, false), ...);
        (dependencies[type_hash<RW>::value()].emplace_back(index, true), ...);
    }

    [[nodiscard]] std::vector<bool> adjacency_matrix() {
        const auto length = vertices.size();
        std::vector<bool> edges(length * length, false);

        // creates the ajacency matrix
        for(const auto &deps: dependencies) {
            const auto last = deps.second.cend();
            auto it = deps.second.cbegin();

            while(it != last) {
                if(it->second) {
                    // rw item
                    if(auto curr = it++; it != last) {
                        if(it->second) {
                            edges[curr->first * length + it->first] = true;
                        } else {
                            if(const auto next = std::find_if(it, last, [](const auto &elem) { return elem.second; }); next != last) {
                                for(; it != next; ++it) {
                                    edges[curr->first * length + it->first] = true;
                                    edges[it->first * length + next->first] = true;
                                }
                            } else {
                                for(; it != next; ++it) {
                                    edges[curr->first * length + it->first] = true;
                                }
                            }
                        }
                    }
                } else {
                    // ro item, possibly only on first iteration
                    if(const auto next = std::find_if(it, last, [](const auto &elem) { return elem.second; }); next != last) {
                        for(; it != next; ++it) {
                            edges[it->first * length + next->first] = true;
                        }
                    } else {
                        it = last;
                    }
                }
            }
        }

        // computes the transitive closure
        for(std::size_t vk{}; vk < length; ++vk) {
            for(std::size_t vi{}; vi < length; ++vi) {
                for(std::size_t vj{}; vj < length; ++vj) {
                    edges[vi * length + vj] = edges[vi * length + vj] || (edges[vi * length + vk] && edges[vk * length + vj]);
                }
            }
        }

        // applies the transitive reduction
        for(std::size_t vert{}; vert < length; ++vert) {
            edges[vert * length + vert] = false;
        }

        for(std::size_t vj{}; vj < length; ++vj) {
            for(std::size_t vi{}; vi < length; ++vi) {
                if(edges[vi * length + vj]) {
                    for(std::size_t vk{}; vk < length; ++vk) {
                        if(edges[vj * length + vk]) {
                            edges[vi * length + vk] = false;
                        }
                    }
                }
            }
        }

        return edges;
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
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
        size_type ro_dependency(const type_info **buffer, const std::size_t length) const ENTT_NOEXCEPT {
            return node.dependency(false, buffer, length);
        }

        /**
         * @brief Fills a buffer with the type info objects for the read-only
         * resources of a vertex.
         * @param buffer A buffer pre-allocated by the user.
         * @param length The length of the user-supplied buffer.
         * @return The number of type info objects written to the buffer.
         */
        size_type rw_dependency(const type_info **buffer, const std::size_t length) const ENTT_NOEXCEPT {
            return node.dependency(true, buffer, length);
        }

        /**
         * @brief Returns the number of read-only resources of a vertex.
         * @return The number of read-only resources of the vertex.
         */
        size_type ro_count() const ENTT_NOEXCEPT {
            return node.ro_count;
        }

        /**
         * @brief Returns the number of writable resources of a vertex.
         * @return The number of writable resources of the vertex.
         */
        size_type rw_count() const ENTT_NOEXCEPT {
            return node.rw_count;
        }

        /**
         * @brief Checks if a vertex is also a top-level one.
         * @return True if the vertex is a top-level one, false otherwise.
         */
        bool top_level() const ENTT_NOEXCEPT {
            return is_top_level;
        }

        /**
         * @brief Returns a type info object associated with a vertex.
         * @return A properly initialized type info object.
         */
        const type_info &info() const ENTT_NOEXCEPT {
            return *node.info;
        }

        /**
         * @brief Returns a user defined name associated with a vertex, if any.
         * @return The user defined name associated with the vertex, if any.
         */
        const char *name() const ENTT_NOEXCEPT {
            return node.name;
        }

        /**
         * @brief Returns the function associated with a vertex.
         * @return The function associated with the vertex.
         */
        function_type *callback() const ENTT_NOEXCEPT {
            return node.callback;
        }

        /**
         * @brief Returns the payload associated with a vertex, if any.
         * @return The payload associated with the vertex, if any.
         */
        const void *data() const ENTT_NOEXCEPT {
            return node.payload;
        }

        /**
         * @brief Returns the list of nodes reachable from a given vertex.
         * @return The list of nodes reachable from the vertex.
         */
        const std::vector<std::size_t> &children() const ENTT_NOEXCEPT {
            return reachable;
        }

        /**
         * @brief Prepares a registry and assures that all required resources
         * are properly instantiated before using them.
         * @param reg A valid registry.
         */
        void prepare(basic_registry<entity_type> &reg) const {
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
        using resource_type = decltype(internal::free_function_to_resource<Req...>(Candidate));
        constexpr auto requires_registry = type_list_contains_v<typename resource_type::args, basic_registry<entity_type>>;

        callback_type *callback = +[](const void *, basic_registry<entity_type> &reg) {
            std::apply(Candidate, to_args(reg, typename resource_type::args{}));
        };

        vertex_data vdata{
            resource_type::ro::size,
            resource_type::rw::size,
            name,
            nullptr,
            callback,
            +[](const bool rw, const type_info **buffer, const std::size_t length) { return rw ? fill_dependencies(typename resource_type::rw{}, buffer, length) : fill_dependencies(typename resource_type::ro{}, buffer, length); },
            +[](basic_registry<entity_type> &reg) { void(to_args(reg, typename resource_type::args{})); },
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
        using resource_type = decltype(internal::constrained_function_to_resource<Req...>(Candidate));
        constexpr auto requires_registry = type_list_contains_v<typename resource_type::args, basic_registry<entity_type>>;

        callback_type *callback = +[](const void *payload, basic_registry<entity_type> &reg) {
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
            +[](basic_registry<entity_type> &reg) { void(to_args(reg, typename resource_type::args{})); },
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
        using resource_type = internal::resource<type_list<>, type_list<Req...>>;
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
        const auto edges = adjacency_matrix();

        // creates the adjacency list
        std::vector<vertex> adjacency_list{};
        adjacency_list.reserve(vertices.size());

        for(std::size_t col{}, length = vertices.size(); col < length; ++col) {
            std::vector<std::size_t> reachable{};
            const auto row = col * length;
            bool is_top_level = true;

            for(std::size_t next{}; next < length; ++next) {
                if(edges[row + next]) {
                    reachable.push_back(next);
                }
            }

            for(std::size_t next{}; next < length && is_top_level; ++next) {
                is_top_level = !edges[next * length + col];
            }

            adjacency_list.emplace_back(is_top_level, vertices[col], std::move(reachable));
        }

        return adjacency_list;
    }

    /*! @brief Erases all elements from a container. */
    void clear() {
        dependencies.clear();
        vertices.clear();
    }

private:
    std::unordered_map<id_type, std::vector<std::pair<std::size_t, bool>>> dependencies;
    std::vector<vertex_data> vertices;
};

} // namespace entt

#endif
