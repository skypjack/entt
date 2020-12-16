#ifndef ENTT_ENTITY_VIEW_PACK_HPP
#define ENTT_ENTITY_VIEW_PACK_HPP


#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../core/type_traits.hpp"
#include "fwd.hpp"
#include "utility.hpp"


namespace entt {


/**
 * @brief View pack.
 *
 * The view pack allows users to combine multiple views into a single iterable
 * object, while also giving them full control over which view should lead the
 * iteration.<br/>
 * This class returns all and only the entities present in all views. Its
 * intended primary use is for custom storage and views, but it can also be very
 * convenient in everyday use.
 *
 * @tparam Head Type of the leading view of the pack.
 * @tparam Tail Types of all other views of the pack.
 */
template<typename Head, typename... Tail>
class view_pack {
    template<typename It>
    class view_pack_iterator final {
        friend class view_pack<Head, Tail...>;

        view_pack_iterator(It from, It to, const std::tuple<Tail...> &other) ENTT_NOEXCEPT
            : it{from},
              last{to},
              tail{other}
        {
            if(it != last && !valid()) {
                ++(*this);
            }
        }

        [[nodiscard]] bool valid() const {
            return std::apply([entity = *it](auto &&... curr) { return (curr.contains(entity) && ...); }, tail);
        }

    public:
        using difference_type = typename std::iterator_traits<It>::difference_type;
        using value_type = typename std::iterator_traits<It>::value_type;
        using pointer = typename std::iterator_traits<It>::pointer;
        using reference = typename std::iterator_traits<It>::reference;
        using iterator_category = std::input_iterator_tag;

        view_pack_iterator & operator++() ENTT_NOEXCEPT {
            while(++it != last && !valid());
            return *this;
        }

        view_pack_iterator operator++(int) ENTT_NOEXCEPT {
            view_pack_iterator orig = *this;
            return ++(*this), orig;
        }

        [[nodiscard]] reference operator*() const {
            return *it;
        }

        [[nodiscard]] bool operator==(const view_pack_iterator &other) const ENTT_NOEXCEPT {
            return other.it == it;
        }

        [[nodiscard]] bool operator!=(const view_pack_iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

    private:
        It it;
        const It last;
        const std::tuple<Tail...> tail;
    };

    class iterable_view_pack final {
        friend class view_pack<Head, Tail...>;

        using iterable_view = decltype(std::declval<Head>().each());

        template<typename It>
        class iterable_view_pack_iterator final {
            friend class iterable_view_pack;

            iterable_view_pack_iterator(It from, It to, const std::tuple<Tail...> &other) ENTT_NOEXCEPT
                : it{from},
                  last{to},
                  tail{other}
            {
                if(it != last && !valid()) {
                    ++(*this);
                }
            }

            [[nodiscard]] bool valid() const {
                return std::apply([entity = std::get<0>(*it)](auto &&... curr) { return (curr.contains(entity) && ...); }, tail);
            }

        public:
            using difference_type = typename std::iterator_traits<It>::difference_type;
            using value_type = decltype(std::tuple_cat(*std::declval<It>(), std::declval<Tail>().get({})...));
            using pointer = void;
            using reference = value_type;
            using iterator_category = std::input_iterator_tag;

            iterable_view_pack_iterator & operator++() ENTT_NOEXCEPT {
                while(++it != last && !valid());
                return *this;
            }

            iterable_view_pack_iterator operator++(int) ENTT_NOEXCEPT {
                iterable_view_pack_iterator orig = *this;
                return ++(*this), orig;
            }

            [[nodiscard]] reference operator*() const {
                return std::apply([value = *it](auto &&... curr) { return std::tuple_cat(value, curr.get(std::get<0>(value))...); }, tail);
            }

            [[nodiscard]] bool operator==(const iterable_view_pack_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const iterable_view_pack_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            It it;
            const It last;
            const std::tuple<Tail...> tail;
        };

        iterable_view_pack(const Head &first, const std::tuple<Tail...> &last)
            : iterable{first.each()},
              tail{last}
        {}

    public:
        using iterator = iterable_view_pack_iterator<typename iterable_view::iterator>;
        using reverse_iterator = iterable_view_pack_iterator<typename iterable_view::reverse_iterator>;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return { iterable.begin(), iterable.end(), tail };
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return { iterable.end(), iterable.end(), tail };
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return { iterable.rbegin(), iterable.rend(), tail };
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return { iterable.rend(), iterable.rend(), tail };
        }

    private:
        iterable_view iterable;
        std::tuple<Tail...> tail;
    };

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = std::common_type_t<typename Head::entity_type, typename Tail::entity_type...>;
    /*! @brief Underlying entity identifier. */
    using size_type = std::common_type_t<typename Head::size_type, typename Tail::size_type...>;
    /*! @brief Input iterator type. */
    using iterator = view_pack_iterator<typename Head::iterator>;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = view_pack_iterator<typename Head::reverse_iterator>;

    /**
     * @brief Constructs a pack from a bunch of views.
     * @param first A reference to the leading view for the pack.
     * @param last References to the other views to use to construct the pack.
     */
    view_pack(const Head &first, const Tail &... last)
        : head{first},
          tail{last...}
    {}

