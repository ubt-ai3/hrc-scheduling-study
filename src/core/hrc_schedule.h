#pragma once

#include <vector>

#include "operation.h"


namespace core {
class task;

/**
 * @brief Holds the ordered sequence of operations assigned to each actor for one scheduling epoch.
 *
 * A schedule is produced by calling the Python scheduler (via pybind11) on the current
 * task state.  It stores two queues — one for the human, one for the robot — that are
 * consumed as operations are completed.
 *
 * When @p interrupted is true, the scheduler is told that the human was interrupted mid-task
 * so it can bias the next assignment accordingly.  When @p show_schedule is true, the scheduler
 * prints the resulting plan to stdout (useful for debugging).
 */
class schedule
{
  public:
    schedule() = default;
    /**
     * @brief Builds a new schedule by invoking the Python scheduler on @p task.
     * @param schedule_human_actor     Include the human as an available actor.
     * @param schedule_robot_actor     Include the robot as an available actor.
     * @param interrupted              True if a human interruption just ended.
     * @param show_schedule            Print the resulting plan to stdout.
     */
    schedule(const task &task, bool schedule_human_actor, bool schedule_robot_actor, bool interrupted = false, bool show_schedule = false);

    /** @brief Returns the next pending operation for the human, or nullptr if none remain. */
    Operation *current_human_op();
    /** @brief Marks the current human operation as done and advances the human queue. */
    void complete_human_op();

    /** @brief Returns the next pending operation for the robot, or nullptr if none remain. */
    Operation *current_robot_op();
    /** @brief Marks the current robot operation as done and advances the robot queue. */
    void complete_robot_op();

    /** @brief Returns true when both actor queues are empty. */
    bool complete() const;

  private:
    Operation* first_or_null(const std::vector<Operation*>& vec);
    void pop_or_throw(std::vector<Operation*>& vec);

    std::vector<Operation*> human_ops_;
    std::vector<Operation*> robot_ops_;
};

}// namespace core