#ifndef ENTT_ENTITY_THROWING_ENTITY_HPP
#define ENTT_ENTITY_THROWING_ENTITY_HPP


namespace test {


class throwing_entity {
    struct test_exception {};

public:
    using entity_type = std::uint32_t;
    using exception_type = test_exception;

    static constexpr entity_type null = entt::null;

    throwing_entity(entity_type value)
        : entt{value}
    {}

    throwing_entity(const throwing_entity &other)
        : entt{other.entt}
    {
        if(entt == trigger_on_entity) {
            throw exception_type{};
        }
    }

    throwing_entity & operator=(const throwing_entity &other) {
        if(other.entt == trigger_on_entity) {
            throw exception_type{};
        }

        entt = other.entt;
        return *this;
    }

    operator entity_type() const {
        return entt;
    }

    static inline entity_type trigger_on_entity{null};

private:
    entity_type entt{};
};


}


#endif
