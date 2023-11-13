#ifndef ENTT_COMMON_THROWING_TYPE_HPP
#define ENTT_COMMON_THROWING_TYPE_HPP

namespace test {

class throwing_type {
    struct test_exception {};

public:
    using exception_type = test_exception;

    throwing_type(bool mode)
        : trigger{mode} {}

    throwing_type(const throwing_type &other)
        : trigger{other.trigger} {
        if(trigger) {
            throw exception_type{};
        }
    }

    throwing_type &operator=(const throwing_type &other) {
        if(trigger) {
            throw exception_type{};
        }

        trigger = other.trigger;
        return *this;
    }

    void throw_on_copy(const bool mode) noexcept {
        trigger = mode;
    }

    bool throw_on_copy() const noexcept {
        return trigger;
    }

private:
    bool trigger{};
};

inline bool operator==(const throwing_type &lhs, const throwing_type &rhs) {
    return lhs.throw_on_copy() == rhs.throw_on_copy();
}

} // namespace test

#endif
