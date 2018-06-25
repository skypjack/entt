#ifndef ENTT_PROCESS_SCHEDULER_HPP
#define ENTT_PROCESS_SCHEDULER_HPP


#include <vector>
#include <memory>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "process.hpp"


namespace entt {


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
 * }).then<MyProcess>(arguments...);
 * @endcode
 *
 * In order to invoke all scheduled processes, call the `update` member function
 * passing it the elapsed time to forward to the tasks.
 *
 * @sa Process
 *
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Delta>
class Scheduler final {
    struct ProcessHandler final {
        using instance_type = std::unique_ptr<void, void(*)(void *)>;
        using update_fn_type = bool(ProcessHandler &, Delta, void *);
        using abort_fn_type = void(ProcessHandler &, bool);
        using next_type = std::unique_ptr<ProcessHandler>;

        instance_type instance;
        update_fn_type *update;
        abort_fn_type *abort;
        next_type next;
    };

    struct Then final {
        Then(ProcessHandler *handler)
            : handler{handler}
        {}

        template<typename Proc, typename... Args>
        decltype(auto) then(Args &&... args) && {
            static_assert(std::is_base_of<Process<Proc, Delta>, Proc>::value, "!");
            handler = Scheduler::then<Proc>(handler, std::forward<Args>(args)...);
            return std::move(*this);
        }

        template<typename Func>
        decltype(auto) then(Func &&func) && {
            using Proc = ProcessAdaptor<std::decay_t<Func>, Delta>;
            return std::move(*this).template then<Proc>(std::forward<Func>(func));
        }

    private:
        ProcessHandler *handler;
    };

    template<typename Proc>
    static bool update(ProcessHandler &handler, const Delta delta, void *data) {
        auto *process = static_cast<Proc *>(handler.instance.get());
        process->tick(delta, data);

        auto dead = process->dead();

        if(dead) {
            if(handler.next && !process->rejected()) {
                handler = std::move(*handler.next);
                dead = handler.update(handler, delta, data);
            } else {
                handler.instance.reset();
            }
        }

        return dead;
    }

    template<typename Proc>
    static void abort(ProcessHandler &handler, const bool immediately) {
        static_cast<Proc *>(handler.instance.get())->abort(immediately);
    }

    template<typename Proc>
    static void deleter(void *proc) {
        delete static_cast<Proc *>(proc);
    }

    template<typename Proc, typename... Args>
    static auto then(ProcessHandler *handler, Args &&... args) {
        if(handler) {
            auto proc = typename ProcessHandler::instance_type{new Proc{std::forward<Args>(args)...}, &Scheduler::deleter<Proc>};
            handler->next.reset(new ProcessHandler{std::move(proc), &Scheduler::update<Proc>, &Scheduler::abort<Proc>, nullptr});
            handler = handler->next.get();
        }

        return handler;
    }

public:
    /*! @brief Unsigned integer type. */
    using size_type = typename std::vector<ProcessHandler>::size_type;

    /*! @brief Default constructor. */
    Scheduler() ENTT_NOEXCEPT = default;

    /*! @brief Copying a scheduler isn't allowed. */
    Scheduler(const Scheduler &) = delete;
    /*! @brief Default move constructor. */
    Scheduler(Scheduler &&) = default;

    /*! @brief Copying a scheduler isn't allowed. @return This scheduler. */
    Scheduler & operator=(const Scheduler &) = delete;
    /*! @brief Default move assignment operator. @return This scheduler. */
    Scheduler & operator=(Scheduler &&) = default;

    /**
     * @brief Number of processes currently scheduled.
     * @return Number of processes currently scheduled.
     */
    size_type size() const ENTT_NOEXCEPT {
        return handlers.size();
    }

    /**
     * @brief Returns true if at least a process is currently scheduled.
     * @return True if there are scheduled processes, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
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
     * Returned value is an opaque object that can be used to attach a child to
     * the given process. The child is automatically scheduled when the process
     * terminates and only if the process returns with success.
     *
     * Example of use (pseudocode):
     *
     * @code{.cpp}
     * // schedules a task in the form of a process class
     * scheduler.attach<MyProcess>(arguments...)
     * // appends a child in the form of a lambda function
     * .then([](auto delta, void *, auto succeed, auto fail) {
     *     // code
     * })
     * // appends a child in the form of another process class
     * .then<MyOtherProcess>();
     * @endcode
     *
     * @tparam Proc Type of process to schedule.
     * @tparam Args Types of arguments to use to initialize the process.
     * @param args Parameters to use to initialize the process.
     * @return An opaque object to use to concatenate processes.
     */
    template<typename Proc, typename... Args>
    auto attach(Args &&... args) {
        static_assert(std::is_base_of<Process<Proc, Delta>, Proc>::value, "!");

        auto proc = typename ProcessHandler::instance_type{new Proc{std::forward<Args>(args)...}, &Scheduler::deleter<Proc>};
        ProcessHandler handler{std::move(proc), &Scheduler::update<Proc>, &Scheduler::abort<Proc>, nullptr};
        handlers.push_back(std::move(handler));

        return Then{&handlers.back()};
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
     * void(Delta delta, auto succeed, auto fail);
     * @endcode
     *
     * Where:
     *
     * * `delta` is the elapsed time.
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
     * Returned value is an opaque object that can be used to attach a child to
     * the given process. The child is automatically scheduled when the process
     * terminates and only if the process returns with success.
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
     * .then<MyProcess>(arguments...);
     * @endcode
     *
     * @sa ProcessAdaptor
     *
     * @tparam Func Type of process to schedule.
     * @param func Either a lambda or a functor to use as a process.
     * @return An opaque object to use to concatenate processes.
     */
    template<typename Func>
    auto attach(Func &&func) {
        using Proc = ProcessAdaptor<std::decay_t<Func>, Delta>;
        return attach<Proc>(std::forward<Func>(func));
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
    void update(const Delta delta, void *data = nullptr) {
        bool clean = false;

        for(auto pos = handlers.size(); pos; --pos) {
            auto &handler = handlers[pos-1];
            const bool dead = handler.update(handler, delta, data);
            clean = clean || dead;
        }

        if(clean) {
            handlers.erase(std::remove_if(handlers.begin(), handlers.end(), [](auto &handler) {
                return !handler.instance;
            }), handlers.end());
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
     * @param immediately Requests an immediate operation.
     */
    void abort(const bool immediately = false) {
        decltype(handlers) exec;
        exec.swap(handlers);

        std::for_each(exec.begin(), exec.end(), [immediately](auto &handler) {
            handler.abort(handler, immediately);
        });

        std::move(handlers.begin(), handlers.end(), std::back_inserter(exec));
        handlers.swap(exec);
    }

private:
    std::vector<ProcessHandler> handlers{};
};


}


#endif // ENTT_PROCESS_SCHEDULER_HPP
