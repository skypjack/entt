#ifndef ENTT_COMPONENT_POOL_HPP
#define ENTT_COMPONENT_POOL_HPP


#include <algorithm>
#include <utility>
#include <numeric>
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

    ~SparseSet() noexcept {
        assert(empty());
    }

    SparseSet & operator=(const SparseSet &) = delete;
    SparseSet & operator=(SparseSet &&) = default;

    size_type size() const noexcept {
        return direct.size();
    }

    size_t capacity() const noexcept {
        return direct.capacity();
    }

    bool empty() const noexcept {
        return direct.empty();
    }

    const index_type * data() const noexcept {
        return direct.data();
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

    void swap(index_type lhs, index_type rhs) {
        assert(valid(lhs));
        assert(valid(rhs));

        std::swap(direct[reverse[lhs]], direct[reverse[rhs]]);
        std::swap(reverse[lhs], reverse[rhs]);
    }

    void reset() {
        reverse.clear();
        direct.clear();
    }

private:
    std::vector<pos_type> reverse;
    std::vector<index_type> direct;
};


template<typename Index, typename Type>
class SparseSet<Index, Type> final: public SparseSet<Index> {
    template<typename Compare>
    void arrange(Compare compare) {
        const auto *data = SparseSet<Index>::data();
        const auto size = SparseSet<Index>::size();
        std::vector<pos_type> copy(size);

        std::iota(copy.begin(), copy.end(), pos_type{});
        std::sort(copy.begin(), copy.end(), compare);

        for(pos_type i = 0; i < copy.size(); ++i) {
            const auto target = i;
            auto curr = i;

            while(copy[curr] != target) {
                SparseSet<Index>::swap(*(data + copy[curr]), *(data + curr));
                std::swap(instances[copy[curr]], instances[curr]);
                std::swap(copy[curr], curr);
            }

            copy[curr] = curr;
        }
    }

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
    type & construct(index_type idx, Args&&... args) {
        SparseSet<Index>::construct(idx);
        instances.push_back({ std::forward<Args>(args)... });
        return instances.back();
    }

    void destroy(index_type idx) {
        auto pos = SparseSet<Index>::destroy(idx);
        instances[pos] = std::move(instances[SparseSet<Index>::size()]);
        instances.pop_back();
    }

    void swap(index_type lhs, index_type rhs) {
        std::swap(instances[SparseSet<Index>::get(lhs)], instances[SparseSet<Index>::get(rhs)]);
    }

    template<typename Compare>
    void sort(Compare compare) {
        arrange([this, compare = std::move(compare)](auto lhs, auto rhs) {
            return !compare(instances[lhs], instances[rhs]);
        });
    }

    template<typename Idx>
    void respect(const SparseSet<Idx> &other) {
        const auto *data = SparseSet<Index>::data();

        arrange([data, &other](auto lhs, auto rhs) {
            auto eLhs = *(data + lhs);
            auto eRhs = *(data + rhs);

            bool bLhs = other.has(eLhs);
            bool bRhs = other.has(eRhs);
            bool compare = false;

            if(bLhs && bRhs) {
                compare = other.get(eLhs) < other.get(eRhs);
            } else if(!bLhs && !bRhs) {
                compare = eLhs < eRhs;
            } else {
                compare = bRhs;
            }

            return compare;
        });
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
