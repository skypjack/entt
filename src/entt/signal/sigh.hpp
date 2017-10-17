#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <algorithm>
#include <utility>
#include <vector>


namespace entt {


namespace {


template<typename, typename>
struct Invoker;


template<typename Ret, typename... Args, typename Collector>
struct Invoker<Ret(Args...), Collector> {
    using proto_type = Ret(*)(void *, Args...);
    using call_type = std::pair<void *, proto_type>;

    virtual ~Invoker() = default;

    template<typename SFINAE = Ret>
    typename std::enable_if<std::is_void<SFINAE>::value, bool>::type
    invoke(Collector &, proto_type proto, void *instance, Args... args) {
        proto(instance, args...);
        return true;
    }

    template<typename SFINAE = Ret>
    typename std::enable_if<!std::is_void<SFINAE>::value, bool>::type
    invoke(Collector &collector, proto_type proto, void *instance, Args... args) {
        return collector(proto(instance, args...));
    }
};


template<typename Ret>
struct NullCollector final {
    using result_type = Ret;
    bool operator()(result_type) const noexcept { return true; }
};


template<>
struct NullCollector<void> final {
    using result_type = void;
    bool operator()() const noexcept { return true; }
};


template<typename>
struct DefaultCollector;


template<typename Ret, typename... Args>
struct DefaultCollector<Ret(Args...)> final {
    using collector_type = NullCollector<Ret>;
};


template<typename Function>
using DefaultCollectorType = typename DefaultCollector<Function>::collector_type;


}


/**
 * @brief Signal handler.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename Function, typename = DefaultCollectorType<Function>>
class SigH;


/**
 * @brief Signal handler.
 *
 * Unmanaged signal handler. It works directly with naked pointers to classes
 * and pointers to member functions as well as pointers to free functions. Users
 * of this class are in charge of disconnecting instances before deleting them.
 *
 * This class serves mainly two purposes:
 * * Creating signals to be used later to notify a bunch of listeners.
 * * Collecting results from a set of functions like in a voting system.
 *
 * The default collector does nothing. To properly collect data, define and use
 * a class that has a call operator the signature of which is `bool(Param)` and:
 * * `Param` is a type to which `Ret` can be converted.
 * * The return type is true if the handler must stop collecting data, false
 *   otherwise.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of the arguments of a function type.
 * @tparam Collector The type of the collector to use if any.
 */
template<typename Ret, typename... Args, typename Collector>
class SigH<Ret(Args...), Collector> final: private Invoker<Ret(Args...), Collector> {
    using typename Invoker<Ret(Args...), Collector>::call_type;

    template<Ret(*Function)(Args...)>
    static Ret proto(void *, Args... args) {
        return (Function)(args...);
    }

    template<typename Class, Ret(Class::*Member)(Args... args)>
    static Ret proto(void *instance, Args... args) {
        return (static_cast<Class *>(instance)->*Member)(args...);
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<call_type>::size_type;
    /*! @brief Collector type. */
    using collector_type = Collector;

    /*! @brief Default constructor, explicit on purpose. */
    explicit SigH() noexcept = default;

    /*! @brief Default destructor. */
    ~SigH() noexcept = default;

    /**
     * @brief Copy constructor, listeners are also connected to this signal.
     * @param other A signal to be used as source to initialize this instance.
     */
    SigH(const SigH &other)
        : calls{other.calls}
    {}

    /**
     * @brief Default move constructor.
     * @param other A signal to be used as source to initialize this instance.
     */
    SigH(SigH &&other): SigH{} {
        swap(*this, other);
    }

    /**
     * @brief Assignment operator, listeners are also connected to this signal.
     * @param other A signal to be used as source to initialize this instance.
     * @return This signal.
     */
    SigH & operator=(const SigH &other) {
        calls = other.calls;
        return *this;
    }

    /**
     * @brief Default move operator.
     * @param other A signal to be used as source to initialize this instance.
     * @return This signal.
     */
    SigH & operator=(SigH &&other) {
        swap(*this, other);
        return *this;
    }

    /**
     * @brief The number of listeners connected to the signal.
     * @return The number of listeners currently connected.
     */
    size_type size() const noexcept {
        return calls.size();
    }

    /**
     * @brief Returns true is at least a listener is connected to the signal.
     * @return True if the signal has no listeners connected, false otherwise.
     */
    bool empty() const noexcept {
        return calls.empty();
    }

    /**
     * @brief Disconnects all the listeners from the signal.
     */
    void clear() noexcept {
        calls.clear();
    }

    /**
     * @brief Connects a free function to the signal.
     *
     * @note
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     */
    template<Ret(*Function)(Args...)>
    void connect() {
        disconnect<Function>();
        calls.emplace_back(nullptr, &proto<Function>);
    }

    /**
     * @brief Connects the member function for the given instance to the signal.
     *
     * The signal isn't responsible for the connected object. Users must
     * guarantee that the lifetime of the instance overcomes the one of the
     * signal.
     *
     * @warning
     * The signal handler performs checks to avoid multiple connections for the
     * same member function of a given instance.
     *
     * @tparam Class The type of the class to which the member function belongs.
     * @tparam Member The member function to connect to the signal.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template <typename Class, Ret(Class::*Member)(Args...)>
    void connect(Class *instance) {
        disconnect<Class, Member>(instance);
        calls.emplace_back(instance, &proto<Class, Member>);
    }

    /**
     * @brief Disconnects a free function from the signal.
     * @tparam Function A valid free function pointer.
     */
    template<Ret(*Function)(Args...)>
    void disconnect() {
        call_type target{nullptr, &proto<Function>};
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(target)), calls.end());
    }

    /**
     * @brief Disconnects the given member function from the signal.
     * @tparam Class The type of the class to which the member function belongs.
     * @tparam Member The member function to connect to the signal.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class, Ret(Class::*Member)(Args...)>
    void disconnect(Class *instance) {
        call_type target{instance, &proto<Class, Member>};
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(target)), calls.end());
    }

    /**
     * @brief Removes all existing connections for the given instance.
     * @tparam Class The type of the class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class>
    void disconnect(Class *instance) {
        auto func = [instance](const call_type &call) { return call.first == instance; };
        calls.erase(std::remove_if(calls.begin(), calls.end(), std::move(func)), calls.end());
    }

    /**
     * @brief Triggers the signal.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @param args Arguments to use to invoke listeners.
     */
    void publish(Args... args) {
        for(auto &&call: calls) {
            call.second(call.first, args...);
        }
    }

    /**
     * @brief Collects return values from the listeners.
     * @param args Arguments to use to invoke listeners.
     * @return An instance of the collector filled with collected data.
     */
    collector_type collect(Args... args) {
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
    bool operator==(const SigH &other) const noexcept {
        return (calls.size() == other.calls.size()) && std::equal(calls.cbegin(), calls.cend(), other.calls.cbegin());
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
 * @tparam Args Types of the arguments of a function type.
 * @param lhs A valid signal object.
 * @param rhs A valid signal object.
 * @return True if the two signals are different, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const SigH<Ret(Args...)> &lhs, const SigH<Ret(Args...)> &rhs) noexcept {
    return !(lhs == rhs);
}


/**
 * @brief Event handler.
 *
 * Unmanaged event handler. Collecting data for this kind of signals doesn't
 * make sense at all. Its sole purpose is to provide the listeners with the
 * given event.
 */
template<typename Event>
using EventH = SigH<void(const Event &)>;


}


#endif // ENTT_SIGNAL_SIGH_HPP
