#ifndef ENTT_PROCESS_PROCESS_HPP
#define ENTT_PROCESS_PROCESS_HPP

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include "../core/compressed_pair.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename, typename, typename>
struct process_adaptor;

} // namespace internal
/*! @endcond */

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
 * `succeed` and `fail` member functions and even pause or unpause the process
 * itself.
 *
 * @sa scheduler
 *
 * @tparam Delta Type to use to provide elapsed time.
 * @tparam Allocator Type of allocator used to manage memory and elements.
 */
template<typename Delta, typename Allocator>
class basic_process: public std::enable_shared_from_this<basic_process<Delta, Allocator>> {
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
    /*! @brief Allocator type. */
    using allocator_type = Allocator;
    /*! @brief Type used to provide elapsed time. */
    using delta_type = Delta;

    /*! @brief Default constructor. */
    basic_process()
        : basic_process{allocator_type{}} {}

    /**
     * @brief Constructs a scheduler with a given allocator.
     * @param allocator The allocator to use.
     */
    explicit basic_process(const allocator_type &allocator)
        : next{nullptr, allocator},
          current{state::idle} {}

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_process(const basic_process &) = delete;

    /*! @brief Default move constructor, deleted on purpose. */
    basic_process(basic_process &&) = delete;

    /*! @brief Default destructor. */
    virtual ~basic_process() = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This process scheduler.
     */
    basic_process &operator=(const basic_process &) = delete;

    /**
     * @brief Default move assignment operator, deleted on purpose.
     * @return This process scheduler.
     */
    basic_process &operator=(basic_process &&) = delete;

    /**
     * @brief Returns the associated allocator.
     * @return The associated allocator.
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return next.second();
    }

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
     * @tparam Type Type of child process to create.
     * @tparam Args Types of arguments to use to initialize the child process.
     * @param args Parameters to use to initialize the child process.
     * @return A reference to the newly created child process.
     */
    template<typename Type, typename... Args>
    basic_process &then(Args &&...args) {
        const auto &allocator = next.second();
        return *(next.first() = std::allocate_shared<Type>(allocator, allocator, std::forward<Args>(args)...));
    }

    /**
     * @brief Assigns a child process to run in case of success.
     * @tparam Func Type of child process to create.
     * @param func Either a lambda or a functor to use as a child process.
     * @return A reference to the newly created child process.
     */
    template<typename Func>
    basic_process &then(Func func) {
        const auto &allocator = next.second();
        using process_type = internal::process_adaptor<delta_type, Func, allocator_type>;
        return *(next.first() = std::allocate_shared<process_type>(allocator, allocator, std::move(func)));
    }

    /**
     * @brief Returns the child process without releasing ownership, if any.
     * @return The child process attached to the object, if any.
     */
    std::shared_ptr<basic_process> peek() {
        return next.first();
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
    compressed_pair<std::shared_ptr<basic_process>, allocator_type> next;
    state current;
};

/*! @cond TURN_OFF_DOXYGEN */
namespace internal {

template<typename Delta, typename Func, typename Allocator>
struct process_adaptor: public basic_process<Delta, Allocator> {
    using allocator_type = Allocator;
    using base_type = basic_process<Delta, Allocator>;
    using delta_type = typename base_type::delta_type;

    process_adaptor(const allocator_type &allocator, Func proc)
        : base_type{allocator},
          func{std::move(proc)} {}

    void update(const delta_type delta, void *data) override {
        func(*this, delta, data);
    }

private:
    Func func;
};

} // namespace internal
/*! @endcond */

} // namespace entt

#endif
