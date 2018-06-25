#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <algorithm>
#include <utility>
#include <vector>
#include "../config/config.h"


namespace entt {


namespace internal {


/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


template<typename, typename>
struct Invoker;


template<typename Ret, typename... Args, typename Collector>
struct Invoker<Ret(Args...), Collector> {
    using proto_fn_type = Ret(void *, Args...);
    using call_type = std::pair<void *, proto_fn_type *>;

    virtual ~Invoker() = default;

    bool invoke(Collector &collector, proto_fn_type *proto, void *instance, Args... args) const {
        return collector(proto(instance, args...));
    }
};


template<typename... Args, typename Collector>
struct Invoker<void(Args...), Collector> {
    using proto_fn_type = void(void *, Args...);
    using call_type = std::pair<void *, proto_fn_type *>;

    virtual ~Invoker() = default;

    bool invoke(Collector &, proto_fn_type *proto, void *instance, Args... args) const {
        return (proto(instance, args...), true);
    }
};


template<typename Ret>
struct NullCollector final {
    using result_type = Ret;
    bool operator()(result_type) const ENTT_NOEXCEPT { return true; }
};


template<>
struct NullCollector<void> final {
    using result_type = void;
    bool operator()() const ENTT_NOEXCEPT { return true; }
};


template<typename>
struct DefaultCollector;


template<typename Ret, typename... Args>
struct DefaultCollector<Ret(Args...)> final {
    using collector_type = NullCollector<Ret>;
};


template<typename Function>
using DefaultCollectorType = typename DefaultCollector<Function>::collector_type;


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


}


/**
 * @brief Sink implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class Sink;


/**
 * @brief Unmanaged signal handler declaration.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 * @tparam Collector Type of collector to use, if any.
 */
template<typename Function, typename Collector = internal::DefaultCollectorType<Function>>
class SigH;


/**
 * @brief Sink implementation.
 *
 * A sink is an opaque object used to connect listeners to signals.<br/>
 * The function type for a listener is the one of the signal to which it
 * belongs.
 *
 * The clear separation between a signal and a sink permits to store the
 * former as private data member without exposing the publish functionality
 * to the users of a class.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class Sink<Ret(Args...)> final {
    /*! @brief A signal is allowed to create sinks. */
    template<typename, typename>
    friend class SigH;

    using proto_fn_type = Ret(void *, Args...);
    using call_type = std::pair<void *, proto_fn_type *>;

    template<Ret(*Function)(Args...)>
    static Ret proto(void *, Args... args) {
        return (Function)(args...);
    }

    template<typename Class, Ret(Class:: *Member)(Args... args)>
    static Ret proto(void *instance, Args... args) {
        return (static_cast<Class *>(instance)->*Member)(args...);
    }

    Sink(std::vector<call_type> &calls) ENTT_NOEXCEPT
        : calls{calls}
    {}

public:
    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for
     * free functions.
     *
     * @tparam Function A valid free function pointer.
     */
    template<Ret(*Function)(Args...)>
    void connect() {
        disconnect<Function>();
        calls.emplace_back(nullptr, &proto<Function>);
    }

    /**
     * @brief Connects a member function for a given instance to a signal.
     *
     * The signal isn't responsible for the connected object. Users must
     * guarantee that the lifetime of the instance overcomes the one of the
     * signal. On the other side, the signal handler performs checks to
     * avoid multiple connections for the same member function of a given
     * instance.
     *
     * @tparam Class Type of class to which the member function belongs.
     * @tparam Member Member function to connect to the signal.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template <typename Class, Ret(Class:: *Member)(Args...) = &Class::receive>
    void connect(Class *instance) {
        disconnect<Class, Member>(instance);
        calls.emplace_back(instance, &proto<Class, Member>);
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<Ret(*Function)(Args...)>
    void disconnect() {
        call_type target{nullptr, &proto<Function>};
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(target)), calls.end());
    }

    /**
     * @brief Disconnects the given member function from a signal.
     * @tparam Class Type of class to which the member function belongs.
     * @tparam Member Member function to connect to the signal.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class, Ret(Class:: *Member)(Args...)>
    void disconnect(Class *instance) {
        call_type target{instance, &proto<Class, Member>};
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(target)), calls.end());
    }

    /**
     * @brief Removes all existing connections for the given instance.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class>
    void disconnect(Class *instance) {
        auto func = [instance](const call_type &call) { return call.first == instance; };
        calls.erase(std::remove_if(calls.begin(), calls.end(), std::move(func)), calls.end());
    }

    /**
     * @brief Disconnects all the listeners from a signal.
     */
    void disconnect() {
        calls.clear();
    }

private:
    std::vector<call_type> &calls;
};


