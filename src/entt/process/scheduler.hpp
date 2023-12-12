#ifndef ENTT_PROCESS_SCHEDULER_HPP
#define ENTT_PROCESS_SCHEDULER_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/compressed_pair.hpp"
#include "fwd.hpp"
#include "process.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Delta>
struct basic_process_handler {
    virtual ~basic_process_handler() = default;

    virtual bool update(const Delta, void *) = 0;
    virtual void abort(const bool) = 0;

    // std::shared_ptr because of its type erased allocator which is useful here
    std::shared_ptr<basic_process_handler> next;
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
/*! @endcond */

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
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Delta, typename Allocator>
class basic_scheduler {
    template<typename Type>
    using handler_type = internal::process_handler<Delta, Type>;

    // std::shared_ptr because of its type erased allocator which is useful here
    using process_type = std::shared_ptr<internal::basic_process_handler<Delta>>;

    using alloc_traits = std::allocator_traits<Allocator>;
    using container_allocator = typename alloc_traits::template rebind_alloc<process_type>;
    using container_type = std::vector<process_type, container_allocator>;

public:
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Unsigned integer type. */
    using delta_type = Delta;

    /*! @brief Default constructor. */
    basic_scheduler()
        : basic_scheduler{allocator_type{}} {}

    /**
     * @brief Constructs a scheduler with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_scheduler(const allocator_type &allocator)
        : handlers{allocator, allocator} {}

    /**
     * @brief Move constructor.
     * @param other The instance to move from.
     */
    basic_scheduler(basic_scheduler &&other) noexcept
        : handlers{std::move(other.handlers)} {}

    /**
     * @brief Allocator-extended move constructor.
     * @param other The instance to move from.
     * @param allocator The allocator to use.
     */
    basic_scheduler(basic_scheduler &&other, const allocator_type &allocator) noexcept
        : handlers{container_type{std::move(other.handlers.first()), allocator}, allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || handlers.second() == other.handlers.second(), "Copying a scheduler is not allowed");
    }

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This scheduler.
     */
    basic_scheduler &operator=(basic_scheduler &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || handlers.second() == other.handlers.second(), "Copying a scheduler is not allowed");
        handlers = std::move(other.handlers);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given scheduler.
     * @param other Scheduler to exchange the content with.
     */
    void swap(basic_scheduler &other) {
        using std::swap;
        swap(handlers, other.handlers);
    }

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return handlers.second();
    }

    /**
     * @brief Number of processes currently scheduled.
     * @return Number of processes currently scheduled.
     */
    [[nodiscard]] size_type size() const noexcept {
        return handlers.first().size();
    }

    /**
     * @brief Returns true if at least a process is currently scheduled.
     * @return True if there are scheduled processes, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept {
        return handlers.first().empty();
    }

    /**
     * @brief Discards all scheduled processes.
     *
     * Processes aren't aborted. They are discarded along with their children
     * and never executed again.
     */
    void clear() {
        handlers.first().clear();
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
        auto &ref = handlers.first().emplace_back(std::allocate_shared<handler_type<Proc>>(handlers.second(), std::forward<Args>(args)...));
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
        ENTT_ASSERT(!handlers.first().empty(), "Process not available");
        auto *curr = handlers.first().back().get();
        for(; curr->next; curr = curr->next.get()) {}
        curr->next = std::allocate_shared<handler_type<Proc>>(handlers.second(), std::forward<Args>(args)...);
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
        for(auto next = handlers.first().size(); next; --next) {
            if(const auto pos = next - 1u; handlers.first()[pos]->update(delta, data)) {
                // updating might spawn/reallocate, cannot hold refs until here
                if(auto &curr = handlers.first()[pos]; curr->next) {
                    curr = std::move(curr->next);
                    // forces the process to exit the uninitialized state
                    curr->update({}, nullptr);
                } else {
                    curr = std::move(handlers.first().back());
                    handlers.first().pop_back();
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
        for(auto &&curr: handlers.first()) {
            curr->abort(immediate);
        }
    }

private:
    compressed_pair<container_type, allocator_type> handlers;
};

} // namespace entt

#endif
