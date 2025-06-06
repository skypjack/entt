#ifndef ENTT_PROCESS_PROCESS_HPP
#define ENTT_PROCESS_PROCESS_HPP

#include <cstdint>
#include <type_traits>
#include <utility>
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
 *   void init() override;
 *   @endcode
 *
 *   It's invoked when the process joins the running queue of a scheduler. This
 *   happens as soon as it's attached to the scheduler if the process is a top
 *   level one, otherwise when it replaces its parent if the process is a
 *   continuation.
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
class process {
    enum class state : std::uint8_t {
        uninitialized = 0,
        running,
        paused,
        succeeded,
        failed,
        aborted,
        finished,
        rejected
    };

    virtual void update(const Delta, void *) {
        abort(true);
    }

    virtual void init() {}
    virtual void succeeded() {}
    virtual void failed() {}
    virtual void aborted() {}

public:
    /*! @brief Type used to provide elapsed time. */
    using delta_type = Delta;

    /*! @brief Default constructor. */
    constexpr process() = default;

    /*! @brief Default destructor. */
    virtual ~process() = default;

    /*! @brief Default copy constructor. */
    process(const process &) = default;

    /*! @brief Default move constructor. */
    process(process &&) noexcept = default;

    /**
     * @brief Default copy assignment operator.
     * @return This process.
     */
    process &operator=(const process &) = default;

    /**
     * @brief Default move assignment operator.
     * @return This process.
     */
    process &operator=(process &&) noexcept = default;

    /**
     * @brief Aborts a process if it's still alive, otherwise does nothing.
     * @param immediate Requests an immediate operation.
     */
    void abort(const bool immediate = false) {
        if(alive()) {
            current = state::aborted;

            if(immediate) {
                tick({});
            }
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
        if(current == state::running) {
            current = state::paused;
        }
    }

    /*! @brief Restarts a process if it's paused, otherwise does nothing. */
    void unpause() noexcept {
        if(current == state::paused) {
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
     * @brief Updates a process and its internal state if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void tick(const Delta delta, void *data = nullptr) {
        switch(current) {
        case state::uninitialized:
            init();
            current = state::running;
            break;
        case state::running:
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
    state current{state::uninitialized};
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
template<typename Func, typename Delta>
struct process_adaptor: process<Delta>, private Func {
    /**
     * @brief Constructs a process adaptor from a lambda or a functor.
     * @tparam Args Types of arguments to use to initialize the actual process.
     * @param args Parameters to use to initialize the actual process.
     */
    template<typename... Args>
    process_adaptor(Args &&...args)
        : Func{std::forward<Args>(args)...} {}

    /**
     * @brief Updates a process and its internal state if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void update(const Delta delta, void *data) override {
        Func::operator()(
            delta,
            data,
            [this]() { this->succeed(); },
            [this]() { this->fail(); });
    }
};

} // namespace entt

#endif
