#ifndef ENTT_COMMON_THROWING_ALLOCATOR_HPP
#define ENTT_COMMON_THROWING_ALLOCATOR_HPP

#include <cstddef>
#include <limits>
#include <memory>
#include <type_traits>
#include <entt/container/dense_map.hpp>
#include <entt/core/fwd.hpp>
#include <entt/core/type_info.hpp>

namespace test {

struct throwing_allocator_exception {};

template<typename Type>
class throwing_allocator {
    template<typename Other>
    friend class throwing_allocator;

    template<typename Other, typename = std::enable_if_t<!std::is_void_v<Type> || std::is_constructible_v<std::allocator<Type>, std::allocator<Other>>>>
    throwing_allocator(int, const throwing_allocator<Other> &other)
        : allocator{other.allocator},
          config{other.config} {}

    template<typename Other, typename = std::enable_if_t<std::is_void_v<Type> && !std::is_constructible_v<std::allocator<Type>, std::allocator<Other>>>>
    throwing_allocator(char, const throwing_allocator<Other> &other)
        : allocator{},
          config{other.config} {}

public:
    using value_type = Type;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using void_pointer = void *;
    using const_void_pointer = const void *;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    using container_type = entt::dense_map<entt::id_type, std::size_t>;

    template<typename Other>
    struct rebind {
        using other = throwing_allocator<Other>;
    };

    throwing_allocator()
        : allocator{},
          config{std::allocate_shared<container_type>(allocator)} {}

    template<typename Other>
    throwing_allocator(const throwing_allocator<Other> &other)
        // std::allocator<void> has no cross constructors (waiting for C++20)
        : throwing_allocator{0, other} {}

    pointer allocate(std::size_t length) {
        if(const auto hash = entt::type_id<Type>().hash(); config->contains(hash)) {
            if(auto &elem = (*config)[hash]; elem == 0u) {
                config->erase(hash);
                throw throwing_allocator_exception{};
            } else {
                --elem;
            }
        }

        return allocator.allocate(length);
    }

    void deallocate(pointer mem, std::size_t length) {
        allocator.deallocate(mem, length);
    }

    template<typename Other>
    void throw_counter(const std::size_t len) {
        (*config)[entt::type_id<Other>().hash()] = len;
    }

    bool operator==(const throwing_allocator<Type> &) const {
        return true;
    }

    bool operator!=(const throwing_allocator<Type> &other) const {
        return !(*this == other);
    }

private:
    std::allocator<Type> allocator;
    std::shared_ptr<container_type> config;
};

} // namespace test

#endif
