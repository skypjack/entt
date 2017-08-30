#ifndef ENTT_COMPONENT_POOL_HPP
#define ENTT_COMPONENT_POOL_HPP


#include <utility>
#include <vector>
#include <cstddef>
#include <cassert>


namespace entt {


template<typename...>
class SparseSet;


template<typename Index>
class SparseSet<Index> {
    struct SparseSetIterator;

public:
    using index_type = Index;
    using pos_type = index_type;
    using size_type = std::size_t;
    using iterator_type = SparseSetIterator;

private:
    struct SparseSetIterator {
        using value_type = index_type;

        SparseSetIterator(const std::vector<index_type> *direct, size_type pos)
            : direct{direct}, pos{pos}
        {}

        SparseSetIterator & operator++() noexcept {
            return --pos, *this;
        }

        SparseSetIterator operator++(int) noexcept {
            SparseSetIterator orig = *this;
            return ++(*this), orig;
        }

        bool operator==(const SparseSetIterator &other) const noexcept {
            return other.pos == pos && other.direct == direct;
        }

        bool operator!=(const SparseSetIterator &other) const noexcept {
            return !(*this == other);
        }

        value_type operator*() const noexcept {
            return (*direct)[pos-1];
        }

    private:
        const std::vector<index_type> *direct;
        size_type pos;
    };

    inline bool valid(Index idx) const noexcept {
        return idx < reverse.size() && reverse[idx] < direct.size() && direct[reverse[idx]] == idx;
    }

public:
    explicit SparseSet() = default;

    SparseSet(const SparseSet &) = delete;
    SparseSet(SparseSet &&) = default;

    virtual ~SparseSet() noexcept {
        assert(empty());
    }

    SparseSet & operator=(const SparseSet &) = delete;
    SparseSet & operator=(SparseSet &&) = default;

    bool empty() const noexcept {
        return direct.empty();
    }

    const index_type * data() const noexcept {
        return direct.data();
    }

    size_type size() const noexcept {
        return direct.size();
    }

    iterator_type begin() const noexcept {
        return SparseSetIterator{&direct, direct.size()};
    }

    iterator_type end() const noexcept {
        return SparseSetIterator{&direct, 0};
    }

    bool has(index_type idx) const noexcept {
        return valid(idx);
    }

    pos_type get(index_type idx) const noexcept {
        assert(valid(idx));
        return reverse[idx];
    }

    pos_type construct(index_type idx) {
        assert(!valid(idx));

        if(!(idx < reverse.size())) {
            reverse.resize(idx+1);
        }

        auto pos = pos_type(direct.size());
        reverse[idx] = pos;
        direct.emplace_back(idx);

        return pos;
    }

    pos_type destroy(index_type idx) {
        assert(valid(idx));

        auto last = direct.size() - 1;
        auto pos = reverse[idx];

        reverse[direct[last]] = pos;
        direct[pos] = direct[last];
        direct.pop_back();

        return pos;
    }

    void reset() {
        reverse.resize(0);
        direct.clear();
    }

private:
    std::vector<pos_type> reverse;
    std::vector<index_type> direct;
};


template<typename Index, typename Type>
class SparseSet<Index, Type> final: public SparseSet<Index> {
public:
    using type = Type;
    using index_type = typename SparseSet<Index>::index_type;
    using pos_type = typename SparseSet<Index>::pos_type;
    using size_type = typename SparseSet<Index>::size_type;
    using iterator_type = typename SparseSet<Index>::iterator_type;

    explicit SparseSet() = default;

    SparseSet(const SparseSet &) = delete;
    SparseSet(SparseSet &&) = default;

    SparseSet & operator=(const SparseSet &) = delete;
    SparseSet & operator=(SparseSet &&) = default;

    type * raw() noexcept {
        return instances.data();
    }

    const type * raw() const noexcept {
        return instances.data();
    }

    const type & get(index_type idx) const noexcept {
        return instances[SparseSet<Index>::get(idx)];
    }

    type & get(index_type idx) noexcept {
        return const_cast<type &>(const_cast<const SparseSet *>(this)->get(idx));
    }

    template<typename... Args>
    type & construct(index_type idx, Args... args) {
        SparseSet<Index>::construct(idx);
        instances.push_back({ args... });
        return instances.back();
    }

    void destroy(index_type idx) {
        auto pos = SparseSet<Index>::destroy(idx);
        instances[pos] = std::move(instances[SparseSet<Index>::size()]);
        instances.pop_back();
    }

    void reset() {
        SparseSet<Index>::reset();
        instances.clear();
    }

private:
    std::vector<type> instances;
};


}


#endif // ENTT_COMPONENT_POOL_HPP
