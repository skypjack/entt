#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP


#include <algorithm>
#include <utility>
#include <vector>


namespace entt {


template<typename>
class SigH;


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
    using size_type = typename std::vector<call_type>::size_type;

    explicit SigH()
        : calls{}
    {}

    SigH(const SigH &other)
        : calls{other.calls}
    {}

    SigH(SigH &&other): SigH{} {
        swap(*this, other);
    }

    SigH & operator=(const SigH &other) {
        calls = other.calls;
        return *this;
    }

    SigH & operator=(SigH &&other) {
        swap(*this, other);
        return *this;
    }

    size_type size() const noexcept {
        return calls.size();
    }

    bool empty() const noexcept {
        return calls.empty();
    }

    void clear() noexcept {
        calls.clear();
    }

    template<Ret(*Function)(Args...)>
    void connect() {
        disconnect<Function>();
        calls.emplace_back(call_type{nullptr, &proto<Function>});
    }

    template <typename Class, Ret(Class::*Member)(Args...)>
    void connect(Class *instance) {
        disconnect<Class, Member>(instance);
        calls.emplace_back(call_type{instance, &proto<Class, Member>});
    }

    template<Ret(*Function)(Args...)>
    void disconnect() {
        call_type target{nullptr, &proto<Function>};
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(target)), calls.end());
    }

    template<typename Class, Ret(Class::*Member)(Args...)>
    void disconnect(Class *instance) {
        call_type target{instance, &proto<Class, Member>};
        calls.erase(std::remove(calls.begin(), calls.end(), std::move(target)), calls.end());
    }

    template<typename Class>
    void disconnect(Class *instance) {
        auto func = [instance](const call_type &call) { return call.first == instance; };
        calls.erase(std::remove_if(calls.begin(), calls.end(), std::move(func)), calls.end());
    }

    void publish(Args... args) {
        for(auto &&call: calls) {
            call.second(call.first, args...);
        }
    }

    friend void swap(SigH &lhs, SigH &rhs) {
        using std::swap;
        swap(lhs.calls, rhs.calls);
    }

    bool operator==(const SigH &other) const noexcept {
        return calls.size() == other.calls.size() && std::equal(calls.cbegin(), calls.cend(), other.calls.cbegin());
    }

private:
    std::vector<call_type> calls;
};


template<typename Ret, typename... Args>
bool operator!=(const SigH<Ret(Args...)> &lhs, const SigH<Ret(Args...)> &rhs) noexcept {
    return !(lhs == rhs);
}


}


#endif // ENTT_SIGNAL_SIGH_HPP
