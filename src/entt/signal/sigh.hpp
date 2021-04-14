#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <vector>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
#include <type_traits>
#include "../config/config.h"
#include "delegate.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief Sink class.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sink;


/**
 * @brief Unmanaged signal handler.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
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
 */
template<typename Ret, typename... Args>
class sigh<Ret(Args...)> {
    /*! @brief A sink is allowed to modify a signal. */
    friend class sink<Ret(Args...)>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Sink type. */
    using sink_type = sink<Ret(Args...)>;

    /**
     * @brief Instance type when it comes to connecting member functions.
     * @tparam Class Type of class to which the member function belongs.
     */
    template<typename Class>
    using instance_type = Class *;

    /**
     * @brief Number of listeners connected to the signal.
     * @return Number of listeners currently connected.
     */
    [[nodiscard]] size_type size() const ENTT_NOEXCEPT {
        return calls.size();
    }

    /**
     * @brief Returns false if at least a listener is connected to the signal.
     * @return True if the signal has no listeners connected, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
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
        for(auto &&call: std::as_const(calls)) {
            call(args...);
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
        for(auto &&call: calls) {
            if constexpr(std::is_void_v<Ret>) {
                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    call(args...);
                    if(func()) { break; }
                } else {
                    call(args...);
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    if(func(call(args...))) { break; }
                } else {
                    func(call(args...));
                }
            }
        }
    }

private:
    std::vector<delegate<Ret(Args...)>> calls;
};


/**
 * @brief Connection class.
 *
 * Opaque object the aim of which is to allow users to release an already
 * estabilished connection without having to keep a reference to the signal or
 * the sink that generated it.
 */
class connection {
    /*! @brief A sink is allowed to create connection objects. */
    template<typename>
    friend class sink;

    connection(delegate<void(void *)> fn, void *ref)
        : disconnect{fn}, signal{ref}
    {}

public:
    /*! @brief Default constructor. */
    connection() = default;

    /**
     * @brief Checks whether a connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
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
    void *signal{};
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
        : conn{other}
    {}

    /*! @brief Default copy constructor, deleted on purpose. */
    scoped_connection(const scoped_connection &) = delete;

    /*! @brief Automatically breaks the link on destruction. */
    ~scoped_connection() {
        conn.release();
    }

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This scoped connection.
     */
    scoped_connection & operator=(const scoped_connection &) = delete;

    /**
     * @brief Acquires a connection.
     * @param other The connection object to acquire.
     * @return This scoped connection.
     */
    scoped_connection & operator=(connection other) {
        conn = std::move(other);
        return *this;
    }

    /**
     * @brief Checks whether a scoped connection is properly initialized.
     * @return True if the connection is properly initialized, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const ENTT_NOEXCEPT {
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
 */
template<typename Ret, typename... Args>
class sink<Ret(Args...)> {
    using signal_type = sigh<Ret(Args...)>;
    using difference_type = typename std::iterator_traits<typename decltype(signal_type::calls)::iterator>::difference_type;

    template<auto Candidate, typename Type>
    static void release(Type value_or_instance, void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>(value_or_instance);
    }

    template<auto Candidate>
    static void release(void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>();
    }

public:
    /**
     * @brief Constructs a sink that is allowed to modify a given signal.
     * @param ref A valid reference to a signal object.
     */
    sink(sigh<Ret(Args...)> &ref) ENTT_NOEXCEPT
        : offset{},
          signal{&ref}
    {}

    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    [[nodiscard]] bool empty() const ENTT_NOEXCEPT {
        return signal->calls.empty();
    }

    /**
     * @brief Returns a sink that connects before a given free function or an
     * unbound member.
     * @tparam Function A valid free function pointer.
     * @return A properly initialized sink object.
     */
    template<auto Function>
    [[nodiscard]] sink before() {
        delegate<Ret(Args...)> call{};
        call.template connect<Function>();

        const auto &calls = signal->calls;
        const auto it = std::find(calls.cbegin(), calls.cend(), std::move(call));

        sink other{*this};
        other.offset = std::distance(it, calls.cend());
        return other;
    }

