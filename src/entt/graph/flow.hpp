#ifndef ENTT_GRAPH_FLOW_HPP
#define ENTT_GRAPH_FLOW_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../container/dense_set.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/utility.hpp"
#include "adjacency_matrix.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @brief Utility class for creating task graphs.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Allocator>
class basic_flow {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, id_type>, "Invalid value type");
    using task_container_type = dense_set<id_type, identity, std::equal_to<id_type>, typename alloc_traits::template rebind_alloc<id_type>>;
    using ro_rw_container_type = std::vector<std::pair<std::size_t, bool>, typename alloc_traits::template rebind_alloc<std::pair<std::size_t, bool>>>;
    using deps_container_type = dense_map<id_type, ro_rw_container_type, identity, std::equal_to<id_type>, typename alloc_traits::template rebind_alloc<std::pair<const id_type, ro_rw_container_type>>>;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Iterable task list. */
    using iterable = iterable_adaptor<typename task_container_type::const_iterator>;

    /*! @brief Default constructor. */
    basic_flow()
        : basic_flow{allocator_type{}} {}

    /**
     * @brief Constructs a flow builder with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_flow(const allocator_type &allocator)
        : index{0u, allocator},
          vertices{},
          deps{} {}

    /*! @brief Default copy constructor. */
    basic_flow(const basic_flow &) = default;

    /**
     * @brief Allocator-extended copy constructor.
     * @param other The instance to copy from.
     * @param allocator The allocator to use.
     */
    basic_flow(const basic_flow &other, const allocator_type &allocator)
        : index{other.index.first(), allocator},
          vertices{other.vertices, allocator},
          deps{other.deps, allocator} {}

    /*! @brief Default move constructor. */
    basic_flow(basic_flow &&) noexcept = default;

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_flow(basic_flow &&other, const allocator_type &allocator)
        : index{other.index.first(), allocator},
          vertices{std::move(other.vertices), allocator},
          deps{std::move(other.deps), allocator} {}

    /**
     * @brief Default copy assignment operator.
     * @return This flow builder.
     */
    basic_flow &operator=(const basic_flow &) = default;

    /**
     * @brief Default move assignment operator.
     * @return This flow builder.
     */
    basic_flow &operator=(basic_flow &&) noexcept = default;

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return allocator_type{index.second()};
    }

    /*! @brief Clears the flow builder. */
    void clear() noexcept {
        index.first() = 0u;
        vertices.clear();
        deps.clear();
    }

    /**
     * @brief Exchanges the contents with those of a given flow builder.
     * @param other Flow builder to exchange the content with.
     */
    void swap(basic_flow &other) {
        using std::swap;
        std::swap(index, other.index);
        std::swap(vertices, other.vertices);
        std::swap(deps, other.deps);
    }

    /**
     * @brief Returns the number of tasks.
     * @return The number of tasks.
     */
    [[nodiscard]] size_type size() const noexcept {
        return vertices.size();
    }

    /**
     * @brief Sets the current task.
     * @param value Task identifier.
     * @return This flow builder.
     */
    basic_flow &task(id_type value) {
        const auto it = vertices.emplace(value).first;
        index.first() = size_type(it - vertices.begin());
        return *this;
    }

    /**
     * @brief Returns an iterable object to use to _visit_ the tasks.
     * @return An iterable object to use to _visit_ the tasks.
     */
    iterable tasks() const noexcept {
        return {vertices.cbegin(), vertices.cend()};
    }

    /**
     * @brief Assigns a read-only resource to the current task.
     * @param res Resource identifier.
     * @return This flow builder.
     */
    basic_flow &ro(id_type res) {
        ENTT_ASSERT(index.first() < vertices.size(), "Invalid task");
        deps[res].emplace_back(index.first(), false);
        return *this;
    }

    /**
     * @brief Assigns a range of read-only resources to the current task.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of elements.
     * @param last An iterator past the last element of the range of elements.
     * @return This flow builder.
     */
    template<typename It>
    std::enable_if_t<std::is_same_v<std::remove_const_t<typename std::iterator_traits<It>::value_type>, id_type>, basic_flow &>
    ro(It first, It last) {
        for(; first != last; ++first) {
            ro(*first);
        }

        return *this;
    }

    /**
     * @brief Assigns a writable resource to the current task.
     * @param res Resource identifier.
     * @return This flow builder.
     */
    basic_flow &rw(id_type res) {
        ENTT_ASSERT(index.first() < vertices.size(), "Invalid task");
        deps[res].emplace_back(index.first(), true);
        return *this;
    }

    /**
     * @brief Assigns a range of writable resources to the current task.
     * @tparam It Type of input iterator.
     * @param first An iterator to the first element of the range of elements.
     * @param last An iterator past the last element of the range of elements.
     * @return This flow builder.
     */
    template<typename It>
    std::enable_if_t<std::is_same_v<std::remove_const_t<typename std::iterator_traits<It>::value_type>, id_type>, basic_flow &>
    rw(It first, It last) {
        for(; first != last; ++first) {
            rw(*first);
        }

        return *this;
    }

    /**
     * @brief Generates a task graph for the current content.
     * @return The adjacency matrix of the task graph.
     */
    adjacency_matrix graph() const {
        const auto length = vertices.size();
        adjacency_matrix matrix{length};

        // creates the adjacency matrix
        for(const auto &elem: deps) {
            const auto last = elem.second.cend();
            auto it = elem.second.cbegin();

            while(it != last) {
                if(it->second) {
                    // rw item
                    if(auto curr = it++; it != last) {
                        if(it->second) {
                            matrix.insert(curr->first, it->first);
                        } else if(const auto next = std::find_if(it, last, [](const auto &value) { return value.second; }); next != last) {
                            for(; it != next; ++it) {
                                matrix.insert(curr->first, it->first);
                                matrix.insert(it->first, next->first);
                            }
                        } else {
                            for(; it != next; ++it) {
                                matrix.insert(curr->first, it->first);
                            }
                        }
                    }
                } else {
                    // ro item (first iteration only)
                    if(const auto next = std::find_if(it, last, [](const auto &value) { return value.second; }); next != last) {
                        for(; it != next; ++it) {
                            matrix.insert(it->first, next->first);
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
                    if(matrix.contains(vi, vk) && matrix.contains(vk, vj)) {
                        matrix.insert(vi, vj);
                    }
                }
            }
        }

        // applies the transitive reduction
        for(std::size_t vert{}; vert < length; ++vert) {
            matrix.erase(vert, vert);
        }

        for(std::size_t vj{}; vj < length; ++vj) {
            for(std::size_t vi{}; vi < length; ++vi) {
                if(matrix.contains(vi, vj)) {
                    for(std::size_t vk{}; vk < length; ++vk) {
                        if(matrix.contains(vj, vk)) {
                            matrix.erase(vi, vk);
                        }
                    }
                }
            }
        }

        return matrix;
    }

private:
    compressed_pair<size_type, allocator_type> index;
    task_container_type vertices;
    deps_container_type deps;
};

} // namespace entt

#endif
