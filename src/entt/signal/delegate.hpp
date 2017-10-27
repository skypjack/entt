#ifndef ENTT_SIGNAL_DELEGATE_HPP
#define ENTT_SIGNAL_DELEGATE_HPP


#include <utility>


namespace entt {


/**
 * @brief Basic delegate implementation.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error unless the template parameter is a function type.
 */
template<typename>
class Delegate;


/**
 * @brief A delegate class to send around functions and member functions.
 *
 * Unmanaged delegate for function pointers and member functions. Users of this
 * class are in charge of disconnecting instances before deleting them.
 *
 * A delegate can be used as general purpose invoker with no memory overhead for
 * free functions and member functions provided along with an instance on which
 * to invoke them.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 */
template<typename Ret, typename... Args>
class Delegate<Ret(Args...)> final {
    using proto_type = Ret(*)(void *, Args...);
    using stub_type = std::pair<void *, proto_type>;

    static Ret fallback(void *, Args...) noexcept { return {}; }

    template<Ret(*Function)(Args...)>
    static Ret proto(void *, Args... args) {
        return (Function)(args...);
    }

    template<typename Class, Ret(Class::*Member)(Args...)>
    static Ret proto(void *instance, Args... args) {
        return (static_cast<Class *>(instance)->*Member)(args...);
    }

public:
    /*! @brief Default constructor, explicit on purpose. */
    explicit Delegate() noexcept
        : stub{std::make_pair(nullptr, &fallback)}
    {}

    /**
     * @brief Binds a free function to a delegate.
     * @tparam Function A valid free function pointer.
     */
    template<Ret(*Function)(Args...)>
    void connect() noexcept {
        stub = std::make_pair(nullptr, &proto<Function>);
    }

    /**
     * @brief Connects a member function for a given instance to a delegate.
     *
     * The delegate isn't responsible for the connected object. Users must
     * guarantee that the lifetime of the instance overcomes the one of the
     * delegate.
     *
     * @tparam Class Type of class to which the member function belongs.
     * @tparam Member Member function to connect to the delegate.
     * @param instance A valid instance of type pointer to `Class`.
     */
    template<typename Class, Ret(Class::*Member)(Args...)>
    void connect(Class *instance) noexcept {
        stub = std::make_pair(instance, &proto<Class, Member>);
    }

    /**
     * @brief Resets a delegate.
     *
     * After a reset, a delegate can be safely invoked with no effect.
     */
    void reset() noexcept {
        stub = std::make_pair(nullptr, &fallback);
    }

    /**
     * @brief Triggers a delegate.
     * @param args Arguments to use to invoke the underlying function.
     * @return The value returned by the underlying function.
     */
    Ret operator()(Args... args) {
        return stub.second(stub.first, args...);
    }

    /**
     * @brief Checks if the contents of the two delegates are different.
     *
     * Two delegates are identical if they contain the same listener.
     *
     * @param other Delegate with which to compare.
     * @return True if the two delegates are identical, false otherwise.
     */
    bool operator==(const Delegate<Ret(Args...)> &other) const noexcept {
        return stub.first == other.stub.first && stub.second == other.stub.second;
    }

private:
    stub_type stub;
};


/**
 * @brief Checks if the contents of the two delegates are different.
 *
 * Two delegates are identical if they contain the same listener.
 *
 * @tparam Ret Return type of a function type.
 * @tparam Args Types of arguments of a function type.
 * @param lhs A valid delegate object.
 * @param rhs A valid delegate object.
 * @return True if the two delegates are different, false otherwise.
 */
template<typename Ret, typename... Args>
bool operator!=(const Delegate<Ret(Args...)> &lhs, const Delegate<Ret(Args...)> &rhs) noexcept {
    return !(lhs == rhs);
}


}


#endif // ENTT_SIGNAL_DELEGATE_HPP
