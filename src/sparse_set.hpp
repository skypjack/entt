#ifndef ENTT_COMPONENT_POOL_HPP
#define ENTT_COMPONENT_POOL_HPP


#include <algorithm>
#include <utility>
#include <vector>
#include <cstddef>
#include <cassert>


namespace entt {


template<typename...>
class SparseSet;


template<typename Index>
class SparseSet<Index> {
    struct SparseSetIterator {
        using value_type = Index;

        SparseSetIterator(const std::vector<Index> *direct, std::size_t pos)
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
        const std::vector<Index> *direct;
        std::size_t pos;
    };

    inline bool valid(Index idx) const noexcept {
        return idx < reverse.size() && reverse[idx] < direct.size() && direct[reverse[idx]] == idx;
    }

public:
    using index_type = Index;
    using pos_type = index_type;
    using size_type = std::size_t;
    using iterator_type = SparseSetIterator;

    explicit SparseSet() = default;

    SparseSet(const SparseSet &) = delete;
    SparseSet(SparseSet &&) = default;

    virtual ~SparseSet() noexcept {
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

    virtual void destroy(index_type idx) {
        assert(valid(idx));
        auto pos = reverse[idx];
        reverse[direct.back()] = pos;
        direct[pos] = direct.back();
        direct.pop_back();
    }

    virtual void swap(Index lhs, Index rhs) {
        assert(valid(lhs));
        assert(valid(rhs));
        std::swap(direct[reverse[lhs]], direct[reverse[rhs]]);
        std::swap(reverse[lhs], reverse[rhs]);
    }

    template<typename Compare>
    void sort(Compare compare) {
        std::vector<pos_type> copy{direct.cbegin(), direct.cend()};
        std::sort(copy.begin(), copy.end(), [compare = std::move(compare)](auto... args) {
            return not compare(args...);
        });

        for(pos_type i = 0; i < copy.size(); ++i) {
            if(direct[i] != copy[i]) {
                swap(direct[i], copy[i]);
            }
        }
    }

    template<typename Idx>
    void respect(const SparseSet<Idx> &other) {
        struct Bool { bool value{false}; };
        std::vector<Bool> check(reverse.size());

        for(auto entity: other.direct) {
            check[entity].value = true;
        }

        sort([this, &other, &check](auto lhs, auto rhs) {
            bool bLhs = check[lhs].value;
            bool bRhs = check[rhs].value;
            bool compare = false;

            if(bLhs && bRhs) {
                compare = other.get(rhs) < other.get(lhs);
            } else if(!bLhs && !bRhs) {
                compare = rhs < lhs;
            } else {
                compare = bLhs;
            }

            return compare;
        });
    }

    virtual void reset() {
        reverse.clear();
        direct.clear();
    }

private:
    std::vector<Index> reverse;
    std::vector<Index> direct;
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
    type & construct(index_type idx, Args&&... args) {
        SparseSet<Index>::construct(idx);
        instances.push_back({ std::forward<Args>(args)... });
        return instances.back();
    }

    void destroy(index_type idx) override {
        instances[SparseSet<Index>::get(idx)] = std::move(instances.back());
        instances.pop_back();
        SparseSet<Index>::destroy(idx);
    }

    void swap(Index lhs, Index rhs) override {
        std::swap(instances[SparseSet<Index>::get(lhs)], instances[SparseSet<Index>::get(rhs)]);
        SparseSet<Index>::swap(lhs, rhs);
    }

    void reset() override {
        SparseSet<Index>::reset();
        instances.clear();
    }

private:
    std::vector<Type> instances;
};


}


#endif // ENTT_COMPONENT_POOL_HPP
