#ifndef ENTT_COMMON_THROWING_TYPE_HPP
#define ENTT_COMMON_THROWING_TYPE_HPP

namespace test {

struct throwing_type_exception {};

struct throwing_type {
    throwing_type(bool mode)
        : trigger{mode} {}

    throwing_type(const throwing_type &other)
        : trigger{other.trigger} {
        if(trigger) {
            throw throwing_type_exception{};
        }
    }

    ~throwing_type() { /* make it non trivially destructible */ }

    throwing_type &operator=(const throwing_type &other) {
        trigger = other.trigger;
        return *this;
    }

    void throw_on_copy(const bool mode) noexcept {
        trigger = mode;
    }

    bool throw_on_copy() const noexcept {
        return trigger;
    }

    [[nodiscard]] bool operator==(const throwing_type &other) const noexcept {
        return trigger == other.trigger;
    }

private:
    bool trigger{};
};

} // namespace test

#endif
