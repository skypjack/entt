#ifndef ENTT_ENTITY_THROWING_ALLOCATOR_HPP
#define ENTT_ENTITY_THROWING_ALLOCATOR_HPP


#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>


namespace test {


template<typename Type>
class throwing_allocator {
    template<typename Other>
    friend class throwing_allocator;

    struct fancy_pointer final {
        using difference_type = typename std::iterator_traits<Type *>::difference_type;
        using element_type = Type;
        using value_type = element_type;
        using pointer = value_type *;
        using reference = value_type &;
        using iterator_category = std::random_access_iterator_tag;

        fancy_pointer(Type *init = nullptr)
            : ptr{init}
        {}

        fancy_pointer(const fancy_pointer &other)
            : ptr{other.ptr}
        {
            if(throwing_allocator::trigger_on_pointer_copy) {
                throwing_allocator::trigger_on_pointer_copy = false;
                throw test_exception{};
            }
        }

        fancy_pointer & operator++() {
            return ++ptr, *this;
        }

        fancy_pointer operator++(int) {
            auto orig = *this;
            return ++(*this), orig;
        }

        fancy_pointer & operator--() {
            return --ptr, *this;
        }

        fancy_pointer operator--(int) {
            auto orig = *this;
            return operator--(), orig;
        }

        fancy_pointer & operator+=(const difference_type value) {
            return (ptr += value, *this);
        }

        fancy_pointer operator+(const difference_type value) const {
            auto copy = *this;
            return (copy += value);
        }

        fancy_pointer & operator-=(const difference_type value) {
            return (ptr -= value, *this);
        }

        fancy_pointer operator-(const difference_type value) const {
            auto copy = *this;
            return (copy -= value);
        }

        difference_type operator-(const fancy_pointer &other) const {
            return ptr - other.ptr;
        }

        [[nodiscard]] reference operator[](const difference_type value) const {
            return ptr[value];
        }

        [[nodiscard]] bool operator==(const fancy_pointer &other) const {
            return other.ptr == ptr;
        }

        [[nodiscard]] bool operator!=(const fancy_pointer &other) const {
            return !(*this == other);
        }

        [[nodiscard]] bool operator<(const fancy_pointer &other) const {
            return ptr > other.ptr;
        }

        [[nodiscard]] bool operator>(const fancy_pointer &other) const {
            return ptr < other.ptr;
        }

        [[nodiscard]] bool operator<=(const fancy_pointer &other) const {
            return !(*this > other);
        }

        [[nodiscard]] bool operator>=(const fancy_pointer &other) const {
            return !(*this < other);
        }

        explicit operator bool() const {
            return (ptr != nullptr);
        }

        [[nodiscard]] pointer operator->() const {
            return ptr;
        }

        [[nodiscard]] reference operator*() const {
            return *ptr;
        }

    private:
        Type *ptr;
    };

    struct test_exception {};

public:
    using value_type = Type;
    using pointer = fancy_pointer;
    using const_pointer = fancy_pointer;
    using void_pointer = fancy_pointer;
    using const_void_pointer = fancy_pointer;
    using propagate_on_container_move_assignment = std::true_type;
    using exception_type = test_exception;

    constexpr throwing_allocator() = default;

    template<class Other>
    throwing_allocator(const throwing_allocator<Other> &other)
        : allocator{other.allocator}
    {}

    pointer allocate(std::size_t length) {
        if(trigger_on_allocate) {
            trigger_on_allocate = false;
            throw test_exception{};
        }

        trigger_on_allocate = trigger_after_allocate;
        trigger_after_allocate = false;

        return allocator.allocate(length);
    }

    void deallocate(pointer mem, std::size_t length) {
        allocator.deallocate(mem.operator->(), length);
    }

    static inline bool trigger_on_allocate{};
    static inline bool trigger_after_allocate{};
    static inline bool trigger_on_pointer_copy{};

private:
    std::allocator<Type> allocator;
};


}


#endif
