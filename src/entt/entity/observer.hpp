#ifndef ENTT_ENTITY_OBSERVER_HPP
#define ENTT_ENTITY_OBSERVER_HPP


#include <limits>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "registry.hpp"
#include "storage.hpp"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


/*! @brief Grouping matcher. */
template<typename...>
struct matcher {};


/**
 * @brief Collector.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
struct basic_collector;


/**
 * @brief Collector.
 *
 * A collector contains a set of rules (literally, matchers) to use to track
 * entities.<br/>
 * Its main purpose is to generate a descriptor that allows an observer to know
 * how to connect to a registry.
 */
template<>
struct basic_collector<> {
    /**
     * @brief Adds a grouping matcher to the collector.
     * @tparam AllOf Types of components tracked by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto group(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        return basic_collector<matcher<matcher<type_list<>, type_list<>>, type_list<NoneOf...>, type_list<AllOf...>>>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of component for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto replace() ENTT_NOEXCEPT {
        return basic_collector<matcher<matcher<type_list<>, type_list<>>, AnyOf>>{};
    }
};

/**
 * @brief Collector.
 * @copydetails basic_collector<>
 * @tparam AnyOf Types of components for which changes should be detected.
 * @tparam Matcher Types of grouping matchers.
 */
template<typename... Reject, typename... Require, typename... Rule, typename... Other>
struct basic_collector<matcher<matcher<type_list<Reject...>, type_list<Require...>>, Rule...>, Other...> {
    /**
     * @brief Adds a grouping matcher to the collector.
     * @tparam AllOf Types of components tracked by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto group(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        using first = matcher<matcher<type_list<Reject...>, type_list<Require...>>, Rule...>;
        return basic_collector<matcher<matcher<type_list<>, type_list<>>, type_list<NoneOf...>, type_list<AllOf...>>, first, Other...>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of component for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto replace() ENTT_NOEXCEPT {
        using first = matcher<matcher<type_list<Reject...>, type_list<Require...>>, Rule...>;
        return basic_collector<matcher<matcher<type_list<>, type_list<>>, AnyOf>, first, Other...>{};
    }

    /**
     * @brief Updates the filter of the last added matcher.
     * @tparam AllOf Types of components required by the matcher.
     * @tparam NoneOf Types of components used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto where(exclude_t<NoneOf...> = {}) ENTT_NOEXCEPT {
        return basic_collector<matcher<matcher<type_list<Reject..., NoneOf...>, type_list<Require..., AllOf...>>, Rule...>, Other...>{};
    }
};


/*! @brief Variable template used to ease the definition of collectors. */
constexpr basic_collector<> collector{};


/**
 * @brief Observer.
 *
 * An observer returns all the entities and only the entities that fit the
 * requirements of at least one matcher. Moreover, it's guaranteed that the
 * entity list is tightly packed in memory for fast iterations.<br/>
 * In general, observers don't stay true to the order of any set of components.
 *
 * Observers work mainly with two types of matchers, provided through a
 * collector:
 *
 * * Observing matcher: an observer will return at least all the living entities
 *   for which one or more of the given components have been explicitly
 *   replaced and not yet destroyed.
 * * Grouping matcher: an observer will return at least all the living entities
 *   that would have entered the given group if it existed and that would have
 *   not yet left it.
 *
 * If an entity respects the requirements of multiple matchers, it will be
 * returned once and only once by the observer in any case.
 *
 * Matchers support also filtering by means of a _where_ clause that accepts
 * both a list of types and an exclusion list.<br/>
 * Whenever a matcher finds that an entity matches its requirements, the
 * condition of the filter is verified before to register the entity itself.
 * Moreover, a registered entity isn't returned by the observer if the condition
 * set by the filter is broken in the meantime.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all the other cases, modifying the pools of the given components in any
 * way invalidates all the iterators and using them results in undefined
 * behavior.
 *
 * @warning
 * Lifetime of an observer doesn't necessarily have to overcome the one of the
 * registry to which it is connected. However, the observer must be disconnected
 * from the registry before being destroyed to avoid crashes due to dangling
 * pointers.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 */
template<typename Entity>
class basic_observer {
    using payload_type = std::uint32_t;

