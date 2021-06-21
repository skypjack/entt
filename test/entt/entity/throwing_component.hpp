#ifndef ENTT_ENTITY_THROWING_COMPONENT_HPP
#define ENTT_ENTITY_THROWING_COMPONENT_HPP


namespace test {


class throwing_component {
    struct test_exception {};

public:
    using exception_type = test_exception;
    static constexpr auto moved_from_value = -1;

    throwing_component(int value)
        : data{value}
    {}

    throwing_component(const throwing_component &other)
        : data{other.data}
    {
        if(data == trigger_on_value) {
            data = moved_from_value;
            throw exception_type{};
        }
    }

    throwing_component & operator=(const throwing_component &other) {
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


}


#endif
