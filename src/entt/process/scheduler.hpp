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
    using base_type = basic_process<Delta, Allocator>;
    using alloc_traits = std::allocator_traits<Allocator>;
    using container_allocator = typename alloc_traits::template rebind_alloc<std::shared_ptr<base_type>>;
    using container_type = std::vector<std::shared_ptr<base_type>, container_allocator>;

public:
    /*! @brief Process type. */
    using type = base_type;
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

    /*! @brief Default copy constructor, deleted on purpose. */
    basic_scheduler(const basic_scheduler &) = delete;

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
    basic_scheduler(basic_scheduler &&other, const allocator_type &allocator)
        : handlers{container_type{std::move(other.handlers.first()), allocator}, allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a scheduler is not allowed");
    }

    /*! @brief Default destructor. */
    ~basic_scheduler() = default;

    /**
     * @brief Default copy assignment operator, deleted on purpose.
     * @return This process scheduler.
     */
    basic_scheduler &operator=(const basic_scheduler &) = delete;

    /**
     * @brief Move assignment operator.
     * @param other The instance to move from.
     * @return This process scheduler.
     */
    basic_scheduler &operator=(basic_scheduler &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a scheduler is not allowed");
        swap(other);
        return *this;
    }

    /**
     * @brief Exchanges the contents with those of a given scheduler.
     * @param other Scheduler to exchange the content with.
     */
    void swap(basic_scheduler &other) noexcept {
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
     * @tparam Type Type of process to create.
     * @tparam Args Types of arguments to use to initialize the process.
     * @param args Parameters to use to initialize the process.
     * @return A reference to the newly created process.
     */
    template<typename Type, typename... Args>
    type &attach(Args &&...args) {
        const auto &allocator = handlers.second();
        return *handlers.first().emplace_back(std::allocate_shared<Type>(allocator, allocator, std::forward<Args>(args)...));
    }

    /**
     * @brief Schedules a process for the next tick.
     * @tparam Func Type of process to create.
     * @param func Either a lambda or a functor to use as a process.
     * @return A reference to the newly created process.
     */
    template<typename Func>
    type &attach(Func func) {
        const auto &allocator = handlers.second();
        using process_type = internal::process_adaptor<delta_type, Func, allocator_type>;
        return *handlers.first().emplace_back(std::allocate_shared<process_type>(allocator, allocator, std::move(func)));
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
            const auto pos = next - 1u;
            handlers.first()[pos]->tick(delta, data);
            // updating might spawn/reallocate, cannot hold refs until here
            auto &elem = handlers.first()[pos];

            if(elem->finished()) {
                elem = elem->peek();
            }

            if(!elem || elem->rejected()) {
                elem = std::move(handlers.first().back());
                handlers.first().pop_back();
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
            curr->abort();

            if(immediate) {
                curr->tick({});
            }
        }
    }

private:
    compressed_pair<container_type, allocator_type> handlers;
};

} // namespace entt

#endif
