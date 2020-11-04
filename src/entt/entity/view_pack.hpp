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
 * @tparam View Type of the leading view of the pack.
 * @tparam Other Types of all other views of the pack.
 */
template<typename View, typename... Other>
class view_pack {
    template<typename It>
    class view_pack_iterator final {
        friend class view_pack<View, Other...>;

        view_pack_iterator(It from, It to, const std::tuple<View, Other...> &ref) ENTT_NOEXCEPT
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
        const std::tuple<Other...> pack;
    };

    class iterable_view_pack final {
        friend class view_pack<View, Other...>;

        using iterable_view = decltype(std::declval<View>().each());

        template<typename It>
        class iterable_view_pack_iterator final {
            friend class iterable_view_pack;

            iterable_view_pack_iterator(It from, It to, const std::tuple<Other...> &ref) ENTT_NOEXCEPT
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
            using difference_type = typename std::iterator_traits<It>::difference_type;
            using value_type = decltype(std::tuple_cat(*std::declval<It>(), std::declval<Other>().get({})...));
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
            It it;
            const It last;
            const std::tuple<Other...> pack;
        };

        iterable_view_pack(const std::tuple<View, Other...> &ref)
            : iterable{std::get<View>(ref).each()},
              pack{std::get<Other>(ref)...}
        {}

    public:
        using iterator = iterable_view_pack_iterator<typename iterable_view::iterator>;
        using reverse_iterator = iterable_view_pack_iterator<typename iterable_view::reverse_iterator>;

        [[nodiscard]] iterator begin() const ENTT_NOEXCEPT {
            return { iterable.begin(), iterable.end(), pack };
        }

        [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
            return { iterable.end(), iterable.end(), pack };
        }

        [[nodiscard]] reverse_iterator rbegin() const ENTT_NOEXCEPT {
            return { iterable.rbegin(), iterable.rend(), pack };
        }

        [[nodiscard]] reverse_iterator rend() const ENTT_NOEXCEPT {
            return { iterable.rend(), iterable.rend(), pack };
        }

    private:
        iterable_view iterable;
        std::tuple<Other...> pack;
    };

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = std::common_type_t<typename View::entity_type, typename Other::entity_type...>;
    /*! @brief Underlying entity identifier. */
    using size_type = std::common_type_t<typename View::size_type, typename Other::size_type...>;
    /*! @brief Input iterator type. */
    using iterator = view_pack_iterator<typename View::iterator>;
    /*! @brief Reversed iterator type. */
    using reverse_iterator = view_pack_iterator<typename View::reverse_iterator>;

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
     * @return An iterator to the entity following the last entity of the pack.
     */
    [[nodiscard]] iterator end() const ENTT_NOEXCEPT {
        return { std::get<View>(pack).end(), std::get<View>(pack).end(), pack };
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
        return { std::get<View>(pack).rbegin(), std::get<View>(pack).rend(), pack };
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
        return { std::get<View>(pack).rend(), std::get<View>(pack).rend(), pack };
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
        iterator it{std::get<View>(pack).find(entt), std::get<View>(pack).end(), pack};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a pack contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the pack contains the given entity, false otherwise.
     */
    [[nodiscard]] bool contains(const entity_type entt) const {
        return std::get<View>(pack).contains(entt) && (std::get<Other>(pack).contains(entt) && ...);
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
        auto component = std::tuple_cat(std::get<View>(pack).get(entt), std::get<Other>(pack).get(entt)...);

        if constexpr(sizeof...(Comp) == 0) {
            return component;
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<std::add_lvalue_reference_t<Comp>>(component), ...);
        } else {
            return std::forward_as_tuple(std::get<std::add_lvalue_reference_t<Comp>>(component)...);
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
        for(const auto value: std::get<View>(pack).each()) {
            const auto entity = std::get<0>(value);

            if((std::get<Other>(pack).contains(entity) && ...)) {
                if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(value, std::get<Other>(pack).get(entity)...))>) {
                    std::apply(func, std::tuple_cat(value, std::get<Other>(pack).get(entity)...));
                } else {
                    std::apply([&func](const auto, auto &&... component) { func(std::forward<decltype(component)>(component)...); }, std::tuple_cat(value, std::get<Other>(pack).get(entity)...));
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