    template<typename>
    struct matcher_handler;

    template<typename... Reject, typename... Require, typename AnyOf>
    struct matcher_handler<matcher<matcher<type_list<Reject...>, type_list<Require...>>, AnyOf>> {
        template<std::size_t Index>
        static void maybe_valid_if(basic_observer &obs, const Entity entt, const basic_registry<Entity> &reg) {
            if(reg.template has<Require...>(entt) && !(reg.template has<Reject>(entt) || ...)) {
                auto *comp = obs.view.try_get(entt);
                (comp ? *comp : obs.view.construct(entt)) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void discard_if(basic_observer &obs, const Entity entt) {
            if(auto *value = obs.view.try_get(entt); value && !(*value &= (~(1 << Index)))) {
                obs.view.destroy(entt);
            }
        }

        template<std::size_t Index>
        static void connect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<Reject>().template connect<&discard_if<Index>>(obs), ...);
            reg.template on_replace<AnyOf>().template connect<&maybe_valid_if<Index>>(obs);
            reg.template on_destroy<AnyOf>().template connect<&discard_if<Index>>(obs);
        }

        static void disconnect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().disconnect(obs), ...);
            (reg.template on_construct<Reject>().disconnect(obs), ...);
            reg.template on_replace<AnyOf>().disconnect(obs);
            reg.template on_destroy<AnyOf>().disconnect(obs);
        }
    };

    template<typename... Reject, typename... Require, typename... NoneOf, typename... AllOf>
    struct matcher_handler<matcher<matcher<type_list<Reject...>, type_list<Require...>>, type_list<NoneOf...>, type_list<AllOf...>>> {
        template<std::size_t Index>
        static void maybe_valid_if(basic_observer &obs, const Entity entt, const basic_registry<Entity> &reg) {
            if(reg.template has<AllOf...>(entt) && !(reg.template has<NoneOf>(entt) || ...)
                    && reg.template has<Require...>(entt) && !(reg.template has<Reject>(entt) || ...))
            {
                auto *comp = obs.view.try_get(entt);
                (comp ? *comp : obs.view.construct(entt)) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void discard_if(basic_observer &obs, const Entity entt) {
            if(auto *value = obs.view.try_get(entt); value && !(*value &= (~(1 << Index)))) {
                obs.view.destroy(entt);
            }
        }

        template<std::size_t Index>
        static void connect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<Reject>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<AllOf>().template connect<&maybe_valid_if<Index>>(obs), ...);
            (reg.template on_destroy<NoneOf>().template connect<&maybe_valid_if<Index>>(obs), ...);
            (reg.template on_destroy<AllOf>().template connect<&discard_if<Index>>(obs), ...);
            (reg.template on_construct<NoneOf>().template connect<&discard_if<Index>>(obs), ...);
        }

        static void disconnect(basic_observer &obs, basic_registry<Entity> &reg) {
            (reg.template on_destroy<Require>().disconnect(obs), ...);
            (reg.template on_construct<Reject>().disconnect(obs), ...);
            (reg.template on_construct<AllOf>().disconnect(obs), ...);
            (reg.template on_destroy<NoneOf>().disconnect(obs), ...);
            (reg.template on_destroy<AllOf>().disconnect(obs), ...);
            (reg.template on_construct<NoneOf>().disconnect(obs), ...);
        }
    };

    template<typename... Matcher>
    static void disconnect(basic_observer &obs, basic_registry<Entity> &reg) {
        (matcher_handler<Matcher>::disconnect(obs, reg), ...);
    }

