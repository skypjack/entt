#ifndef ENTT_COMMON_NON_COMPARABLE_H
#define ENTT_COMMON_NON_COMPARABLE_H

namespace test {

struct non_comparable {
    bool operator==(const non_comparable &) const = delete;
};

} // namespace test

#endif
