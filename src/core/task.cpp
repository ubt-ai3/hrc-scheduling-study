#include <pybind11/embed.h>

#include "task.h"

#include <iostream>
#include <fstream>
#include <string>


#include <QErrorMessage>
#include <QUrl>
#include <QMessageBox>

#include <nlohmann/json.hpp>
#include <pcl/visualization/pcl_visualizer.h>
#include <spdlog/spdlog.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "vision/visualization.h"

#include "json_logger.h"
#include "serialization.h"


namespace core {

task::task(QObject *parent) : QObject(parent)
{
    state = TaskState::NotStarted;
    logger_.bind_to_taskState(this);
}


void task::load_json(const QUrl &task_description_path, const QUrl &config_file_path)
{
    task_description_ = task_description_path;
    config_file_ = config_file_path;
    reset();
}


void task::reset()
{
    using namespace std::literals;

    auto string_path = task_description_.toLocalFile().toStdString();
    if (!config_file_.isEmpty()) {
        string_path =
            config_file_.adjusted(QUrl::RemoveFilename).toLocalFile().toStdString() + string_path;
    }

    spdlog::trace("Loading task description from "s + string_path);

    std::ifstream file(string_path);
    if (!file.is_open()) {
        throw std::runtime_error("could not read task");
    }
    auto json = nlohmann::json();
    file >> json;

    auto new_plates = QVector<core::Baseplate *>();
    for (const auto &element : json.at("scene_elements")) {
        new_plates.push_back(new core::Baseplate(this));
        element.get_to(*new_plates.back());
    }
    set_baseplates(new_plates);


    auto new_ops = std::map<int, core::Operation *>();
    for (auto it = json.at("nodes").begin() + 2; it != json.at("nodes").end(); it++) {
        auto op = new Operation(this);
        it->get_to(*op);
        new_ops[op->id()] = op;
    }

    for (const auto &e : json.at("edges")) {
        const auto start = e.at("from_node_id").get<int>();
        const auto end = e.at("to_node_id").get<int>();
        if (start > 1 && end > 1) {
            new_ops[start]->right.push_back(new_ops[end]);
            new_ops[end]->left.push_back(new_ops[start]);
        }
    }
    set_operations(new_ops);

    spdlog::info("Reseting task");
    reschedule();

    state = TaskState::NotStarted;
    emit stateChanged();
    spdlog::get("data_logger")->warn("\"Task progress reset\"");
}

QVector<core::Baseplate *> task::baseplates() const { return baseplates_; }

void task::set_baseplates(const QVector<core::Baseplate *> &baseplates)
{
    for (auto &plate : baseplates_) {
        delete plate;
    }

    baseplates_ = baseplates;
    emit baseplatesChanged();
}


QVector<core::Operation *> task::operations() const { return operations_; }

void task::set_operations(const QVector<core::Operation *> &operations)
{
    operations_completed_by_human_ = 0;
    for (auto &op : operations_) {
        delete op;
    }
    operations_ = operations;
    logger_.bind_to_operations(operations_);
    emit operationsChanged();
}

void task::human_op_completed()
{
    std::lock_guard lk(schedule_lock_);
    auto curr_op = schedule_.current_human_op();
    if (!curr_op) throw std::logic_error("completed non-existing human operation");

    curr_op->set_state(OperationState::Closed);
    schedule_.complete_human_op();

    if (settings_->interrupt_human_after.contains(++operations_completed_by_human_)) {
        state = TaskState::Interrupted;
        emit stateChanged();
        if (settings_->reschedule_after_interrupt) reschedule(true);
    }

    assign_next_operations();
}


void task::human_returned_from_interrupt()
{
    logger_.log_return_from_interrupt();
    // the robot finished the task while the humand was gone
    if (state == TaskState::Done) return;

    state = TaskState::Running;
    emit stateChanged();
}

void task::start()
{
    using namespace std::literals;
    using namespace benchmark;

    if (state != TaskState::NotStarted) reset();
    state = TaskState::Running;
    emit stateChanged();

    if (!load_exploration()) {
        spdlog::info("Starting exploration");
        robot_->start_exporation([this]() {
            try {

                if (connect_to_hardware_) {
                    const auto entities = robot_->expolation_result();
                }

                std::lock_guard lk(schedule_lock_);
                assign_next_operations();

            } catch (const std::exception &e) {
                spdlog::error(e.what() + ". Reseting task"s);
                state = TaskState::NotStarted;
                emit stateChanged();
            }
        });
    } else {
        std::lock_guard lk(schedule_lock_);
        assign_next_operations();
    
    }
}

bool task::load_exploration()
{
    try {
        if (config_file_.isEmpty()) return false;
        auto exploration_path =
            config_file_.adjusted(QUrl::RemoveFilename).toLocalFile().toStdString()
            + "found_entities.dat";

        std::ifstream fileStream(exploration_path);
        if (!fileStream.is_open()) {
            spdlog::warn("Failed to load exploration from disk");
            return false;
        }

        auto exploration = std::vector<benchmark::Entity>();
        boost::archive::text_iarchive archive(fileStream);
        // Deserialize the map from the archive
        archive >> exploration;

        const auto corner = map_ops_to_world(exploration);
        snap_ops_to_detected_entities(exploration, corner);

    } catch (const std::exception &e) {
        spdlog::error(fmt::format("Error loading exploration from disk: {}", e.what()));
        return false;
    }
    return true;

}

void task::assign_next_operations()
{
    if (schedule_.complete()) {
        state = TaskState::Done;
        emit stateChanged();
        return;
    }

    auto next_op = schedule_.current_human_op();
    if (next_op && next_op->reached() && next_op->state() == OperationState::Open) {
        next_op->set_state(OperationState::HumanNext);
    }

    auto bot_op = schedule_.current_robot_op();
    if (bot_op && bot_op->reached() && bot_op->state() == OperationState::Open) {
        bot_op->set_state(OperationState::RobotNext);

        if (connect_to_hardware_) {
            const auto [start, end] = worldPose_from_op_.at(bot_op);
            robot_->execute_operation(start, end, [this]() { robot_op_completed(); });
        } else {
            robot_->execute_operation({}, {}, [this]() { robot_op_completed(); });
        }
    }
}

void task::set_operations(const std::map<int, core::Operation *> &operations)
{
    QVector<Operation *> new_operations;
    for (auto &new_op : operations) new_operations.push_back(new_op.second);
    set_operations(new_operations);
}

void task::set_settings(settings *new_settings)
{
    if (settings_ == new_settings) return;
    settings_ = new_settings;

    QObject::connect(settings_, &settings::completedWrite, this, [this] {
        try {
            load_json(settings_->task_description(), settings_->config_file_path);
        } catch (const std::exception &e) {
            spdlog::error(e.what());
            state = TaskState::NotStarted;
            emit stateChanged();
        }
    });
}

bool task::connect_to_hardware() const { return connect_to_hardware_; }
void task::set_connect_to_hardware(bool connect)
{
    connect_to_hardware_ = connect;
    if (connect) {
        spdlog::info("Connecting to hardware");
        robot_ = std::make_unique<hardware_robot_handler>(*this);
    } else {
        spdlog::info("Simulating robot");
        robot_ = std::make_unique<simulated_robot_handler>();
    }
}

void task::robot_op_completed()
{
    std::lock_guard lk(schedule_lock_);

    if (auto curr_op = schedule_.current_robot_op()) {
        curr_op->set_state(OperationState::Closed);
        schedule_.complete_robot_op();
        assign_next_operations();
    }
}
benchmark::Entity task::map_ops_to_world(const std::vector<benchmark::Entity> &explo_result)
{
    {
        std::stringstream ss;
        ss << "Exploration results:\n";
        for (const auto &e : explo_result) ss << "\t" << e << "\n";
        spdlog::trace(ss.str());
    }


    const auto corner =
        std::find_if(explo_result.begin(), explo_result.end(), [](const benchmark::Entity &e) {
            return e.kind_ == benchmark::Entity::kind::corner_marker;
        });
    if (corner == explo_result.end())
        throw std::runtime_error("No corner_marker in exploration result");

    std::stringstream ss;
    ss << "operations in world frame:\n";
    worldPose_from_op_.clear();
    for (auto op : operations_) {
        const auto [s, e] = op->to_world_frame(*corner);
        ss << "\t" << s << "\tExpected start: " << op->start_position().x() << " "
           << op->start_position().y() << " " << op->start_position().z() << "\n ";
        worldPose_from_op_[op] = { s, e };
    }
    spdlog::trace(ss.str());
    // TODO check if calculated start_entites are actually in the exploration result
    return *corner;
}

double angle_z(const Eigen::Affine3f &pose)
{
    const auto angle = std::atan2(pose.matrix()(1, 0), pose.matrix()(0, 0)) + M_PI_2;
    return std::fmod(angle + 2 * M_PI, 2 * M_PI);
}


void update_operation_start(Operation *op,
    const benchmark::Entity &new_start,
    const benchmark::Entity &corner_marker)
{
    const auto cf = core::corner_frame(corner_marker);
    const auto rel_pose = cf.inverse() * new_start.pose_;
    const Eigen::Affine3f pose_f = rel_pose.cast<float>();

    const auto t = pose_f.translation();
    op->set_start_position({ t.x(), t.y(), t.z() });
    op->set_start_rotation(angle_z(pose_f));

    spdlog::trace(fmt::format("Setting op translation to {}, {},{}", t.x(), t.y(), t.z()));
}

void task::snap_ops_to_detected_entities(std::vector<benchmark::Entity> explo_result,
    const benchmark::Entity &corner)
{
    auto dist = [](const benchmark::Entity &start, const benchmark::Entity &end) {
        if (start.kind_ != end.kind_ || start.color_ != end.color_)
            return std::numeric_limits<double>::infinity();

        const auto start_pos =
            Eigen::Vector2d(start.pose_.translation().x(), start.pose_.translation().y());
        const auto end_pos =
            Eigen::Vector2d(end.pose_.translation().x(), end.pose_.translation().y());

        return (end_pos - start_pos).norm();
    };

    for (auto &[op, value] : worldPose_from_op_) {
        auto &entity = value.first;

        // sort by distance descending
        std::sort(explo_result.begin(),
            explo_result.end(),
            [&entity, &dist](
                const auto &l, const auto &r) { return dist(entity, l) > dist(entity, r); });

        auto &closest = explo_result.back();
        if (closest.kind_ == entity.kind_ && closest.color_ == entity.color_) {
            entity.pose_ = closest.pose_;
            update_operation_start(op, entity, corner);
            op->display = true;
            explo_result.pop_back();
        }

        else {
            std::stringstream ss;
            ss << "Closest Entity for op " << entity << " is " << closest
               << " but kind or color don't match";
            spdlog::error(ss.str());
            op->display = false;
        }
    }
    for (auto op : operations_) emit op->displayChanged();

    for (auto op : operations_)
        if (!op->display) throw std::runtime_error("Operation not found in exploration result");
}

QUrl task::get_config_file() { return config_file_; }

void task::reschedule(bool interrupted)
{
    spdlog::info("Generating new schedule");
    try {
    schedule_ = schedule(*this, settings_->human_actor, settings_->robot_actor, interrupted);
    } catch (std::exception &e) {
        spdlog::error("{}", e.what());
    }
}
}// namespace core