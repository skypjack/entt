#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <algorithm>
#include <utility>
#include <vector>


namespace entt {

/**
 * @brief Signal handler.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename>
class SigH;


/**
 * @brief Signal handler.
 *
 * Unmanaged signal handler. It works directly with naked pointers to classes
 * and pointers to member functions as well as pointers to free functions. Users
 * of this class are in charge of disconnecting instances before deleting them.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of the arguments of a function type.
 */
template<typename Ret, typename... Args>
class SigH<Ret(Args...)> {
    using proto_type = void(*)(void *, Args...);
    using call_type = std::pair<void *, proto_type>;

    template<Ret(*Function)(Args...)>
    static void proto(void *, Args... args) {
        (Function)(args...);
    }

    template<typename Class, Ret(Class::*Member)(Args...)>
    static void proto(void *instance, Args... args) {
        (static_cast<Class *>(instance)->*Member)(args...);
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<call_type>::size_type;

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
     * @warning
     * The signal doesn't perform checks on multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     */
    template<Ret(*Function)(Args...)>
    void connect() {
        disconnect<Function>();
        calls.emplace_back(call_type{nullptr, &proto<Function>});
    }

    /**
     * @brief Connects the member function for the given instance to the signal.
     *
     * The signal isn't responsible for the connected object. Users must
     * guarantee that the lifetime of the instance overcomes the one of the
     * signal.
     *
     * @warning
     * The signal doesn't perform checks on multiple connections for the same
     * member method of a given instance.
     *
     * @tparam Class The type of the class to which the member function belongs.
     * @tparam Member The member function to connect to the signal.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template <typename Class, Ret(Class::*Member)(Args...)>
    void connect(Class *instance) {
        disconnect<Class, Member>(instance);
        calls.emplace_back(call_type{instance, &proto<Class, Member>});
    }

    /**
     * @brief Disconnects a free function from the signal.
     *
     * If the free function has been connected more than once, all the
     * connections are broken once the function returns.
     *
     * @tparam Function A valid free function pointer.
     */
    template<Ret(*Function)(Args...)>
    void disconnect() {
        call_type target{nullptr, &proto<Function>};
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(target)), calls.end());
    }

    /**
     * @brief Disconnects the member function of the given instance from the
     * signal.
     *
     * If the member function for the given instance has been connected more
     * than once, all the connections are broken once the function returns.
     *
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
     *
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
     * @param args Arguments to use when invoking listeners.
     */
    void publish(Args... args) {
        for(auto &&call: calls) {
            call.second(call.first, args...);
        }
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
     * listeners have been connected to them.
     *
     * @note
     * This function is pretty expensive, use it carefully.
     *
     * @param other Signal with which to compare.
     * @return True if the two signals are identical, false otherwise.
     */
    bool operator==(const SigH &other) const noexcept {
        const auto &ref = other.calls;
        auto pred = [&ref](const auto &call) { return (std::find(ref.cbegin(), ref.cend(), call) != ref.cend()); };
        return (calls.size() == other.calls.size()) && std::all_of(calls.cbegin(), calls.cend(), std::move(pred));
    }

private:
    std::vector<call_type> calls;
};


/**
 * @brief Checks if the contents of the two signals are different.
 *
 * Two signals are identical if they have the same size and the same
 * listeners have been connected to them. They are different otherwiser.
 *
 * @note
 * This function is pretty expensive, use it carefully.
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


}


#endif // ENTT_SIGNAL_SIGH_HPP
