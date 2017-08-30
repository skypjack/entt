#ifndef ENTT_TAG_HANDLER_HPP
#define ENTT_TAG_HANDLER_HPP


#include <type_traits>
#include <utility>
#include <cstddef>
#include <cassert>


namespace entt {


template<typename Index, typename Type>
struct TagHandler {
    using type = Type;
    using index_type = Index;
    using pos_type = index_type;
    using size_type = std::size_t;
    using iterator_type = const type *;

private:
    inline bool valid(index_type idx) const noexcept {
        return !idx && tag;
    }

public:
    explicit TagHandler() = default;

    TagHandler(const TagHandler &) = delete;
    TagHandler(TagHandler &&) = default;

    ~TagHandler() noexcept {
        assert(not tag);
    }

    TagHandler & operator=(const TagHandler &) = delete;
    TagHandler & operator=(TagHandler &&) = default;

    bool empty() const noexcept {
        return !tag;
    }

    size_type size() const noexcept {
        return tag ? size_type{1} : size_type{0};
    }

    iterator_type begin() const noexcept {
        return tag;
    }

    iterator_type end() const noexcept {
        return tag ? tag + 1 : tag;
    }

    bool has(index_type idx) const noexcept {
        return valid(idx);
    }

    const type & get(index_type idx) const noexcept {
        assert(valid(idx));
        return *tag;
    }

    type & get(index_type idx) noexcept {
        return const_cast<type &>(const_cast<const TagHandler *>(this)->get(idx));
    }

    template<typename... Args>
    type & construct(index_type idx, Args... args) {
        assert(!valid(idx));
        tag = new(&chunk) Type{std::forward<Args>(args)...};
        return *tag;
    }

    void destroy(index_type idx) {
        assert(valid(idx));
        tag->~Type();
        tag = nullptr;
    }

    void reset() {
        if(tag) {
            destroy(index_type{0});
        }
    }

private:
    std::aligned_storage_t<sizeof(Type), alignof(Type)> chunk;
    Type *tag{nullptr};
};


}


#endif // ENTT_TAG_HANDLER_HPP
