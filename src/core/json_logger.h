#pragma once

#include <chrono>

#include <QObject>
#include <QtQml/qqml.h>
#include <nlohmann/json.hpp>

#include "enums.h"

namespace core {
class task;
class Operation;

/**
 * @brief Study data logger that writes timestamped JSONL records for each experiment event.
 *
 * Connect it to a task and its operations after loading a task description.  The logger
 * observes Operation::stateChanged and task::stateChanged signals and appends one JSON
 * object per event to the spdlog "data_logger" sink (a rotating file in the working directory).
 *
 * Recorded events:
 *   - Operation assigned to human or robot (HumanNext / RobotNext): starts timer.
 *   - Operation completed (Closed): logs actor and duration in ms.
 *   - Task interrupted (Interrupted): logs start of secondary task.
 *   - Human returned from interrupt: logs interruption duration in ms.
 *   - Task completed (Done): logs total task duration in ms.
 */
class json_logger : public QObject
{
  public:
    json_logger(QObject *parent = nullptr);

    /** @brief Connects to all operations' stateChanged signals to log per-operation events. */
    void bind_to_operations(const QVector<Operation *> &operations);
    /** @brief Connects to the task's stateChanged signal to log task-level events. */
    void bind_to_taskState(task *t);
    /** @brief Logs the end of an interruption and its duration; call when the human presses "I am back". */
    void log_return_from_interrupt();

  private:
    static void write(const nlohmann::json &j);
    void opStateChanged(const Operation *op);
    void taskStateChanged(const task *t);

    std::map<const core::Operation *, std::chrono::time_point<std::chrono::steady_clock>>
        start_times_;
    std::map<const core::Operation *, std::string> assigned_actors_;
    std::chrono::time_point<std::chrono::steady_clock> interruption_start_ = {};
    std::chrono::time_point<std::chrono::steady_clock> task_start_ = {};


    TaskState::task_state current_task_state_ = TaskState::NotStarted;
    TaskState::task_state previous_task_state_ = TaskState::NotStarted;
};

}// namespace core