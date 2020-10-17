#ifndef ENTT_ENTITY_VIEW_PACK_HPP
#define ENTT_ENTITY_VIEW_PACK_HPP


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
 * @tparam View Type of the leading view of the pack.
 * @tparam Other Types of all other views of the pack.
 */
template<typename View, typename... Other>
class view_pack {
    using common_entity_type = std::common_type_t<typename View::entity_type, typename Other::entity_type...>;

    class view_pack_iterator {
        friend class view_pack<View, Other...>;

        using underlying_iterator = typename View::iterator;

        view_pack_iterator(underlying_iterator from, underlying_iterator to, const std::tuple<View, Other...> &ref) ENTT_NOEXCEPT
            : it{from},
              last{to},
              pack{std::get<Other>(ref)...}
        {
            if(it != last && !valid()) {
                ++(*this);
            }
        }

        [[nodiscard]] bool valid() const {
            const auto entity = *it;
            return (std::get<Other>(pack).contains(entity) && ...);
        }

    public:
        using difference_type = typename std::iterator_traits<underlying_iterator>::difference_type;
        using value_type = decltype(*std::declval<underlying_iterator>());
        using pointer = void;
        using reference = value_type;
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
        underlying_iterator it;
        const underlying_iterator last;
        const std::tuple<Other...> pack;
    };

    class iterable_view_pack {
        friend class view_pack<View, Other...>;

        using iterable_view = decltype(std::declval<View>().each());

        class iterable_view_pack_iterator {
            friend class iterable_view_pack;

            using underlying_iterator = typename iterable_view::iterator;

            iterable_view_pack_iterator(underlying_iterator from, underlying_iterator to, const std::tuple<Other...> &ref) ENTT_NOEXCEPT
                : it{from},
                  last{to},
                  pack{ref}
            {
                if(it != last && !valid()) {
                    ++(*this);
                }
            }

            [[nodiscard]] bool valid() const {
                const auto entity = std::get<0>(*it);
                return (std::get<Other>(pack).contains(entity) && ...);
            }

        public:
            using difference_type = typename std::iterator_traits<underlying_iterator>::difference_type;
            using value_type = decltype(std::tuple_cat(*std::declval<underlying_iterator>(), std::declval<Other>().get({})...));
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
                const auto curr = *it;
                return std::tuple_cat(curr, std::get<Other>(pack).get(std::get<0>(curr))...);
            }

            [[nodiscard]] bool operator==(const iterable_view_pack_iterator &other) const ENTT_NOEXCEPT {
                return other.it == it;
            }

            [[nodiscard]] bool operator!=(const iterable_view_pack_iterator &other) const ENTT_NOEXCEPT {
                return !(*this == other);
            }

        private:
            underlying_iterator it;
            const underlying_iterator last;
            const std::tuple<Other...> pack;
        };

        iterable_view_pack(const std::tuple<View, Other...> &ref)
            : iterable{std::get<View>(ref).each()},
              pack{std::get<Other>(ref)...}
        {}

    public:
        using iterator = iterable_view_pack_iterator;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return { iterable.begin(), iterable.end(), pack };
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return { iterable.end(), iterable.end(), pack };
        }

    private:
        iterable_view iterable;
        std::tuple<Other...> pack;
    };

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = common_entity_type;
    /*! @brief Input iterator type. */
    using iterator = view_pack_iterator;

    /**
     * @brief Constructs a pack from a bunch of views.
     * @param view A reference to the leading view for the pack.
     * @param other References to the other views to use to construct the pack.
     */
    view_pack(const View &view, const Other &... other)
        : pack{view, other...}
    {}

    /**
     * @brief Returns an iterator to the first entity of the pack.
     *
     * The returned iterator points to the first entity of the pack. If the pack
     * is empty, the returned iterator will be equal to `end()`.
     *
     * @note
     * Iterators stay true to the order imposed by the first view of the pack.
     *
     * @return An iterator to the first entity of the pack.
     */
    [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
        return { std::get<View>(pack).begin(), std::get<View>(pack).end(), pack };
    }

    /**
     * @brief Returns an iterator that is past the last entity of the pack.
     *
     * The returned iterator points to the entity following the last entity of
     * the pack. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @note
     * Iterators stay true to the order imposed by the first view of the pack.
     *
     * @return An iterator to the entity following the last entity of the pack.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return { std::get<View>(pack).end(), std::get<View>(pack).end(), pack };
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
        for(const auto value: std::get<View>(pack).each()) {
            const auto entity = std::get<0>(value);

            if((std::get<Other>(pack).contains(entity) && ...)) {
                if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(value, std::get<Other>(pack).get(entity)...))>) {
                    std::apply(func, std::tuple_cat(value, std::get<Other>(pack).get(entity)...));
                } else {
                    const auto args = std::apply([](const auto entity, auto &&... component) { return std::forward_as_tuple(component...); }, value);
                    std::apply(func, std::tuple_cat(args, std::get<Other>(pack).get(entity)...));
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
        return pack;
    }

    /**
     * @brief Returns a copy of the requested view from a pack.
     * @tparam Type Type of the view to return.
     * @return A copy of the requested view from the pack.
     */
    template<typename Type>
    operator Type() const ENTT_NOEXCEPT {
        return std::get<Type>(pack);
    }

    /**
     * @brief Appends a view to a pack.
     * @tparam Args View template arguments.
     * @param view A reference to a view to append to the pack.
     * @return The extended pack.
     */
    template<typename... Args>
    [[nodiscard]] auto operator|(const basic_view<Args...> &view) const {
        return std::make_from_tuple<view_pack<View, Other..., basic_view<Args...>>>(std::tuple_cat(pack, std::make_tuple(view)));
    }

    /**
     * @brief Appends a pack and therefore all its views to another pack.
     * @tparam Pack Types of views of the pack to append.
     * @param other A reference to the pack to append.
     * @return The extended pack.
     */
    template<typename... Pack>
    [[nodiscard]] auto operator|(const view_pack<Pack...> &other) const {
        return std::make_from_tuple<view_pack<View, Other..., Pack...>>(std::tuple_cat(pack, std::make_tuple(static_cast<Pack>(other)...)));
    }

private:
    std::tuple<View, Other...> pack;
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