    /**
     * @brief Returns a sink that connects before a free function with payload
     * or a bound member.
     * @tparam Candidate Member or free function to look for.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<auto Candidate, typename Type>
    [[nodiscard]] sink before(Type &&value_or_instance) {
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(value_or_instance);

        const auto &calls = signal->calls;
        const auto it = std::find(calls.cbegin(), calls.cend(), std::move(call));

        sink other{*this};
        other.offset = std::distance(it, calls.cend());
        return other;
    }

    /**
     * @brief Returns a sink that connects before a given instance or specific
     * payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<typename Type>
    [[nodiscard]] sink before(Type &value_or_instance) {
        return before(&value_or_instance);
    }

    /**
     * @brief Returns a sink that connects before a given instance or specific
     * payload.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     * @return A properly initialized sink object.
     */
    template<typename Type>
    [[nodiscard]] sink before(Type *value_or_instance) {
        sink other{*this};

        if(value_or_instance) {
            const auto &calls = signal->calls;
            const auto it = std::find_if(calls.cbegin(), calls.cend(), [value_or_instance](const auto &delegate) {
                return delegate.instance() == value_or_instance;
            });

            other.offset = std::distance(it, calls.cend());
        }

        return other;
    }

    /**
     * @brief Returns a sink that connects before anything else.
     * @return A properly initialized sink object.
     */
    [[nodiscard]] sink before() {
        sink other{*this};
        other.offset = signal->calls.size();
        return other;
    }

    /**
     * @brief Connects a free function or an unbound member to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for the
     * same function.
     *
     * @tparam Candidate Function or member to connect to the signal.
     * @return A properly initialized connection object.
     */
    template<auto Candidate>
    connection connect() {
        disconnect<Candidate>();

        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>();
        signal->calls.insert(signal->calls.end() - offset, std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate>>();
        return { std::move(conn), signal };
    }

    /**
     * @brief Connects a free function with payload or a bound member to a
     * signal.
     *
     * The signal isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one of the signal. On the other side, the signal handler performs
     * checks to avoid multiple connections for the same function.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the signal itself.
     *
     * @tparam Candidate Function or member to connect to the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     * @return A properly initialized connection object.
     */
    template<auto Candidate, typename Type>
    connection connect(Type &&value_or_instance) {
        disconnect<Candidate>(value_or_instance);

        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(value_or_instance);
        signal->calls.insert(signal->calls.end() - offset, std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate, Type>>(value_or_instance);
        return { std::move(conn), signal };
    }

    /**
     * @brief Disconnects a free function or an unbound member from a signal.
     * @tparam Candidate Function or member to disconnect from the signal.
     */
    template<auto Candidate>
    void disconnect() {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>();
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(call)), calls.end());
    }

    /**
     * @brief Disconnects a free function with payload or a bound member from a
     * signal.
     * @tparam Candidate Function or member to disconnect from the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void disconnect(Type &&value_or_instance) {
        auto &calls = signal->calls;
        delegate<Ret(Args...)> call{};
        call.template connect<Candidate>(value_or_instance);
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(call)), calls.end());
    }

    /**
     * @brief Disconnects free functions with payload or bound members from a
     * signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type &value_or_instance) {
        disconnect(&value_or_instance);
    }

    /**
     * @brief Disconnects free functions with payload or bound members from a
     * signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid object that fits the purpose.
     */
    template<typename Type>
    void disconnect(Type *value_or_instance) {
        if(value_or_instance) {
            auto &calls = signal->calls;
            calls.erase(std::remove_if(calls.begin(), calls.end(), [value_or_instance](const auto &delegate) {
                return delegate.instance() == value_or_instance;
            }), calls.end());
        }
    }

    /*! @brief Disconnects all the listeners from a signal. */
    void disconnect() {
        signal->calls.clear();
    }

private:
    difference_type offset;
    signal_type *signal;
};


/**
 * @brief Deduction guide.
 *
 * It allows to deduce the function type of a sink directly from the signal it
 * refers to.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
sink(sigh<Ret(Args...)> &)
-> sink<Ret(Args...)>;


}


#endif
