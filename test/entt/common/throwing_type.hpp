#ifndef ENTT_COMMON_THROWING_TYPE_HPP
#define ENTT_COMMON_THROWING_TYPE_HPP

namespace test {

class throwing_type {
    struct test_exception {};

public:
    using exception_type = test_exception;
    static constexpr auto moved_from_value = -1;

    throwing_type(int value)
        : data{value} {}

    throwing_type(const throwing_type &other)
        : data{other.data} {
        if(data == trigger_on_value) {
            data = moved_from_value;
            throw exception_type{};
        }
    }

    throwing_type &operator=(const throwing_type &other) {
        if(other.data == trigger_on_value) {
            data = moved_from_value;
            throw exception_type{};
        }

        data = other.data;
        return *this;
    }

    operator int() const {
        return data;
    }

    static inline int trigger_on_value{};

private:
    int data{};
};

} // namespace test

#endif
