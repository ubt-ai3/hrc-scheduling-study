#include "robot_handler.h"

#include <fstream>

#include <spdlog/spdlog.h>
#include <fmt/ostream.h>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <serialization.h>
#include "franka_robot.h"
#include "task.h"

namespace core {

robot_handler::robot_handler() : worker_(&robot_handler::work, this) {}


hardware_robot_handler::hardware_robot_handler(task& task) :task_(task), bot_(std::make_unique<benchmark::franka_robot>())
{}

void robot_handler::execute_operation(const benchmark::Entity &start,
    const benchmark::Entity &end,
    const std::function<void()> &callback)
{

    std::unique_lock lk(lock_);
    operation_completed_callback_ = callback;
    open_operation_ = true;
    start_ = start;
    end_ = end;

    cv_.notify_one();
}

void robot_handler::start_exporation(const std::function<void()> &callback)
{
    std::unique_lock lk(lock_);
    exploration_completed_callback_ = callback;
    explore_ = true;

    cv_.notify_one();
}

std::vector<benchmark::Entity> robot_handler::expolation_result() const
{
    std::lock_guard lk(explore_result_lock_);
    return explored_entities_;
}

robot_handler::~robot_handler()
{
    stopped_ = true;
    cv_.notify_all();
    worker_.join();
}


void robot_handler::work()
{
    try {
        while (true) {
            std::unique_lock lk(lock_);
            cv_.wait(lk, [&]() { return open_operation_ || stopped_ || explore_; });
            if (stopped_)
                return;
            else if (explore_) {
                std::unique_lock exp_lk(explore_result_lock_);
                explored_entities_ = explore();
                explore_ = false;
                lk.unlock();
                exp_lk.unlock();
                exploration_completed_callback_();

            } else if (open_operation_) {
                handle_op(start_, end_);
                open_operation_ = false;
                lk.unlock();
                operation_completed_callback_();
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "hardware_robot_handler thread died\n" << e.what() << "\n";
    }
}

std::vector<benchmark::Entity> hardware_robot_handler::explore() {
    const auto entities = bot_->explore(); 

        // assert that all operations are found
    const auto corner = task_.map_ops_to_world(entities);
    task_.snap_ops_to_detected_entities(entities, corner);
    // let the user move the bot to each entity
    const auto refined_entities = teach_entity_positions(entities);

    const auto new_corner = task_.map_ops_to_world(refined_entities);
    task_.snap_ops_to_detected_entities(refined_entities, new_corner);

    auto config_file = task_.get_config_file();
    try {
        if (config_file.isEmpty()) {
            spdlog::error("Can not write exploration file; There is no config set");
            return {};
        }
        auto exploration_path =
            config_file.adjusted(QUrl::RemoveFilename).toLocalFile().toStdString()
            + "found_entities.dat";

        std::ofstream fileStream(exploration_path, std::ios::trunc);
        if (!fileStream.is_open()) {
            spdlog::error("Can not write exploration file; Couldn't open file");
            return {};
        }
        boost::archive::text_oarchive archive(fileStream);
        archive << refined_entities;

    } catch (const std::exception &e) {
        spdlog::error(fmt::format("Could not write exploration to disk: {}", e.what()));
        return {};
    }

    bot_->move_to_start_pose();
    return refined_entities;
}

std::vector<benchmark::Entity> hardware_robot_handler::teach_entity_positions(
    std::vector<benchmark::Entity> entities)
{   
    using kind = benchmark::Entity::kind;

    bot_->close_gripper(kind::conductor);
    for (auto &e : entities) {
        if (e.kind_ == kind::conductor || e.kind_ == kind::cube
           || e.kind_ == kind::long_cube || e.kind_ == kind::gear ||
            e.kind_
            == kind::corner_marker) {
            if (e.kind_ == kind::corner_marker)
                e.pose_ *= Eigen::Translation3d(-sqrt(2 * 0.04 * 0.04), 0, 0);

            while (true) 
                try {
                    bot_->move_to(e);
                    break;
                } catch (...) {
                    spdlog::warn("Cannot move. Press enter to try again.");
                    std::cin.get();
                }

            spdlog::critical("Move the robot just above the entity {}", fmt::streamed(e));
            std::cin.get();
            const auto t = bot_->current_tcp_position();
            spdlog::info("Changed entity position from {} to {}",
                fmt::streamed(e.pose_.translation().transpose()),
                fmt::streamed(t.transpose()));
            e.pose_.translation() = t;
            
            if (e.kind_ == kind::corner_marker)
                e.pose_ *= Eigen::Translation3d(sqrt(2 * 0.04 * 0.04), 0, 0);
        }
    }
    bot_->move_to_start_pose();
    bot_->open_gripper(kind::conductor);

    return entities;
}

void hardware_robot_handler::handle_op(const benchmark::Entity &start, const benchmark::Entity &end)
{
    bot_->pick(start);
    bot_->place(end);
}

std::vector<benchmark::Entity> simulated_robot_handler::explore() { return {}; }
void simulated_robot_handler::handle_op(const benchmark::Entity &,
    const benchmark::Entity &)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
}// namespace core