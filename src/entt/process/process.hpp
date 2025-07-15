#ifndef ENTT_PROCESS_PROCESS_HPP
#define ENTT_PROCESS_PROCESS_HPP

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {

/**
 * @brief Base class for processes.
 *
 * Derived classes must specify what's the intended type for elapsed times.<br/>
 * A process can implement the following member functions whether required:
 *
 * * @code{.cpp}
 *   void update(Delta, void *) override;
 *   @endcode
 *
 *   It's invoked once per tick until a process is explicitly aborted or it
 *   terminates either with or without errors. Even though it's not mandatory to
 *   declare this member function, as a rule of thumb each process should at
 *   least define it to work properly. The `void *` parameter is an opaque
 *   pointer to user data (if any) forwarded directly to the process during an
 *   update.
 *
 * * @code{.cpp}
 *   void succeeded() override;
 *   @endcode
 *
 *   It's invoked in case of success, immediately after an update and during the
 *   same tick.
 *
 * * @code{.cpp}
 *   void failed() override;
 *   @endcode
 *
 *   It's invoked in case of errors, immediately after an update and during the
 *   same tick.
 *
 * * @code{.cpp}
 *   void aborted() override;
 *   @endcode
 *
 *   It's invoked only if a process is explicitly aborted. There is no guarantee
 *   that it executes in the same tick, this depends solely on whether the
 *   process is aborted immediately or not.
 *
 * Derived classes can change the internal state of a process by invoking the
 * `succeed` and `fail` protected member functions and even pause or unpause the
 * process itself.
 *
 * @sa scheduler
 *
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Delta>
class basic_process: public std::enable_shared_from_this<basic_process<Delta>> {
    enum class state : std::uint8_t {
        idle = 0,
        running,
        paused,
        succeeded,
        failed,
        aborted,
        finished,
        rejected
    };

    virtual void update(const Delta, void *) {
        abort();
    }

    virtual void succeeded() {}
    virtual void failed() {}
    virtual void aborted() {}

public:
    /*! @brief Type used to provide elapsed time. */
    using delta_type = Delta;

    /*! @brief Default constructor. */
    constexpr basic_process()
        : next{},
          current{state::idle} {}

    /*! @brief Default destructor. */
    virtual ~basic_process() = default;

    /*! @brief Default copy constructor. */
    basic_process(const basic_process &) = default;

    /*! @brief Default move constructor. */
    basic_process(basic_process &&) noexcept = default;

    /**
     * @brief Default copy assignment operator.
     * @return This process.
     */
    basic_process &operator=(const basic_process &) = default;

    /**
     * @brief Default move assignment operator.
     * @return This process.
     */
    basic_process &operator=(basic_process &&) noexcept = default;

    /*! @brief Aborts a process if it's still alive, otherwise does nothing. */
    void abort() {
        if(alive()) {
            current = state::aborted;
        }
    }

    /**
     * @brief Terminates a process with success if it's still alive, otherwise
     * does nothing.
     */
    void succeed() noexcept {
        if(alive()) {
            current = state::succeeded;
        }
    }

    /**
     * @brief Terminates a process with errors if it's still alive, otherwise
     * does nothing.
     */
    void fail() noexcept {
        if(alive()) {
            current = state::failed;
        }
    }

    /*! @brief Stops a process if it's running, otherwise does nothing. */
    void pause() noexcept {
        if(alive()) {
            current = state::paused;
        }
    }

    /*! @brief Restarts a process if it's paused, otherwise does nothing. */
    void unpause() noexcept {
        if(alive()) {
            current = state::running;
        }
    }

    /**
     * @brief Returns true if a process is either running or paused.
     * @return True if the process is still alive, false otherwise.
     */
    [[nodiscard]] bool alive() const noexcept {
        return current == state::running || current == state::paused;
    }

    /**
     * @brief Returns true if a process is already terminated.
     * @return True if the process is terminated, false otherwise.
     */
    [[nodiscard]] bool finished() const noexcept {
        return current == state::finished;
    }

    /**
     * @brief Returns true if a process is currently paused.
     * @return True if the process is paused, false otherwise.
     */
    [[nodiscard]] bool paused() const noexcept {
        return current == state::paused;
    }

    /**
     * @brief Returns true if a process terminated with errors.
     * @return True if the process terminated with errors, false otherwise.
     */
    [[nodiscard]] bool rejected() const noexcept {
        return current == state::rejected;
    }

    /**
     * @brief Assigns a child process to run in case of success.
     * @param child A child process to run in case of success.
     * @return The newly assigned child process.
     */
    std::shared_ptr<basic_process> then(std::shared_ptr<basic_process> child) {
        next = std::move(child);
        return this->shared_from_this();
    }

    /**
     * @brief Returns the child process without releasing ownership, if any.
     * @return The child process attached to the object, if any.
     */
    std::shared_ptr<basic_process> peek() {
        return next;
    }

    /**
     * @brief Updates a process and its internal state, if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void tick(const Delta delta, void *data = nullptr) {
        switch(current) {
        case state::idle:
        case state::running:
            current = state::running;
            update(delta, data);
            break;
        default:
            // suppress warnings
            break;
        }

        // if it's dead, it must be notified and removed immediately
        switch(current) {
        case state::succeeded:
            succeeded();
            current = state::finished;
            break;
        case state::failed:
            failed();
            current = state::rejected;
            break;
        case state::aborted:
            aborted();
            current = state::rejected;
            break;
        default:
            // suppress warnings
            break;
        }
    }

private:
    std::shared_ptr<basic_process> next;
    state current;
};

/**
 * @brief Adaptor for lambdas and functors to turn them into processes.
 *
 * Lambdas and functors can't be used directly with a scheduler for they are not
 * properly defined processes with managed life cycles.<br/>
 * This class helps in filling the gap and turning lambdas and functors into
 * full featured processes usable by a scheduler.
 *
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
 * Usually users shouldn't worry about creating adaptors. A scheduler will
 * create them internally each and avery time a lambda or a functor is used as
 * a process.
 *
 * @sa process
 * @sa scheduler
 *
 * @tparam Func Actual type of process.
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Delta, typename Func>
struct process_adaptor: public basic_process<Delta> {
    /*! @brief Type used to provide elapsed time. */
    using delta_type = typename basic_process<Delta>::delta_type;

    /**
     * @brief Constructs a process adaptor from a lambda or a functor.
     * @param proc Actual process to use under the hood.
     */
    process_adaptor(Func proc)
        : basic_process<Delta>{},
          func{std::move(proc)} {}

    /**
     * @brief Updates a process and its internal state if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void update(const delta_type delta, void *data) override {
        func(delta, data, *this);
    }

private:
    Func func;
};

/**
 * @brief Deduction guide.
 * @tparam Func Actual type of process.
 */
template<typename Func>
process_adaptor(Func) -> process_adaptor<nth_argument_t<0u, Func>, Func>;

} // namespace entt

#endif
