#pragma once

#include <mutex>
#include <thread>
#include <condition_variable>

#include "entity.h"
#include "operation.h"

#include "abstract_robot.h"
#include "world_model.h"

namespace core {
class task;

/**
 * @brief Thread-safe queue that serialises asynchronous robot commands onto a worker thread.
 *
 * All robot actions (exploration, pick-and-place operations) are submitted to
 * robot_handler and executed sequentially on a dedicated background thread so that
 * the Qt event loop (and thus the UI) remains responsive.
 *
 * Subclasses implement the actual motion logic:
 *   - hardware_robot_handler  — drives a real Franka robot via abstract_robot
 *   - simulated_robot_handler — simulates motion with a delay, for UI testing
 *
 * A callback is fired on the worker thread when each command completes; callers
 * (typically task) use this to advance the state machine.
 */
class robot_handler
{
  public:
    robot_handler();
    ~robot_handler();

    /**
     * @brief Enqueues a pick-and-place operation and invokes @p callback when done.
     *
     * Thread-safe.  Only one operation can be in flight at a time; calling this
     * while a previous operation is still running is undefined behaviour.
     */
    virtual void execute_operation(const benchmark::Entity &start,
        const benchmark::Entity &end,
        const std::function<void()> &callback) final;

    /**
     * @brief Starts a workspace exploration sweep and invokes @p callback when done.
     *
     * Thread-safe.  Detected entities are available via exploration_result() after
     * the callback fires.
     */
    virtual void start_exporation(const std::function<void()> &callback) final;

    /** @brief Returns the entities detected during the last exploration. */
    std::vector<benchmark::Entity> expolation_result() const;

  protected:
    virtual std::vector<benchmark::Entity> explore() = 0;
    virtual void handle_op(const benchmark::Entity &start, const benchmark::Entity &end) = 0;

  private:
    mutable std::mutex lock_;
    std::condition_variable cv_;


    bool explore_ = false;
    mutable std::mutex explore_result_lock_;
    std::vector<benchmark::Entity> explored_entities_;
    std::function<void()> exploration_completed_callback_;


    bool open_operation_ = false;
    benchmark::Entity start_;
    benchmark::Entity end_;
    std::function<void()> operation_completed_callback_;


    std::atomic_bool stopped_ = false;
    std::thread worker_;
    void work();
};


/**
 * @brief robot_handler implementation that drives the physical Franka robot.
 *
 * Constructs a franka_robot, runs workspace exploration to build a world model,
 * and executes each pick-and-place operation by looking up the entity pose in the
 * world model and calling abstract_robot::pick / abstract_robot::place.
 */
class hardware_robot_handler : public robot_handler
{
  public:
    hardware_robot_handler(task &task);
    ~hardware_robot_handler() = default;

  protected:
    virtual std::vector<benchmark::Entity> explore() override;
    virtual void handle_op(const benchmark::Entity &start, const benchmark::Entity &end) override;

  private:
    std::vector<benchmark::Entity> teach_entity_positions(std::vector<benchmark::Entity> entities);

    task &task_;
    std::unique_ptr<benchmark::abstract_robot> bot_;
};

/**
 * @brief robot_handler implementation that simulates robot actions without hardware.
 *
 * Returns a fixed set of pre-recorded entity positions for exploration and
 * introduces a short sleep to simulate operation duration.  Used when
 * connect_to_hardware is false.
 */
class simulated_robot_handler : public robot_handler
{
    virtual std::vector<benchmark::Entity> explore() override;
    virtual void handle_op(const benchmark::Entity &start, const benchmark::Entity &end) override;
};
}// namespace core