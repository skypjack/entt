#ifndef ENTT_SIGNAL_SIGNAL_HPP
#define ENTT_SIGNAL_SIGNAL_HPP


#include <memory>
#include <vector>
#include <utility>
#include <cstdint>
#include <iterator>
#include <algorithm>


namespace entt {


/**
 * @brief Managed signal handler declaration.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename>
class Signal;


/**
 * @brief Managed signal handler definition.
 *
 * Managed signal handler. It works with weak pointers to classes and pointers
 * to member functions as well as pointers to free functions. References are
 * automatically removed when the instances to which they point are freed.
 *
 * This class can be used to create signals used later to notify a bunch of
 * listeners.
 *
 * @tparam Args Types of arguments of a function type.
 */
template<typename... Args>
class Signal<void(Args...)> final {
    using proto_type = bool(*)(std::weak_ptr<void> &, Args...);
    using call_type = std::pair<std::weak_ptr<void>, proto_type>;

    template<void(*Function)(Args...)>
    static bool proto(std::weak_ptr<void> &, Args... args) {
        Function(args...);
        return true;
    }

    template<typename Class, void(Class::*Member)(Args...)>
    static bool proto(std::weak_ptr<void> &wptr, Args... args) {
        bool ret = false;

        if(!wptr.expired()) {
            auto ptr = std::static_pointer_cast<Class>(wptr.lock());
            (ptr.get()->*Member)(args...);
            ret = true;
        }

        return ret;
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /**
     * @brief Instance type when it comes to connecting member functions.
     * @tparam Class Type of class to which the member function belongs.
     */
    template<typename Class>
    using instance_type = std::shared_ptr<Class>;

    /**
     * @brief Number of listeners connected to the signal.
     * @return Number of listeners currently connected.
     */
    size_type size() const noexcept {
        return calls.size();
    }

    /**
     * @brief Returns false if at least a listener is connected to the signal.
     * @return True if the signal has no listeners connected, false otherwise.
     */
    bool empty() const noexcept {
        return calls.empty();
    }

    /**
     * @brief Disconnects all the listeners from a signal.
     */
    void clear() noexcept {
        calls.clear();
    }

    /**
     * @brief Connects a free function to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for free
     * functions.
     *
     * @tparam Function A valid free function pointer.
     */
    template<void(*Function)(Args...)>
    void connect() {
        disconnect<Function>();
        calls.emplace_back(std::weak_ptr<void>{}, &proto<Function>);
    }

    /**
     * @brief Connects a member function for a given instance to a signal.
     *
     * The signal handler performs checks to avoid multiple connections for the
     * same member function of a given instance.
     *
     * @tparam Class Type of class to which the member function belongs.
     * @tparam Member Member function to connect to the signal.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class, void(Class::*Member)(Args...)>
    void connect(instance_type<Class> instance) {
        disconnect<Class, Member>(instance);
        calls.emplace_back(std::move(instance), &proto<Class, Member>);
    }

    /**
     * @brief Disconnects a free function from a signal.
     * @tparam Function A valid free function pointer.
     */
    template<void(*Function)(Args...)>
    void disconnect() {
        calls.erase(std::remove_if(calls.begin(), calls.end(),
            [](const call_type &call) { return call.second == &proto<Function> && !call.first.lock(); }
        ), calls.end());
    }

    /**
     * @brief Disconnects the given member function from a signal.
     * @tparam Class Type of class to which the member function belongs.
     * @tparam Member Member function to connect to the signal.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class, void(Class::*Member)(Args...)>
    void disconnect(instance_type<Class> instance) {
        calls.erase(std::remove_if(calls.begin(), calls.end(),
            [instance{std::move(instance)}](const call_type &call) { return call.second == &proto<Class, Member> && call.first.lock() == instance; }
        ), calls.end());
    }

    /**
     * @brief Removes all existing connections for the given instance.
     * @tparam Class Type of class to which the member function belongs.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class>
    void disconnect(instance_type<Class> instance) {
        calls.erase(std::remove_if(calls.begin(), calls.end(),
            [instance{std::move(instance)}](const call_type &call) { return call.first.lock() == instance; }
        ), calls.end());
    }

    /**
     * @brief Triggers a signal.
     *
     * All the listeners are notified. Order isn't guaranteed.
     *
     * @param args Arguments to use to invoke listeners.
     */
    void publish(Args... args) {
        std::vector<call_type> next;

        for(auto pos = calls.size(); pos; --pos) {
            auto &call = calls[pos-1];

            if((call.second)(call.first, args...)) {
                next.push_back(call);
            }
        }

        calls.swap(next);
    }

    /**
     * @brief Swaps listeners between the two signals.
     * @param lhs A valid signal object.
     * @param rhs A valid signal object.
     */
    friend void swap(Signal &lhs, Signal &rhs) {
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
    bool operator==(const Signal &other) const noexcept {
        return std::equal(calls.cbegin(), calls.cend(), other.calls.cbegin(), other.calls.cend(), [](const auto &lhs, const auto &rhs) {
            return (lhs.second == rhs.second) && (lhs.first.lock() == rhs.first.lock());
        });
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
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid signal object.
 * @param rhs A valid signal object.
 * @return True if the two signals are different, false otherwise.
 */
template<typename... Args>
bool operator!=(const Signal<void(Args...)> &lhs, const Signal<void(Args...)> &rhs) noexcept {
    return !(lhs == rhs);
}


}


#endif // ENTT_SIGNAL_SIGNAL_HPP
