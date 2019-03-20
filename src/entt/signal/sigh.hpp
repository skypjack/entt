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
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */


namespace internal {


template<typename, typename>
struct invoker;


template<typename Ret, typename... Args, typename Collector>
struct invoker<Ret(Args...), Collector> {
    virtual ~invoker() = default;

    bool invoke(Collector &collector, const delegate<Ret(Args...)> &delegate, Args... args) const {
        return collector(delegate(args...));
    }
};


template<typename... Args, typename Collector>
struct invoker<void(Args...), Collector> {
    virtual ~invoker() = default;

    bool invoke(Collector &, const delegate<void(Args...)> &delegate, Args... args) const {
        return (delegate(args...), true);
    }
};


template<typename Ret>
struct null_collector {
    using result_type = Ret;
    bool operator()(result_type) const ENTT_NOEXCEPT { return true; }
};


template<>
struct null_collector<void> {
    using result_type = void;
    bool operator()() const ENTT_NOEXCEPT { return true; }
};


template<typename>
struct default_collector;


template<typename Ret, typename... Args>
struct default_collector<Ret(Args...)> {
    using collector_type = null_collector<Ret>;
};


template<typename Function>
using default_collector_type = typename default_collector<Function>::collector_type;


}


/**
 * Internal details not to be documented.
 * @endcond TURN_OFF_DOXYGEN
 */


/**
 * @brief Sink implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 */
template<typename Function>
class sink;


/**
 * @brief Unmanaged signal handler declaration.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 *
 * @tparam Function A valid function type.
 * @tparam Collector Type of collector to use, if any.
 */
template<typename Function, typename Collector = internal::default_collector_type<Function>>
struct sigh;


/**
 * @brief Sink implementation.
 *
 * A sink is an opaque object used to connect listeners to signals.<br/>
 * The function type for a listener is the one of the signal to which it
 * belongs.
 *
 * The clear separation between a signal and a sink permits to store the former
 * as private data member without exposing the publish functionality to the
 * users of a class.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class sink<Ret(Args...)> {
    /*! @brief A signal is allowed to create sinks. */
    template<typename, typename>
    friend struct sigh;

    template<typename Type>
    Type * payload_type(Ret(*)(Type *, Args...));

    sink(std::vector<delegate<Ret(Args...)>> *ref) ENTT_NOEXCEPT
        : calls{ref}
    {}

public:
    /**
     * @brief Returns false if at least a listener is connected to the sink.
     * @return True if the sink has no listeners connected, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return calls->empty();
    }

    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void connect() {
        disconnect<Function>();
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Function>();
        calls->emplace_back(std::move(delegate));
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
     * @tparam Candidate Member or free function to connect to the delegate.
     * @tparam Type Type of class or type of payload.
     * @param value_or_instance A valid pointer that fits the purpose.
     */
    template<auto Candidate, typename Type>
    void connect(Type *value_or_instance) {
        if constexpr(std::is_member_function_pointer_v<decltype(Candidate)>) {
            disconnect<Candidate>(value_or_instance);
        } else {
            disconnect<Candidate>();
        }

        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Candidate>(value_or_instance);
        calls->emplace_back(std::move(delegate));
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<auto Function>
    void disconnect() {
        delegate<Ret(Args...)> delegate{};

        if constexpr(std::is_invocable_r_v<Ret, decltype(Function), Args...>) {
            delegate.template connect<Function>();
        } else {
            decltype(payload_type(Function)) payload = nullptr;
            delegate.template connect<Function>(payload);
        }

        calls->erase(std::remove(calls->begin(), calls->end(), std::move(delegate)), calls->end());
    }

    /**
     * @brief Disconnects a given member function from a signal.
     * @tparam Member Member function to disconnect from the signal.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<auto Member, typename Class>
    void disconnect(Class *instance) {
        static_assert(std::is_member_function_pointer_v<decltype(Member)>);
        delegate<Ret(Args...)> delegate{};
        delegate.template connect<Member>(instance);
        calls->erase(std::remove_if(calls->begin(), calls->end(), [&delegate](const auto &other) {
            return other == delegate && other.instance() == delegate.instance();
        }), calls->end());
    }

    /**
     * @brief Disconnects all the listeners from a signal.
     */
    void disconnect() {
        calls->clear();
    }

private:
    std::vector<delegate<Ret(Args...)>> *calls;
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
struct sigh<Ret(Args...), Collector>: private internal::invoker<Ret(Args...), Collector> {
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<delegate<Ret(Args...)>>::size_type;
    /*! @brief Collector type. */
    using collector_type = Collector;
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
     * @brief Returns a sink object for the given signal.
     *
     * A sink is an opaque object used to connect listeners to signals.<br/>
     * The function type for a listener is the one of the signal to which it
     * belongs. The order of invocation of the listeners isn't guaranteed.
     *
     * @return A temporary sink object.
     */
    sink_type sink() ENTT_NOEXCEPT {
        return { &calls };
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
            call(args...);
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
            if(!this->invoke(collector, call, args...)) {
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
    friend void swap(sigh &lhs, sigh &rhs) {
        using std::swap;
        swap(lhs.calls, rhs.calls);
    }

private:
    std::vector<delegate<Ret(Args...)>> calls;
};


}


#endif // ENTT_SIGNAL_SIGH_HPP
