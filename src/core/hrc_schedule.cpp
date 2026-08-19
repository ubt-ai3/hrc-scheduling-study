#include <pybind11/embed.h>//include this fist, because of possible defines

#include <spdlog/spdlog.h>
#include "hrc_schedule.h"
#include "task.h"

namespace core {


schedule::schedule(const task &task,
    bool schedule_human_actor,
    bool schedule_robot_actor,
    bool interrupted,
    bool show_schedule)
{
    const int robot_speed = 10;
    const int human_speed = 5;
    const int interrupt_duration = 30;

    human_ops_.clear();
    robot_ops_.clear();

    namespace py = pybind11;
    using namespace pybind11::literals;// to bring in the `_a` literal

    const auto operations = task.operations();
    try {

        // Convert task progess to python
        auto modelling = py::module_::import("task_modelling");

        auto pyTask = modelling.attr("task")();
        std::map<int, py::object> ops;
        py::dict id_from_pyOp;
        py::object start =  pyTask.attr("start_node")();
        py::object end  = pyTask.attr("end_node")();

        int count_open = 0;
        std::stringstream ss;
        ss << "Open operations ";

        // add operations
        for (const auto op : operations) {
            switch (op->state()) {
            // conserve already assigned operations
            case OperationState::HumanNext:
                human_ops_.push_back(op);
                break;
            case OperationState::RobotNext:
                robot_ops_.push_back(op);
                break;
            // schedule open operations
            case OperationState::Open: {
                count_open++;
                ss << op->id() << ",";
                auto pyOp = modelling.attr("operation")();
                ops[op->id()] = pyOp;
                id_from_pyOp[pyOp] = op->id();
                pyTask.attr("add_operation")(ops[op->id()]);
                break;
            }
            // skip closed operations
            default:
                break;
            }
        }
        //ss << std::endl;

        auto ready = [](const Operation *op) {
            for (const auto l : op->left)
                if (l->state() == OperationState::Open) return false;
            return true;
        };
        // add edges
        for (const auto op : operations) {
            if (op->state() == OperationState::RobotNext)
                for (const auto post : op->right)
                    pyTask.attr("add_precedence")(start, ops.at(post->id()));

            if (op->state() != OperationState::Open) continue;

            if (op->reached()) {
                pyTask.attr("add_precedence")(start, ops.at(op->id()));
            }
            
            if (op->right.isEmpty()) {
                pyTask.attr("add_precedence")(ops.at(op->id()), end);
            } else {
                for (const auto post : op->right) {
                    pyTask.attr("add_precedence")(ops.at(op->id()), ops.at(post->id()));
                }
            }
        }


        pyTask.attr("initialize_node_states")();
        // pyTask.attr("draw_task_progress")();


        // generate schedule
        auto scheduling = py::module_::import("list_scheduler");
        auto scheduler = scheduling.attr("list_scheduler")(pyTask);

        if (schedule_human_actor ) {
            py::dict human_tasks;
            for (auto &[id, pyOp] : ops) {
                human_tasks[pyOp] = human_speed;
            }
            py::list availability;// empty list implies available actor

            if (interrupted)// HACK: plan assembly actions without the human
                for (int i = 0; i < interrupt_duration; i++) availability.append(false);

            scheduler.attr("add_agent")(
                "human", human_tasks, "agent_availability"_a = availability);
        }


        if (schedule_robot_actor) {
            py::dict robot_tasks;
            for (auto &[id, pyOp] : ops) {
                robot_tasks[pyOp] = robot_speed;
            }
            scheduler.attr("add_agent")("robot", robot_tasks);
        }


        auto workflow = scheduler.attr("generate_schedule")();
        if (show_schedule)
            workflow.attr("show")();

        ss << "; Human operations: ";
        std::set<int> human_op_ids;
        if (schedule_human_actor) {
            // store operations assigned to the human partner
            for (auto segment : workflow.attr("agent_timelines")["human"]) {
                const auto op_id = id_from_pyOp[segment.attr("operation")].cast<int>();
                for (const auto op : operations)
                    if (op->id() == op_id) {
                        ss << op_id << ", ";
                        human_op_ids.emplace(op_id);
                        human_ops_.push_back(op);
                        break;
                    }
            }
        }

        ss << "; Robot operations: ";
        if (schedule_robot_actor) {
            // store operations assigned to the robot partner
            for (auto segment : workflow.attr("agent_timelines")["robot"]) {
                const auto op_id = id_from_pyOp[segment.attr("operation")].cast<int>();
                if (human_op_ids.count(op_id)) continue; // fix broken scheduler with double allocation by removing those from robot schedule

                for (const auto op : operations)
                    if (op->id() == op_id) {
                        ss << op_id << ", ";
                        robot_ops_.push_back(op);
                        break;
                    }
            }
        }
        //ss << std::endl;
        spdlog::info(ss.str());

        if (count_open > human_ops_.size() + robot_ops_.size())
        {
            spdlog::critical("Not all open operations scheduled");
        }
    } catch (const py::error_already_set &e) {
        std::cerr << e.what() << "\n";

        // Convert to c++ exception, so the underlying python object can be destroyed.
        throw std::runtime_error("Error generating schedule");
    }
}

Operation *schedule::current_human_op() { return first_or_null(human_ops_); }
Operation *schedule::current_robot_op() { return first_or_null(robot_ops_); }

void schedule::complete_human_op() { pop_or_throw(human_ops_); }
void schedule::complete_robot_op() { pop_or_throw(robot_ops_); }

Operation *schedule::first_or_null(const std::vector<Operation *> &vec)
{
    if (!vec.empty()) return vec.front();
    return nullptr;
}

void schedule::pop_or_throw(std::vector<Operation *> &vec)
{
    if (!vec.empty())
        vec.erase(vec.begin());
    else
        throw std::runtime_error("No operation to complete");
}

bool schedule::complete() const { return human_ops_.empty() && robot_ops_.empty(); }


}// namespace core