#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "delegate.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @brief Sink class.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Type A valid signal handler type.
 */
template<typename Type>
class sink;

/**
 * @brief Unmanaged signal handler.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Type A valid function type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Type, typename Allocator>
class sigh;

/**
 * @brief Unmanaged signal handler.
 *
 * It works directly with references to classes and pointers to member functions
 * as well as pointers to free functions. Users of this class are in charge of
 * disconnecting instances before deleting them.
 *
 * This class serves mainly two purposes:
 *
 * * Creating signals to use later to notify a bunch of listeners.
 * * Collecting results from a set of functions like in a voting system.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Ret, typename... Args, typename Allocator>
class sigh<Ret(Args...), Allocator> {
    friend class sink<sigh<Ret(Args...), Allocator>>;

    using alloc_traits = std::allocator_traits<Allocator>;
    using delegate_type = delegate<Ret(Args...)>;
    using container_type = std::vector<delegate_type, typename alloc_traits::template rebind_alloc<delegate_type>>;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Sink type. */
    using sink_type = sink<sigh<Ret(Args...), Allocator>>;

    /*! @brief Default constructor. */
    sigh() noexcept(std::is_nothrow_default_constructible_v<allocator_type> && std::is_nothrow_constructible_v<container_type, const allocator_type &>)
        : sigh{allocator_type{}} {}

    /**
     * @brief Constructs a signal handler with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit sigh(const allocator_type &allocator) noexcept(std::is_nothrow_constructible_v<container_type, const allocator_type &>)
        : calls{allocator} {}

    /**
     * @brief Copy constructor.
     * @param other The instance to copy from.
     */
    sigh(const sigh &other) noexcept(std::is_nothrow_copy_constructible_v<container_type>)
        : calls{other.calls} {}

    /**
     * @brief Allocator-extended copy constructor.
     * @param other The instance to copy from.
     * @param allocator The allocator to use.
     */
    sigh(const sigh &other, const allocator_type &allocator) noexcept(std::is_nothrow_constructible_v<container_type, const container_type &, const allocator_type &>)
        : calls{other.calls, allocator} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    sigh(sigh &&other) noexcept(std::is_nothrow_move_constructible_v<container_type>)
        : calls{std::move(other.calls)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    sigh(sigh &&other, const allocator_type &allocator) noexcept(std::is_nothrow_constructible_v<container_type, container_type &&, const allocator_type &>)
        : calls{std::move(other.calls), allocator} {}

    /*! @brief Default destructor. */
    ~sigh() noexcept = default;

    /**
     * @brief Copy assignment operator.
     * @param other The instance to copy from.
     * @return This signal handler.
     */
    sigh &operator=(const sigh &other) noexcept(std::is_nothrow_copy_assignable_v<container_type>) {
        calls = other.calls;
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This signal handler.
     */
    sigh &operator=(sigh &&other) noexcept(std::is_nothrow_move_assignable_v<container_type>) {
        calls = std::move(other.calls);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given signal handler.
     * @param other Signal handler to exchange the content with.
     */
    void swap(sigh &other) noexcept(std::is_nothrow_swappable_v<container_type>) {
        using std::swap;
        swap(calls, other.calls);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return calls.get_allocator();
    }

    /**
     * @brief Number of listeners connected to the signal.
     * @return Number of listeners currently connected.
     */
    [[nodiscard]] size_type size() const noexcept {
        return calls.size();
    }

    /**
     * @brief Returns false if at least a listener is connected to the signal.
     * @return True if the signal has no listeners connected, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return calls.empty();
    }

    /**
     * @brief Triggers a signal.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @param args Arguments to use to invoke listeners.
     */
    void publish(Args... args) const {
        for(auto pos = calls.size(); pos; --pos) {
            calls[pos - 1u](args...);
        }
    }

    /**
     * @brief Collects return values from the listeners.
     *
     * The collector must expose a call operator with the following properties:
     *
     * * The return type is either `void` or such that it's convertible to
     *   `bool`. In the second case, a true value will stop the iteration.
     * * The list of parameters is empty if `Ret` is `void`, otherwise it
     *   contains a single element such that `Ret` is convertible to it.
     *
     * @tparam Func Type of collector to use, if any.
     * @param func A valid function object.
     * @param args Arguments to use to invoke listeners.
     */
    template<typename Func>
    void collect(Func func, Args... args) const {
        for(auto pos = calls.size(); pos; --pos) {
            if constexpr(std::is_void_v<Ret> || !std::is_invocable_v<Func, Ret>) {
                calls[pos - 1u](args...);

                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    if(func()) {
                        break;
                    }
                } else {
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    if(func(calls[pos - 1u](args...))) {
                        break;
                    }
                } else {
                    func(calls[pos - 1u](args...));
                }
            }
        }
    }

private:
    container_type calls;
};

/**
 * @brief Connection class.
 *
 * Opaque object the aim of which is to allow users to release an already
 * estabilished connection without having to keep a reference to the signal or
 * the sink that generated it.
 */
class connection {
    template<typename>
    friend class sink;

    connection(delegate<void(void *)> fn, void *ref)
        : disconnect{fn}, signal{ref} {}

public:
    /*! @brief Default constructor. */
    connection()
        : disconnect{},
          signal{} {}

    /**
     * @brief Checks whether a connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(disconnect);
    }

    /*! @brief Breaks the connection. */
    void release() {
        if(disconnect) {
            disconnect(signal);
            disconnect.reset();
        }
    }

private:
    delegate<void(void *)> disconnect;
    void *signal;
};

/**
 * @brief Scoped connection class.
 *
 * Opaque object the aim of which is to allow users to release an already
 * estabilished connection without having to keep a reference to the signal or
 * the sink that generated it.<br/>
 * A scoped connection automatically breaks the link between the two objects
 * when it goes out of scope.
 */
struct scoped_connection {
    /*! @brief Default constructor. */
    scoped_connection() = default;

    /**
     * @brief Constructs a scoped connection from a basic connection.
     * @param other A valid connection object.
     */
    scoped_connection(const connection &other)
        : conn{other} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    scoped_connection(const scoped_connection &) = delete;

    /**
     * @brief Move constructor.
     * @param other The scoped connection to move from.
     */
    scoped_connection(scoped_connection &&other) noexcept
        : conn{std::exchange(other.conn, {})} {}

    /*! @brief Automatically breaks the link on destruction. */
    ~scoped_connection() noexcept {
        conn.release();
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This scoped connection.
     */
    scoped_connection &operator=(const scoped_connection &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The scoped connection to move from.
     * @return This scoped connection.
     */
    scoped_connection &operator=(scoped_connection &&other) noexcept {
        conn = std::exchange(other.conn, {});
        return *this;
    }

    /**
     * @brief Acquires a connection.
     * @param other The connection object to acquire.
     * @return This scoped connection.
     */
    scoped_connection &operator=(connection other) {
        conn = other;
        return *this;
    }

    /**
     * @brief Checks whether a scoped connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(conn);
    }

    /*! @brief Breaks the connection. */
    void release() {
        conn.release();
    }

private:
    connection conn;
};

/**
 * @brief Sink class.
 *
 * A sink is used to connect listeners to signals and to disconnect them.<br/>
 * The function type for a listener is the one of the signal to which it
 * belongs.
 *
 * The clear separation between a signal and a sink permits to store the former
 * as private data member without exposing the publish functionality to the
 * users of the class.
 *
 * @warning
 * Lifetime of a sink must not overcome that of the signal to which it refers.
 * In any other case, attempting to use a sink results in undefined behavior.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Ret, typename... Args, typename Allocator>
class sink<sigh<Ret(Args...), Allocator>> {
    using signal_type = sigh<Ret(Args...), Allocator>;
    using delegate_type = typename signal_type::delegate_type;
    using difference_type = typename signal_type::container_type::difference_type;

    template<auto Candidate, typename Type>
    static void release(Type value_or_instance, void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>(value_or_instance);
    }

    template<auto Candidate>
    static void release(void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>();
    }

    template<typename Func>
    void disconnect_if(Func callback) {
        for(auto pos = signal->calls.size(); pos; --pos) {
            if(auto &elem = signal->calls[pos - 1u]; callback(elem)) {
                elem = std::move(signal->calls.back());
                signal->calls.pop_back();
            }
        }
    }

public:
    /**
     * @brief Constructs a sink that is allowed to modify a given signal.
     * @param ref A valid reference to a signal object.
     */
    sink(sigh<Ret(Args...), Allocator> &ref) noexcept
        : signal{&ref} {}

    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return signal->calls.empty();
    }

    /**
     * @brief Connects a free function (with or without payload), a bound or an
     * unbound member to a signal.
     * @tparam Candidate Function or member to connect to the signal.
     * @tparam Type Type of class or type of payload, if any.
     * @param value_or_instance A valid object that fits the purpose, if any.
     * @return A properly initialized connection object.
     */
    template<auto Candidate, typename... Type>
    connection connect(Type &&...value_or_instance) {
        disconnect<Candidate>(value_or_instance...);

        delegate_type call{};
        call.template connect<Candidate>(value_or_instance...);
        signal->calls.push_back(std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate, Type...>>(value_or_instance...);
        return {conn, signal};
    }

    /**
     * @brief Disconnects a free function (with or without payload), a bound or
     * an unbound member from a signal.
     * @tparam Candidate Function or member to disconnect from the signal.
     * @tparam Type Type of class or type of payload, if any.
     * @param value_or_instance A valid object that fits the purpose, if any.
     */
    template<auto Candidate, typename... Type>
    void disconnect(Type &&...value_or_instance) {
        delegate_type call{};
        call.template connect<Candidate>(value_or_instance...);
        disconnect_if([&call](const auto &elem) { return elem == call; });
    }

    /**
     * @brief Disconnects free functions with payload or bound members from a
     * signal.
     * @param value_or_instance A valid object that fits the purpose.
     */
    void disconnect(const void *value_or_instance) {
        if(value_or_instance) {
            disconnect_if([value_or_instance](const auto &elem) { return elem.data() == value_or_instance; });
        }
    }

    /*! @brief Disconnects all the listeners from a signal. */
    void disconnect() {
        signal->calls.clear();
    }

private:
    signal_type *signal;
};

/**
 * @brief Deduction guide.
 *
 * It allows to deduce the signal handler type of a sink directly from the
 * signal it refers to.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Ret, typename... Args, typename Allocator>
sink(sigh<Ret(Args...), Allocator> &) -> sink<sigh<Ret(Args...), Allocator>>;

} // namespace entt

#endif