/**
 * @brief Unmanaged signal handler definition.
 *
 * Unmanaged signal handler. It works directly with naked pointers to classes
 * and pointers to member functions as well as pointers to free functions. Users
 * of this class are in charge of disconnecting instances before deleting them.
 *
 * This class serves mainly two purposes:
 *
 * * Creating signals used later to notify a bunch of listeners.
 * * Collecting results from a set of functions like in a voting system.
 *
 * The default collector does nothing. To properly collect data, define and use
 * a class that has a call operator the signature of which is `bool(Param)` and:
 *
 * * `Param` is a type to which `Ret` can be converted.
 * * The return type is true if the handler must stop collecting data, false
 * otherwise.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @tparam Collector Type of collector to use, if any.
 */
template<typename Ret, typename... Args, typename Collector>
class SigH<Ret(Args...), Collector> final: private internal::Invoker<Ret(Args...), Collector> {
    using call_type = typename internal::Invoker<Ret(Args...), Collector>::call_type;

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<call_type>::size_type;
    /*! @brief Collector type. */
    using collector_type = Collector;
    /*! @brief Sink type. */
    using sink_type = Sink<Ret(Args...)>;

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
     * @brief Returns a sink object for the given signal.
     *
     * A sink is an opaque object used to connect listeners to signals.<br/>
     * The function type for a listener is the one of the signal to which it
     * belongs. The order of invocation of the listeners isn't guaranteed.
     *
     * @return A temporary sink object.
     */
    sink_type sink() ENTT_NOEXCEPT {
        return { calls };
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
            auto &call = calls[pos-1];
            call.second(call.first, args...);
        }
    }

    /**
     * @brief Collects return values from the listeners.
     * @param args Arguments to use to invoke listeners.
     * @return An instance of the collector filled with collected data.
     */
    collector_type collect(Args... args) const {
        collector_type collector;

        for(auto &&call: calls) {
            if(!this->invoke(collector, call.second, call.first, args...)) {
                break;
            }
        }

        return collector;
    }

    /**
     * @brief Swaps listeners between the two signals.
     * @param lhs A valid signal object.
     * @param rhs A valid signal object.
     */
    friend void swap(SigH &lhs, SigH &rhs) {
        using std::swap;
        swap(lhs.calls, rhs.calls);
    }

    /**
     * @brief Checks if the contents of the two signals are identical.
     *
     * Two signals are identical if they have the same size and the same
     * listeners registered exactly in the same order.
     *
     * @param other Signal with which to compare.
     * @return True if the two signals are identical, false otherwise.
     */
    bool operator==(const SigH &other) const ENTT_NOEXCEPT {
        return std::equal(calls.cbegin(), calls.cend(), other.calls.cbegin(), other.calls.cend());
    }

private:
    std::vector<call_type> calls;
};


/**
 * @brief Checks if the contents of the two signals are different.
 *
 * Two signals are identical if they have the same size and the same
 * listeners registered exactly in the same order.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid signal object.
 * @param rhs A valid signal object.
 * @return True if the two signals are different, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const SigH<Ret(Args...)> &lhs, const SigH<Ret(Args...)> &rhs) ENTT_NOEXCEPT {
    return !(lhs == rhs);
}


}


#endif // ENTT_SIGNAL_SIGH_HPP
