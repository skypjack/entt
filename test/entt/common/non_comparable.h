#ifndef ENTT_COMMON_NON_COMPARABLE_HPP
#define ENTT_COMMON_NON_COMPARABLE_HPP

namespace test {

struct non_comparable {
    bool operator==(const non_comparable &) const = delete;
};

} // namespace test

#endif
