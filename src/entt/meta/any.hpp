#ifndef ENTT_META_ANY_HPP
#define ENTT_META_ANY_HPP


#include <memory>
#include <utility>
#include <type_traits>
#include "info.hpp"


namespace entt {


namespace internal {


struct Holder {
    virtual ~Holder() = default;

    virtual MetaTypeNode * meta() const ENTT_NOEXCEPT = 0;
    virtual const void * data() const ENTT_NOEXCEPT = 0;
    virtual bool operator==(const Holder &) const ENTT_NOEXCEPT = 0;

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const Holder *>(this)->data());
    }
};


template<typename Type>
struct HolderType: public Holder {
    template<typename Object>
    static auto compare(int, const Object &lhs, const Object &rhs)
    -> decltype(lhs == rhs, bool{})
    {
        return lhs == rhs;
    }

    template<typename Object>
    static bool compare(char, const Object &, const Object &) {
        return false;
    }

public:
    template<typename... Args>
    HolderType(Args &&... args)
        : storage{}
    {
        new (&storage) Type{std::forward<Args>(args)...};
    }

    ~HolderType() {
        reinterpret_cast<Type *>(&storage)->~Type();
    }

    MetaTypeNode * meta() const ENTT_NOEXCEPT override {
        return internal::MetaInfo<Type>::type;
    }

    inline const void * data() const ENTT_NOEXCEPT override {
        return &storage;
    }

    bool operator==(const Holder &other) const ENTT_NOEXCEPT override {
        return meta() == other.meta() && compare(0, *reinterpret_cast<const Type *>(&storage), *static_cast<const Type *>(other.data()));
    }

private:
    typename std::aligned_storage_t<sizeof(Type), alignof(Type)> storage;
};


}


struct MetaAny final {
    MetaAny() ENTT_NOEXCEPT = default;

    template<typename Type>
    MetaAny(Type &&type)
        : actual{std::make_unique<internal::HolderType<std::decay_t<Type>>>(std::forward<Type>(type))}
    {}

    MetaAny(const MetaAny &) = delete;
    MetaAny(MetaAny &&) = default;

    MetaAny & operator=(const MetaAny &other) = delete;
    MetaAny & operator=(MetaAny &&) = default;

    inline bool valid() const ENTT_NOEXCEPT {
        return *this;
    }

    MetaTypeNode * meta() const ENTT_NOEXCEPT {
        return actual ? actual->meta() : nullptr;
    }

    template<typename Type>
    inline const Type & get() const ENTT_NOEXCEPT {
        return *static_cast<const Type *>(actual->data());
    }

    template<typename Type>
    inline Type & get() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const MetaAny *>(this)->get<Type>());
    }

    template<typename Type>
    inline const Type * data() const ENTT_NOEXCEPT {
        return actual ? static_cast<const Type *>(actual->data()) : nullptr;
    }

    template<typename Type>
    inline Type * data() ENTT_NOEXCEPT {
        return const_cast<Type *>(const_cast<const MetaAny *>(this)->data<Type>());
    }

    inline const void * data() const ENTT_NOEXCEPT {
        return *this;
    }

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const MetaAny *>(this)->data());
    }

    inline operator const void *() const ENTT_NOEXCEPT {
        return actual ? actual->data() : nullptr;
    }

    inline operator void *() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const MetaAny *>(this)->data());
    }

    inline explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(actual);
    }

    inline bool operator==(const MetaAny &other) const ENTT_NOEXCEPT {
        return actual == other.actual || (actual && other.actual && *actual == *other.actual);
    }

private:
    std::unique_ptr<internal::Holder> actual;
};


}


#endif // ENTT_META_ANY_HPP
