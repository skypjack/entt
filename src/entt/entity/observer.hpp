#ifndef ENTT_ENTITY_OBSERVER_HPP
#define ENTT_ENTITY_OBSERVER_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include "../core/type_traits.hpp"
#include "fwd.hpp"
#include "storage.hpp"

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
     * @tparam AllOf Types of elements tracked by the matcher.
     * @tparam NoneOf Types of elements used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto group(exclude_t<NoneOf...> = exclude_t{}) noexcept {
        return basic_collector<matcher<type_list<>, type_list<>, type_list<NoneOf...>, AllOf...>>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of element for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto update() noexcept {
        return basic_collector<matcher<type_list<>, type_list<>, AnyOf>>{};
    }
};

/**
 * @brief Collector.
 * @copydetails basic_collector<>
 * @tparam Reject Untracked types used to filter out entities.
 * @tparam Require Untracked types required by the matcher.
 * @tparam Rule Specific details of the current matcher.
 * @tparam Other Other matchers.
 */
template<typename... Reject, typename... Require, typename... Rule, typename... Other>
struct [[deprecated("use reactive mixin instead")]] basic_collector<matcher<type_list<Reject...>, type_list<Require...>, Rule...>, Other...> {
    /*! @brief Current matcher. */
    using current_type = matcher<type_list<Reject...>, type_list<Require...>, Rule...>;

    /**
     * @brief Adds a grouping matcher to the collector.
     * @tparam AllOf Types of elements tracked by the matcher.
     * @tparam NoneOf Types of elements used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto group(exclude_t<NoneOf...> = exclude_t{}) noexcept {
        return basic_collector<matcher<type_list<>, type_list<>, type_list<NoneOf...>, AllOf...>, current_type, Other...>{};
    }

    /**
     * @brief Adds an observing matcher to the collector.
     * @tparam AnyOf Type of element for which changes should be detected.
     * @return The updated collector.
     */
    template<typename AnyOf>
    static constexpr auto update() noexcept {
        return basic_collector<matcher<type_list<>, type_list<>, AnyOf>, current_type, Other...>{};
    }

    /**
     * @brief Updates the filter of the last added matcher.
     * @tparam AllOf Types of elements required by the matcher.
     * @tparam NoneOf Types of elements used to filter out entities.
     * @return The updated collector.
     */
    template<typename... AllOf, typename... NoneOf>
    static constexpr auto where(exclude_t<NoneOf...> = exclude_t{}) noexcept {
        using extended_type = matcher<type_list<Reject..., NoneOf...>, type_list<Require..., AllOf...>, Rule...>;
        return basic_collector<extended_type, Other...>{};
    }
};

/*! @brief Variable template used to ease the definition of collectors. */
inline constexpr basic_collector<> collector{};

