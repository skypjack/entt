#ifndef ENTT_PROCESS_PROCESS_HPP
#define ENTT_PROCESS_PROCESS_HPP


#include <utility>
#include <type_traits>
#include "../config/config.h"


namespace entt {


/**
 * @brief Base class for processes.
 *
 * This class stays true to the CRTP idiom. Derived classes must specify what's
 * the intended type for elapsed times.<br/>
 * A process should expose publicly the following member functions whether
 * required:
 *
 * * @code{.cpp}
 *   void update(Delta, void *);
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
 *   void init();
 *   @endcode
 *
 *   It's invoked when the process joins the running queue of a scheduler. This
 *   happens as soon as it's attached to the scheduler if the process is a top
 *   level one, otherwise when it replaces its parent if the process is a
 *   continuation.
 *
 * * @code{.cpp}
 *   void succeeded();
 *   @endcode
 *
 *   It's invoked in case of success, immediately after an update and during the
 *   same tick.
 *
 * * @code{.cpp}
 *   void failed();
 *   @endcode
 *
 *   It's invoked in case of errors, immediately after an update and during the
 *   same tick.
 *
 * * @code{.cpp}
 *   void aborted();
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
 * @tparam Derived Actual type of process that extends the class template.
 * @tparam Delta Type to use to provide elapsed time.
 */
template<typename Derived, typename Delta>
class process {
    enum class state: unsigned int {
        UNINITIALIZED = 0,
        RUNNING,
        PAUSED,
        SUCCEEDED,
        FAILED,
        ABORTED,
        FINISHED
    };

    template<state value>
    using state_value_t = std::integral_constant<state, value>;

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::UNINITIALIZED>)
    -> decltype(std::declval<Target>().init()) {
        static_cast<Target *>(this)->init();
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::RUNNING>, Delta delta, void *data)
    -> decltype(std::declval<Target>().update(delta, data)) {
        static_cast<Target *>(this)->update(delta, data);
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::SUCCEEDED>)
    -> decltype(std::declval<Target>().succeeded()) {
        static_cast<Target *>(this)->succeeded();
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::FAILED>)
    -> decltype(std::declval<Target>().failed()) {
        static_cast<Target *>(this)->failed();
    }

    template<typename Target = Derived>
    auto tick(int, state_value_t<state::ABORTED>)
    -> decltype(std::declval<Target>().aborted()) {
        static_cast<Target *>(this)->aborted();
    }

    template<state value, typename... Args>
    void tick(char, state_value_t<value>, Args &&...) const ENTT_NOEXCEPT {}

protected:
    /**
     * @brief Terminates a process with success if it's still alive.
     *
     * The function is idempotent and it does nothing if the process isn't
     * alive.
     */
    void succeed() ENTT_NOEXCEPT {
        if(alive()) {
            current = state::SUCCEEDED;
        }
    }

    /**
     * @brief Terminates a process with errors if it's still alive.
     *
     * The function is idempotent and it does nothing if the process isn't
     * alive.
     */
    void fail() ENTT_NOEXCEPT {
        if(alive()) {
            current = state::FAILED;
        }
    }

    /**
     * @brief Stops a process if it's in a running state.
     *
     * The function is idempotent and it does nothing if the process isn't
     * running.
     */
    void pause() ENTT_NOEXCEPT {
        if(current == state::RUNNING) {
            current = state::PAUSED;
        }
    }

    /**
     * @brief Restarts a process if it's paused.
     *
     * The function is idempotent and it does nothing if the process isn't
     * paused.
     */
    void unpause() ENTT_NOEXCEPT {
        if(current  == state::PAUSED) {
            current  = state::RUNNING;
        }
    }

public:
    /*! @brief Type used to provide elapsed time. */
    using delta_type = Delta;

    /*! @brief Default destructor. */
    virtual ~process() ENTT_NOEXCEPT {
        static_assert(std::is_base_of_v<process, Derived>);
    }

    /**
     * @brief Aborts a process if it's still alive.
     *
     * The function is idempotent and it does nothing if the process isn't
     * alive.
     *
     * @param immediately Requests an immediate operation.
     */
    void abort(const bool immediately = false) ENTT_NOEXCEPT {
        if(alive()) {
            current = state::ABORTED;

            if(immediately) {
                tick({});
            }
        }
    }

    /**
     * @brief Returns true if a process is either running or paused.
     * @return True if the process is still alive, false otherwise.
     */
    bool alive() const ENTT_NOEXCEPT {
        return current == state::RUNNING || current == state::PAUSED;
    }

    /**
     * @brief Returns true if a process is already terminated.
     * @return True if the process is terminated, false otherwise.
     */
    bool dead() const ENTT_NOEXCEPT {
        return current == state::FINISHED;
    }

    /**
     * @brief Returns true if a process is currently paused.
     * @return True if the process is paused, false otherwise.
     */
    bool paused() const ENTT_NOEXCEPT {
        return current == state::PAUSED;
    }

    /**
     * @brief Returns true if a process terminated with errors.
     * @return True if the process terminated with errors, false otherwise.
     */
    bool rejected() const ENTT_NOEXCEPT {
        return stopped;
    }

    /**
     * @brief Updates a process and its internal state if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void tick(const Delta delta, void *data = nullptr) {
        switch (current) {
        case state::UNINITIALIZED:
            tick(0, state_value_t<state::UNINITIALIZED>{});
            current = state::RUNNING;
            break;
        case state::RUNNING:
            tick(0, state_value_t<state::RUNNING>{}, delta, data);
            break;
        default:
            // suppress warnings
            break;
        }

        // if it's dead, it must be notified and removed immediately
        switch(current) {
        case state::SUCCEEDED:
            tick(0, state_value_t<state::SUCCEEDED>{});
            current = state::FINISHED;
            break;
        case state::FAILED:
            tick(0, state_value_t<state::FAILED>{});
            current = state::FINISHED;
            stopped = true;
            break;
        case state::ABORTED:
            tick(0, state_value_t<state::ABORTED>{});
            current = state::FINISHED;
            stopped = true;
            break;
        default:
            // suppress warnings
            break;
        }
    }

private:
    state current{state::UNINITIALIZED};
    bool stopped{false};
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
struct process_adaptor: process<process_adaptor<Func, Delta>, Delta>, private Func {
    /**
     * @brief Constructs a process adaptor from a lambda or a functor.
     * @tparam Args Types of arguments to use to initialize the actual process.
     * @param args Parameters to use to initialize the actual process.
     */
    template<typename... Args>
    process_adaptor(Args &&... args)
        : Func{std::forward<Args>(args)...}
    {}

    /**
     * @brief Updates a process and its internal state if required.
     * @param delta Elapsed time.
     * @param data Optional data.
     */
    void update(const Delta delta, void *data) {
        Func::operator()(delta, data, [this]() { this->succeed(); }, [this]() { this->fail(); });
    }
};


}


#endif // ENTT_PROCESS_PROCESS_HPP