    /**
     * @brief Returns an iterator to the first entity of the pack.
     *
     * The returned iterator points to the first entity of the pack. If the pack
     * is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the pack.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return { head.begin(), head.end(), tail };
    }

    /**
     * @brief Returns an iterator that is past the last entity of the pack.
     *
     * The returned iterator points to the entity following the last entity of
     * the pack. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the pack.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return { head.end(), head.end(), tail };
    }

    /**
     * @brief Returns an iterator to the first entity of the pack.
     *
     * The returned iterator points to the first entity of the reversed pack. If
     * the pack is empty, the returned iterator will be equal to `rend()`.
     *
     * @return An iterator to the first entity of the pack.
     */
    [[nodiscard]] reverse_iterator rbegin() const {
        return { head.rbegin(), head.rend(), tail };
    }

    /**
     * @brief Returns an iterator that is past the last entity of the reversed
     * pack.
     *
     * The returned iterator points to the entity following the last entity of
     * the reversed pack. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * reversed pack.
     */
    [[nodiscard]] reverse_iterator rend() const {
        return { head.rend(), head.rend(), tail };
    }

    /**
     * @brief Returns the first entity of the pack, if any.
     * @return The first entity of the pack if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity of the pack, if any.
     * @return The last entity of the pack if one exists, the null entity
     * otherwise.
     */
    [[nodiscard]] entity_type back() const {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    [[nodiscard]] iterator find(const entity_type entt) const {
        iterator it{head.find(entt), head.end(), tail};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a pack contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the pack contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return head.contains(entt) && std::apply([entt](auto &&... curr) { return (curr.contains(entt) && ...); }, tail);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the pack
     * results in undefined behavior.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt));
        auto component = std::apply([this, entt](auto &&... curr) { return std::tuple_cat(head.get(entt), curr.get(entt)...); }, tail);

        if constexpr(sizeof...(Comp) == 0) {
            return component;
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<Comp &>(component), ...);
        } else {
            return std::forward_as_tuple(std::get<Comp &>(component)...);
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        for(auto &&value: head.each()) {
            if(std::apply([&value](auto &&... curr) { return (curr.contains(std::get<0>(value)) && ...); }, tail)) {
                auto args = std::apply([&value](auto &&... curr) { return std::tuple_cat(value, curr.get(std::get<0>(value))...); }, tail);

                if constexpr(is_applicable_v<Func, decltype(args)>) {
                    std::apply(func, args);
                } else {
                    std::apply([&func](const auto, auto &&... component) { func(std::forward<decltype(component)>(component)...); }, args);
                }
            }
        }
    }

    /**
     * @brief Returns an iterable object to use to _visit_ the pack.
     *
     * The iterable object returns tuples that contain the current entity and a
     * set of references to its non-empty components. The _constness_ of the
     * components is as requested.
     *
     * @note
     * Empty types aren't explicitly instantiated and therefore they are never
     * returned during iterations.
     *
     * @return An iterable object to use to _visit_ the pack.
     */
    [[nodiscard]] iterable_view_pack each() const ENTT_NOEXCEPT {
        return { head, tail };
    }

    /**
     * @brief Returns a copy of the views stored by the pack.
     * @return A copy of the views stored by the pack.
     */
    std::tuple<Head, Tail...> pack() const ENTT_NOEXCEPT {
        return std::apply([this](auto &&... curr) { return std::make_tuple(head, curr...); }, tail);
    }

    /**
     * @brief Appends a view to a pack.
     * @tparam Args View template arguments.
     * @param other A reference to a view to append to the pack.
     * @return The extended pack.
     */
    template<typename... Args>
    [[nodiscard]] auto operator|(const basic_view<Args...> &other) const {
        return std::make_from_tuple<view_pack<Head, Tail..., basic_view<Args...>>>(std::tuple_cat(std::make_tuple(head), tail, std::make_tuple(other)));
    }

    /**
     * @brief Appends a pack and therefore all its views to another pack.
     * @tparam Pack Types of views of the pack to append.
     * @param other A reference to the pack to append.
     * @return The extended pack.
     */
    template<typename... Pack>
    [[nodiscard]] auto operator|(const view_pack<Pack...> &other) const {
        return std::make_from_tuple<view_pack<Head, Tail..., Pack...>>(std::tuple_cat(std::make_tuple(head), tail, other.pack()));
    }

private:
    Head head;
    std::tuple<Tail...> tail;
};


/**
 * @brief Combines two views in a pack.
 * @tparam Args Template arguments of the first view.
 * @tparam Other Template arguments of the second view.
 * @param lhs A reference to the first view with which to create the pack.
 * @param rhs A reference to the second view with which to create the pack.
 * @return A pack that combines the two views in a single iterable object.
 */
template<typename... Args, typename... Other>
[[nodiscard]] auto operator|(const basic_view<Args...> &lhs, const basic_view<Other...> &rhs) {
    return view_pack{lhs, rhs};
}


/**
 * @brief Combines a view with a pack.
 * @tparam Args View template arguments.
 * @tparam Pack Types of views of the pack.
 * @param view A reference to the view to combine with the pack.
 * @param pack A reference to the pack to combine with the view.
 * @return The extended pack.
 */
template<typename... Args, typename... Pack>
[[nodiscard]] auto operator|(const basic_view<Args...> &view, const view_pack<Pack...> &pack) {
    return view_pack{view} | pack;
}


}


#endif