/**
 * @brief Observer.
 *
 * An observer returns all the entities and only the entities that fit the
 * requirements of at least one matcher. Moreover, it's guaranteed that the
 * entity list is tightly packed in memory for fast iterations.<br/>
 * In general, observers don't stay true to the order of any set of elements.
 *
 * Observers work mainly with two types of matchers, provided through a
 * collector:
 *
 * * Observing matcher: an observer will return at least all the living entities
 *   for which one or more of the given elements have been updated and not yet
 *   destroyed.
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
 * * New instances of the given elements are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given elements is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all the other cases, modifying the pools of the given elements in any way
 * invalidates all the iterators.
 *
 * @warning
 * Lifetime of an observer doesn't necessarily have to overcome that of the
 * registry to which it is connected. However, the observer must be disconnected
 * from the registry before being destroyed to avoid crashes due to dangling
 * pointers.
 *
 * @tparam Registry Basic registry type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Registry, typename Allocator>
class basic_observer {
    using mask_type = std::uint64_t;
    using storage_type = basic_storage<mask_type, typename Registry::entity_type, typename std::allocator_traits<Allocator>::template rebind_alloc<mask_type>>;

    template<std::size_t Index>
    static void discard_if(storage_type &storage, Registry &, const typename Registry::entity_type entt) {
        if(storage.contains(entt) && !(storage.get(entt) &= (~(1 << Index)))) {
            storage.erase(entt);
        }
    }

    template<typename>
    struct matcher_handler;

    template<typename... Reject, typename... Require, typename AnyOf>
    struct matcher_handler<matcher<type_list<Reject...>, type_list<Require...>, AnyOf>> {
        template<std::size_t Index>
        static void maybe_valid_if(storage_type &storage, Registry &parent, const typename Registry::entity_type entt) {
            if(parent.template all_of<Require...>(entt) && !parent.template any_of<Reject...>(entt)) {
                if(!storage.contains(entt)) {
                    storage.emplace(entt);
                }

                storage.get(entt) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void connect(storage_type &storage, Registry &parent) {
            (parent.template on_destroy<Require>().template connect<&discard_if<Index>>(storage), ...);
            (parent.template on_construct<Reject>().template connect<&discard_if<Index>>(storage), ...);
            parent.template on_update<AnyOf>().template connect<&maybe_valid_if<Index>>(storage);
            parent.template on_destroy<AnyOf>().template connect<&discard_if<Index>>(storage);
        }

        static void disconnect(storage_type &storage, Registry &parent) {
            (parent.template on_destroy<Require>().disconnect(&storage), ...);
            (parent.template on_construct<Reject>().disconnect(&storage), ...);
            parent.template on_update<AnyOf>().disconnect(&storage);
            parent.template on_destroy<AnyOf>().disconnect(&storage);
        }
    };

    template<typename... Reject, typename... Require, typename... NoneOf, typename... AllOf>
    struct matcher_handler<matcher<type_list<Reject...>, type_list<Require...>, type_list<NoneOf...>, AllOf...>> {
        template<std::size_t Index, typename... Ignore>
        static void maybe_valid_if(storage_type &storage, Registry &parent, const typename Registry::entity_type entt) {
            bool guard{};

            if constexpr(sizeof...(Ignore) == 0) {
                guard = parent.template all_of<AllOf..., Require...>(entt) && !parent.template any_of<NoneOf..., Reject...>(entt);
            } else {
                guard = parent.template all_of<AllOf..., Require...>(entt) && ((std::is_same_v<Ignore..., NoneOf> || !parent.template any_of<NoneOf>(entt)) && ...) && !parent.template any_of<Reject...>(entt);
            }

            if(guard) {
                if(!storage.contains(entt)) {
                    storage.emplace(entt);
                }

                storage.get(entt) |= (1 << Index);
            }
        }

        template<std::size_t Index>
        static void connect(storage_type &storage, Registry &parent) {
            (parent.template on_destroy<Require>().template connect<&discard_if<Index>>(storage), ...);
            (parent.template on_construct<Reject>().template connect<&discard_if<Index>>(storage), ...);
            (parent.template on_construct<AllOf>().template connect<&maybe_valid_if<Index>>(storage), ...);
            (parent.template on_destroy<NoneOf>().template connect<&maybe_valid_if<Index, NoneOf>>(storage), ...);
            (parent.template on_destroy<AllOf>().template connect<&discard_if<Index>>(storage), ...);
            (parent.template on_construct<NoneOf>().template connect<&discard_if<Index>>(storage), ...);
        }

        static void disconnect(storage_type &storage, Registry &parent) {
            (parent.template on_destroy<Require>().disconnect(&storage), ...);
            (parent.template on_construct<Reject>().disconnect(&storage), ...);
            (parent.template on_construct<AllOf>().disconnect(&storage), ...);
            (parent.template on_destroy<NoneOf>().disconnect(&storage), ...);
            (parent.template on_destroy<AllOf>().disconnect(&storage), ...);
            (parent.template on_construct<NoneOf>().disconnect(&storage), ...);
        }
    };

    template<typename... Matcher>
    static void disconnect(Registry &parent, storage_type &storage) {
        (matcher_handler<Matcher>::disconnect(storage, parent), ...);
    }

    template<typename... Matcher, std::size_t... Index>
    static void connect(Registry &parent, storage_type &storage, std::index_sequence<Index...>) {
        static_assert(sizeof...(Matcher) < std::numeric_limits<mask_type>::digits, "Too many matchers");
        (matcher_handler<Matcher>::template connect<Index>(storage, parent), ...);
    }

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! Basic registry type. */
    using registry_type = Registry;
    /*! @brief Underlying entity identifier. */
    using entity_type = typename registry_type::entity_type;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Random access iterator type. */
    using iterator = typename registry_type::common_type::iterator;

    /*! @brief Default constructor. */
    basic_observer()
        : basic_observer{allocator_type{}} {}

    /**
     * @brief Constructs an empty storage with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_observer(const allocator_type &allocator)
        : release{},
          parent{},
          storage{allocator} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_observer(const basic_observer &) = delete;

    /*! @brief Default move constructor, deleted on purpose. */
    basic_observer(basic_observer &&) = delete;

    /**
     * @brief Creates an observer and connects it to a given registry.
     * @tparam Matcher Types of matchers to use to initialize the observer.
     * @param reg A valid reference to a registry.
     * @param allocator The allocator to use.
     */
    template<typename... Matcher>
    basic_observer(registry_type &reg, basic_collector<Matcher...>, const allocator_type &allocator = allocator_type{})
        : release{&basic_observer::disconnect<Matcher...>},
          parent{&reg},
          storage{allocator} {
        connect<Matcher...>(reg, storage, std::index_sequence_for<Matcher...>{});
    }

    /*! @brief Default destructor. */
    ~basic_observer() = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This observer.
     */
    basic_observer &operator=(const basic_observer &) = delete;

    /**
     * @brief Default move assignment operator, deleted on purpose.
     * @return This observer.
     */
    basic_observer &operator=(basic_observer &&) = delete;

    /**
     * @brief Connects an observer to a given registry.
     * @tparam Matcher Types of matchers to use to initialize the observer.
     * @param reg A valid reference to a registry.
     */
    template<typename... Matcher>
    void connect(registry_type &reg, basic_collector<Matcher...>) {
        disconnect();
        storage.clear();

        parent = &reg;
        release = &basic_observer::disconnect<Matcher...>;
        connect<Matcher...>(reg, storage, std::index_sequence_for<Matcher...>{});
    }

    /*! @brief Disconnects an observer from the registry it keeps track of. */
    void disconnect() {
        if(release) {
            release(*parent, storage);
            release = nullptr;
        }
    }

    /**
     * @brief Returns the number of elements in an observer.
     * @return Number of elements.
     */
    [[nodiscard]] size_type size() const noexcept {
        return storage.size();
    }

    /**
     * @brief Checks whether an observer is empty.
     * @return True if the observer is empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return storage.empty();
    }

    /**
     * @brief Direct access to the list of entities of the observer.
     *
     * The returned pointer is such that range `[data(), data() + size())` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * Entities are in the reverse order as returned by the `begin`/`end`
     * iterators.
     *
     * @return A pointer to the array of entities.
     */
    [[nodiscard]] const entity_type *data() const noexcept {
        return storage.data();
    }

    /**
     * @brief Returns an iterator to the first entity of the observer.
     *
     * If the observer is empty, the returned iterator will be equal to `end()`.
     *
     * @return An iterator to the first entity of the observer.
     */
    [[nodiscard]] iterator begin() const noexcept {
        return storage.storage_type::base_type::begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity of the observer.
     * @return An iterator to the entity following the last entity of the
     * observer.
     */
    [[nodiscard]] iterator end() const noexcept {
        return storage.storage_type::base_type::end();
    }

    /*! @brief Clears the underlying container. */
    void clear() noexcept {
        storage.clear();
    }

    /**
     * @brief Iterates entities and applies the given function object to them.
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
        for(const auto entity: *this) {
            func(entity);
        }
    }

    /**
     * @brief Iterates entities and applies the given function object to them,
     * then clears the observer.
     *
     * @sa each
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
    void (*release)(registry_type &, storage_type &);
    registry_type *parent;
    storage_type storage;
};

} // namespace entt

#endif
