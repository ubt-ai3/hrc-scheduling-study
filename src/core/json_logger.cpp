#include "json_logger.h"

#include <fstream>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "task.h"


namespace core {

json_logger::json_logger(QObject *parent) : QObject(parent) {}


void json_logger::bind_to_operations(const QVector<Operation *> &operations)
{
    for (const auto &op : operations) {
        QObject::connect(op, &Operation::stateChanged, this, [op, this] { opStateChanged(op); });
    }
}

void json_logger::bind_to_taskState(task *t)
{
    QObject::connect(t, &task::stateChanged, this, [t, this] { taskStateChanged(t); });
}

void json_logger::opStateChanged(const Operation *op)
{
    using namespace std::chrono;

    const auto state = op->state();
    if (state == core::OperationState::HumanNext) {
        start_times_[op] = std::chrono::steady_clock::now();
        assigned_actors_[op] = "Human";
    } else if (state == core::OperationState::RobotNext) {
        start_times_[op] = std::chrono::steady_clock::now();
        assigned_actors_[op] = "Robot";
    } else if (state == core::OperationState::Closed) {
        nlohmann::json j;
        j["Completion"] = { { "Actor", assigned_actors_[op] },
            { "Duration",
                duration_cast<milliseconds>(steady_clock::now() - start_times_[op]).count() },
            { "Operation", *op } };

        write(j);
    }
}

void json_logger::taskStateChanged(const task *t)
{
    std::cout << "TaskState: " << t->state << "\n";
    previous_task_state_ = current_task_state_;
    current_task_state_ = t->state;

    using namespace std::chrono;


    if (current_task_state_ == TaskState::Interrupted) {
        interruption_start_ = steady_clock::now();
        nlohmann::json j("Interrupt Started");
        write(j);

    } else if (current_task_state_ == TaskState::Running
               && previous_task_state_ == TaskState::NotStarted) {
        task_start_ = steady_clock::now();
    } else if (current_task_state_ == TaskState::Done) {
        nlohmann::json j;
        j["Task Completion"] = { { "Duration",
            duration_cast<milliseconds>(steady_clock::now() - task_start_).count() } };
        write(j);
    }
}

void json_logger::log_return_from_interrupt()
{
    using namespace std::chrono;
    nlohmann::json j;
    j["Interrupt Ended"] = { { "Duration",
        duration_cast<milliseconds>(steady_clock::now() - interruption_start_).count() } };
    write(j);
}


void json_logger::write(const nlohmann::json &j)
{
    try {
        spdlog::get("data_logger")->info(j.dump());
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
    }
}
}// namespace core