#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <algorithm>
#include <utility>
#include <vector>
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
struct sink;


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
 * It works directly with naked pointers to classes and pointers to member
 * functions as well as pointers to free functions. Users of this class are in
 * charge of disconnecting instances before deleting them.
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
    friend struct sink<Ret(Args...)>;

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<delegate<Ret(Args...)>>::size_type;
    /*! @brief Sink type. */
    using sink_type = entt::sink<Ret(Args...)>;

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
    size_type size() const ENTT_NOEXCEPT {
        return calls.size();
    }

    /**
     * @brief Returns false if at least a listener is connected to the signal.
     * @return True if the signal has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
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
            calls[pos-1](args...);
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
        bool stop = false;

        for(auto pos = calls.size(); pos && !stop; --pos) {
            if constexpr(std::is_void_v<Ret>) {
                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    calls[pos-1](args...);
                    stop = func();
                } else {
                    calls[pos-1](args...);
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    stop = func(calls[pos-1](args...));
                } else {
                    func(calls[pos-1](args...));
                }
            }
        }
    }

    /**
     * @brief Swaps listeners between the two signals.
     * @param lhs A valid signal object.
     * @param rhs A valid signal object.
     */
    friend void swap(sigh &lhs, sigh &rhs) {
        using std::swap;
        swap(lhs.calls, rhs.calls);
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
    friend struct sink;

public:
    /*! @brief Breaks the connection. */
    void release() {
        disconnect(parent, value_or_instance);
    }

private:
    void(* disconnect)(void *, const void *){};
    const void *value_or_instance{};
    void *parent{};
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
struct scoped_connection: private connection {
    /**
     * @brief Constructs a scoped connection from a basic connection.
     * @param conn A valid connection object.
     */
    scoped_connection(const connection &conn)
        : connection{conn}
    {}

    /*! @brief Automatically breaks the link on destruction. */
    ~scoped_connection() {
        connection::release();
    }
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
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
struct sink<Ret(Args...)> {
    /*! @brief Signal type. */
    using signal_type = sigh<Ret(Args...)>;

    /**
     * @brief Constructs a sink that is allowed to modify a given signal.
     * @param ref A valid reference to a signal object.
     */
    sink(sigh<Ret(Args...)> &ref) ENTT_NOEXCEPT
        : parent{&ref}
    {}

    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return parent->calls.empty();
    }

    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     * @return A properly initialized connection object.
     */
    template<auto Function>
    connection connect() {
        disconnect<Function>();

        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Function>();
        parent->calls.emplace_back(std::move(delegate));

        connection conn;
        conn.parent = parent;
        conn.value_or_instance = nullptr;
        conn.disconnect = [](void *parent, const void *) {
            sink{*static_cast<signal_type *>(parent)}.disconnect<Function>();
        };

        return conn;
    }

    /**
     * @brief Connects a member function or a free function with payload to a
     * signal.
     *
     * The signal isn't responsible for the connected object or the payload.
     * Users must always guarantee that the lifetime of the instance overcomes
     * the one  of the delegate. On the other side, the signal handler performs
     * checks to avoid multiple connections for the same function.<br/>
     * When used to connect a free function with payload, its signature must be
     * such that the instance is the first argument before the ones used to
     * define the delegate itself.
     *
     * @tparam Candidate Member or free function to connect to the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     * @return A properly initialized connection object.
     */
    template<auto Candidate, typename Type>
    connection connect(Type *value_or_instance) {
        disconnect<Candidate>(value_or_instance);

        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Candidate>(value_or_instance);
        parent->calls.emplace_back(std::move(delegate));

        connection conn;
        conn.parent = parent;
        conn.value_or_instance = value_or_instance;
        conn.disconnect = [](void *parent, const void *value_or_instance) {
            Type *curr = nullptr;

            if constexpr(std::is_const_v<Type>) {
                curr = static_cast<Type *>(value_or_instance);
            } else {
                curr = static_cast<Type *>(const_cast<void *>(value_or_instance));
            }

            sink{*static_cast<signal_type *>(parent)}.disconnect<Candidate>(curr);
        };

        return conn;
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void disconnect() {
        auto &calls = parent->calls;
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Function>();
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(delegate)), calls.end());
    }

    /**
     * @brief Disconnects a member function or a free function with payload from
     * a signal.
     * @tparam Candidate Member or free function to disconnect from the signal.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void disconnect(Type *value_or_instance) {
        auto &calls = parent->calls;
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Candidate>(value_or_instance);
        calls.erase(std::remove_if(calls.begin(), calls.end(), [delegate](const auto &other) {
            return other == delegate && other.instance() == delegate.instance();
        }), calls.end());
    }

    /**
     * @brief Disconnects member functions or free functions based on an
     * instance or specific payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    void disconnect(const void *value_or_instance) {
        auto &calls = parent->calls;
        calls.erase(std::remove_if(calls.begin(), calls.end(), [value_or_instance](const auto &delegate) {
            return value_or_instance == delegate.instance();
        }), calls.end());
    }

    /*! @brief Disconnects all the listeners from a signal. */
    void disconnect() {
        parent->calls.clear();
    }

private:
    signal_type *parent;
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
sink(sigh<Ret(Args...)> &) ENTT_NOEXCEPT -> sink<Ret(Args...)>;


}


#endif // ENTT_SIGNAL_SIGH_HPP
