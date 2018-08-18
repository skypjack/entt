#ifndef ENTT_META_ANY_HPP
#define ENTT_META_ANY_HPP


#include <memory>
#include <cassert>
#include <utility>
#include <type_traits>
#include "../config/config.h"
#include "../core/family.hpp"


namespace entt {


namespace internal {


class AnyData {
    using any_family = Family<struct InternalAnyTypeFamily>;

public:
    using any_type = typename any_family::family_type;

    template<typename Type>
    inline static any_type type() ENTT_NOEXCEPT {
        return any_family::type<Type>();
    }

    virtual ~AnyData() = default;

    virtual any_type type() const ENTT_NOEXCEPT = 0;
    virtual const void * data() const ENTT_NOEXCEPT = 0;

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const AnyData *>(this)->data());
    }

    template<typename Type>
    inline const Type * data() const ENTT_NOEXCEPT {
        assert(type<Type>() == type());
        return static_cast<const Type *>(data());
    }

    template<typename Type>
    inline Type * data() ENTT_NOEXCEPT {
        return const_cast<Type *>(const_cast<const AnyData *>(this)->data<Type>());
    }

    virtual bool operator==(const AnyData &) const ENTT_NOEXCEPT = 0;
};


inline bool operator!=(const AnyData &lhs, const AnyData &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


template<typename Type>
class AnyType: public AnyData {
    template<typename, typename = void>
    struct Comparable {
        static bool compare(const AnyData &, const AnyData &) {
            return false;
        }
    };

    template<typename T>
    struct Comparable<T, decltype(std::declval<T>().operator==(std::declval<T>()), void())> {
        static bool compare(const AnyData &lhs, const AnyData &rhs) {
            return lhs.type() == rhs.type()
                    ? *lhs.data<Type>() == *rhs.data<Type>()
                    : lhs.data() == rhs.data();
        }
    };

public:
    template<typename... Args>
    AnyType(Args &&... args)
        : storage{}
    {
        new (&storage) Type{std::forward<Args>(args)...};
    }

    ~AnyType() {
        reinterpret_cast<Type *>(&storage)->~Type();
    }

    inline any_type type() const ENTT_NOEXCEPT override {
        return AnyData::type<Type>();
    }

    inline const void * data() const ENTT_NOEXCEPT override {
        return &storage;
    }

    bool operator==(const AnyData &other) const ENTT_NOEXCEPT override {
        return Comparable<Type>::compare(*this, other);
    }

private:
    typename std::aligned_storage_t<sizeof(Type), alignof(Type)> storage;
};


}


struct Any {
    using any_type = typename internal::AnyData::any_type;

    template<typename Type>
    static any_type type() ENTT_NOEXCEPT {
        return internal::AnyData::type<std::decay_t<Type>>();
    }

    Any() ENTT_NOEXCEPT = default;

    template<typename Type>
    Any(Type &&type)
        : actual{std::make_unique<internal::AnyType<std::decay_t<Type>>>(std::forward<Type>(type))}
    {}

    Any(const Any &) = delete;
    Any(Any &&) = default;

    Any & operator=(const Any &other) = delete;
    Any & operator=(Any &&) = default;

    inline any_type type() const ENTT_NOEXCEPT {
        return actual->type();
    }

    inline bool valid() const ENTT_NOEXCEPT {
        return *this;
    }

    template<typename Type, typename... Args>
    inline void value(Args &&... args) {
        actual = std::make_unique<internal::AnyType<Type>>(std::forward<Args>(args)...);
    }

    template<typename Type>
    inline const Type & value() const ENTT_NOEXCEPT {
        return *actual->data<Type>();
    }

    template<typename Type>
    inline Type & value() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const Any *>(this)->value<Type>());
    }

    template<typename Type>
    inline const Type * data() const ENTT_NOEXCEPT {
        return actual ? actual->data<Type>() : nullptr;
    }

    template<typename Type>
    inline Type * data() ENTT_NOEXCEPT {
        return const_cast<Type *>(const_cast<const Any *>(this)->data<Type>());
    }

    inline const void * data() const ENTT_NOEXCEPT {
        return *this;
    }

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const Any *>(this)->data());
    }

    inline operator const void *() const ENTT_NOEXCEPT {
        return actual ? actual->data() : nullptr;
    }

    inline operator void *() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const Any *>(this)->data());
    }

    inline explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(actual);
    }

    inline bool operator==(const Any &other) const ENTT_NOEXCEPT {
        return actual == other.actual || (actual && other.actual && *actual == *other.actual);
    }

private:
    // TODO we can introduce small object optimization here
    std::unique_ptr<internal::AnyData> actual;
};


inline bool operator!=(const Any &lhs, const Any &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


#endif // ENTT_META_ANY_HPP