    template<typename... Matcher, std::size_t... Index>
    void connect(basic_registry<Entity> &reg, std::index_sequence<Index...>) {
        static_assert(sizeof...(Matcher) < std::numeric_limits<payload_type>::digits);
        (matcher_handler<Matcher>::template connect<Index>(*this, reg), ...);
        release = &basic_observer::disconnect<Matcher...>;
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /*! @brief Default constructor. */
    basic_observer() ENTT_NOEXCEPT
        : target{}, release{}, view{}
    {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_observer(const basic_observer &) = delete;
    /*! @brief Default move constructor, deleted on purpose. */
    basic_observer(basic_observer &&) = delete;

    /**
     * @brief Creates an observer and connects it to a given registry.
     * @tparam Matcher Types of matchers to use to initialize the observer.
     * @param reg A valid reference to a registry.
     */
    template<typename... Matcher>
    basic_observer(basic_registry<entity_type> &reg, basic_collector<Matcher...>) ENTT_NOEXCEPT
        : target{&reg},
          release{},
          view{}
    {
        connect<Matcher...>(reg, std::make_index_sequence<sizeof...(Matcher)>{});
    }

    /*! @brief Default destructor. */
    ~basic_observer() = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This observer.
     */
    basic_observer & operator=(const basic_observer &) = delete;

    /**
     * @brief Default move assignment operator, deleted on purpose.
     * @return This observer.
     */
    basic_observer & operator=(basic_observer &&) = delete;

    /**
     * @brief Connects an observer to a given registry.
     * @tparam Matcher Types of matchers to use to initialize the observer.
     * @param reg A valid reference to a registry.
     */
    template<typename... Matcher>
    void connect(basic_registry<entity_type> &reg, basic_collector<Matcher...>) {
        disconnect();
        connect<Matcher...>(reg, std::make_index_sequence<sizeof...(Matcher)>{});
        target = &reg;
        view.reset();
    }

    /*! @brief Disconnects an observer from the registry it keeps track of. */
    void disconnect() {
        if(release) {
            release(*this, *target);
            release = nullptr;
        }
    }

    /**
     * @brief Returns the number of elements in an observer.
     * @return Number of elements.
     */
    size_type size() const ENTT_NOEXCEPT {
        return view.size();
    }

    /**
     * @brief Checks whether an observer is empty.
     * @return True if the observer is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return view.empty();
    }

    /**
     * @brief Direct access to the list of entities of the observer.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the observer in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return view.data();
    }

    /**
     * @brief Returns an iterator to the first entity of the observer.
     *
     * The returned iterator points to the first entity of the observer. If the
     * container is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the observer.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        return view.sparse_set<entity_type>::begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity of the observer.
     *
     * The returned iterator points to the entity following the last entity of
     * the observer. Attempting to dereference the returned iterator results in
     * undefined behavior.
     *
     * @return An iterator to the entity following the last entity of the
     * observer.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return view.sparse_set<entity_type>::end();
    }

    /*! @brief Resets the underlying container. */
    void clear() {
        view.reset();
    }

    /**
     * @brief Iterates entities and applies the given function object to them,
     * then clears the observer.
     *
     * The function object is invoked for each entity.<br/>
     * The signature of the function must be equivalent to the following form:
     *
     * @code{.cpp}
     * void(const entity_type);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        static_assert(std::is_invocable_v<Func, entity_type>);
        std::for_each(begin(), end(), std::move(func));
    }

    /**
     * @brief Iterates entities and applies the given function object to them,
     * then clears the observer.
     *
     * The function object is invoked for each entity.<br/>
     * The signature of the function must be equivalent to the following form:
     *
     * @code{.cpp}
     * void(const entity_type);
     * @endcode
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) {
        std::as_const(*this).each(std::move(func));
        clear();
    }

private:
    basic_registry<entity_type> *target;
    void(* release)(basic_observer &, basic_registry<entity_type> &);
    storage<entity_type, payload_type> view;
};


}


#endif // ENTT_ENTITY_OBSERVER_HPP
