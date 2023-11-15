#ifndef ENTT_PROCESS_SCHEDULER_HPP
#define ENTT_PROCESS_SCHEDULER_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "fwd.hpp"
#include "process.hpp"

namespace entt {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

template<typename Delta>
struct basic_process_handler {
    virtual ~basic_process_handler() = default;

    virtual bool update(const Delta, void *) = 0;
    virtual void abort(const bool) = 0;

    std::unique_ptr<basic_process_handler> next;
};

template<typename Delta, typename Type>
struct process_handler final: basic_process_handler<Delta> {
    template<typename... Args>
    process_handler(Args &&...args)
        : process{std::forward<Args>(args)...} {}

    bool update(const Delta delta, void *data) override {
        if(process.tick(delta, data); process.rejected()) {
            this->next.reset();
        }

        return (process.rejected() || process.finished());
    }

    void abort(const bool immediate) override {
        process.abort(immediate);
    }

    Type process;
};

} // namespace internal

/**
 * Internal details not to be documented.
 * @endcond
 */

/**
 * @brief Cooperative scheduler for processes.
 *
 * A cooperative scheduler runs processes and helps managing their life cycles.
 *
 * Each process is invoked once per tick. If a process terminates, it's
 * removed automatically from the scheduler and it's never invoked again.<br/>
 * A process can also have a child. In this case, the process is replaced with
 * its child when it terminates if it returns with success. In case of errors,
 * both the process and its child are discarded.
 *
 * Example of use (pseudocode):
 *
 * @code{.cpp}
 * scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
 *     // code
 * }).then<my_process>(arguments...);
 * @endcode
 *
 * In order to invoke all scheduled processes, call the `update` member function
 * passing it the elapsed time to forward to the tasks.
 *
 * @sa process
 *
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Delta>
class basic_scheduler {
    template<typename Type>
    using handler_type = internal::process_handler<Delta, Type>;

    using process_type = std::unique_ptr<internal::basic_process_handler<Delta>>;

public:
    /*! @brief Unsigned integer type. */
    using delta_type = Delta;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;

    /*! @brief Default constructor. */
    basic_scheduler()
        : handlers{} {
    }

    /*! @brief Default move constructor. */
    basic_scheduler(basic_scheduler &&) = default;

    /*! @brief Default move assignment operator. @return This scheduler. */
    basic_scheduler &operator=(basic_scheduler &&) = default;

    /**
     * @brief Number of processes currently scheduled.
     * @return Number of processes currently scheduled.
     */
    [[nodiscard]] size_type size() const noexcept {
        return handlers.size();
    }

    /**
     * @brief Returns true if at least a process is currently scheduled.
     * @return True if there are scheduled processes, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return handlers.empty();
    }

    /**
     * @brief Discards all scheduled processes.
     *
     * Processes aren't aborted. They are discarded along with their children
     * and never executed again.
     */
    void clear() {
        handlers.clear();
    }

    /**
     * @brief Schedules a process for the next tick.
     *
     * Returned value can be used to attach a continuation for the last process.
     * The continutation is scheduled automatically when the process terminates
     * and only if the process returns with success.
     *
     * Example of use (pseudocode):
     *
     * @code{.cpp}
     * // schedules a task in the form of a process class
     * scheduler.attach<my_process>(arguments...)
     * // appends a child in the form of a lambda function
     * .then([](auto delta, void *, auto succeed, auto fail) {
     *     // code
     * })
     * // appends a child in the form of another process class
     * .then<my_other_process>();
     * @endcode
     *
     * @tparam Proc Type of process to schedule.
     * @tparam Args Types of arguments to use to initialize the process.
     * @param args Parameters to use to initialize the process.
     * @return This process scheduler.
     */
    template<typename Proc, typename... Args>
    basic_scheduler &attach(Args &&...args) {
        static_assert(std::is_base_of_v<process<Proc, Delta>, Proc>, "Invalid process type");
        auto &ref = handlers.emplace_back(std::make_unique<handler_type<Proc>>(std::forward<Args>(args)...));
        // forces the process to exit the uninitialized state
        ref->update({}, nullptr);
        return *this;
    }

    /**
     * @brief Schedules a process for the next tick.
     *
     * A process can be either a lambda or a functor. The scheduler wraps both
     * of them in a process adaptor internally.<br/>
     * The signature of the function call operator should be equivalent to the
     * following:
     *
     * @code{.cpp}
     * void(Delta delta, void *data, auto succeed, auto fail);
     * @endcode
     *
     * Where:
     *
     * * `delta` is the elapsed time.
     * * `data` is an opaque pointer to user data if any, `nullptr` otherwise.
     * * `succeed` is a function to call when a process terminates with success.
     * * `fail` is a function to call when a process terminates with errors.
     *
     * The signature of the function call operator of both `succeed` and `fail`
     * is equivalent to the following:
     *
     * @code{.cpp}
     * void();
     * @endcode
     *
     * Returned value can be used to attach a continuation for the last process.
     * The continutation is scheduled automatically when the process terminates
     * and only if the process returns with success.
     *
     * Example of use (pseudocode):
     *
     * @code{.cpp}
     * // schedules a task in the form of a lambda function
     * scheduler.attach([](auto delta, void *, auto succeed, auto fail) {
     *     // code
     * })
     * // appends a child in the form of another lambda function
     * .then([](auto delta, void *, auto succeed, auto fail) {
     *     // code
     * })
     * // appends a child in the form of a process class
     * .then<my_process>(arguments...);
     * @endcode
     *
     * @sa process_adaptor
     *
     * @tparam Func Type of process to schedule.
     * @param func Either a lambda or a functor to use as a process.
     * @return This process scheduler.
     */
    template<typename Func>
    basic_scheduler &attach(Func &&func) {
        using Proc = process_adaptor<std::decay_t<Func>, Delta>;
        return attach<Proc>(std::forward<Func>(func));
    }

    /**
     * @brief Sets a process as a continuation of the last scheduled process.
     * @tparam Proc Type of process to use as a continuation.
     * @tparam Args Types of arguments to use to initialize the process.
     * @param args Parameters to use to initialize the process.
     * @return This process scheduler.
     */
    template<typename Proc, typename... Args>
    basic_scheduler &then(Args &&...args) {
        static_assert(std::is_base_of_v<process<Proc, Delta>, Proc>, "Invalid process type");
        ENTT_ASSERT(!handlers.empty(), "Process not available");
        auto *curr = handlers.back().get();
        for(; curr->next; curr = curr->next.get()) {}
        curr->next = std::make_unique<handler_type<Proc>>(std::forward<Args>(args)...);
        return *this;
    }

    /**
     * @brief Sets a process as a continuation of the last scheduled process.
     * @tparam Func Type of process to use as a continuation.
     * @param func Either a lambda or a functor to use as a process.
     * @return This process scheduler.
     */
    template<typename Func>
    basic_scheduler &then(Func &&func) {
        using Proc = process_adaptor<std::decay_t<Func>, Delta>;
        return then<Proc>(std::forward<Func>(func));
    }

    /**
     * @brief Updates all scheduled processes.
     *
     * All scheduled processes are executed in no specific order.<br/>
     * If a process terminates with success, it's replaced with its child, if
     * any. Otherwise, if a process terminates with an error, it's removed along
     * with its child.
     *
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void update(const delta_type delta, void *data = nullptr) {
        for(auto next = handlers.size(); next; --next) {
            if(const auto pos = next - 1u; handlers[pos]->update(delta, data)) {
                // updating might spawn/reallocate, cannot hold refs until here
                if(auto &curr = handlers[pos]; curr->next) {
                    curr = std::move(curr->next);
                    // forces the process to exit the uninitialized state
                    curr->update({}, nullptr);
                } else {
                    curr = std::move(handlers.back());
                    handlers.pop_back();
                }
            }
        }
    }

    /**
     * @brief Aborts all scheduled processes.
     *
     * Unless an immediate operation is requested, the abort is scheduled for
     * the next tick. Processes won't be executed anymore in any case.<br/>
     * Once a process is fully aborted and thus finished, it's discarded along
     * with its child, if any.
     *
     * @param immediate Requests an immediate operation.
     */
    void abort(const bool immediate = false) {
        for(auto &&curr: handlers) {
            curr->abort(immediate);
        }
    }

private:
    std::vector<process_type> handlers{};
};

} // namespace entt

#endif
