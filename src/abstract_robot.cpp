#include "abstract_robot.h"

#include <string>

#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <pcl/common/transforms.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/io/pcd_io.h>

#include "visualization.h"
#include "world_model.h"
#include <spdlog/spdlog.h>
#include <boost/archive/text_oarchive.hpp>
#include <serialization.h>

namespace benchmark {

// defines how much of the object should be positioned inside the gripper,
// ie how far we have to move the tcp down.
double pick_offset(Entity::kind k)
{
    switch (k) {
    case Entity::kind::gear:
        return 0.025;
    case Entity::kind::conductor:
        return 0.015;
    case Entity::kind::cube:
    case Entity::kind::long_cube:
        return 0.035;
    default:
        return 0;
    }
}
double place_offset(Entity::kind k)
{
    switch (k) {
    case Entity::kind::conductor:
        return 0.02;
    case Entity::kind::cube:
    case Entity::kind::long_cube:
        return 0.025;
    default:
        return 0;
    }
}


void abstract_robot::pick(Entity e)
{
    const auto entity = e;

    auto approach_pose = tcp_T_entity(entity);
    const auto z_goal = approach_pose.translation().z();

    Eigen::Affine3d hover_pose = approach_pose;
    Eigen::Affine3d grip_pose = approach_pose;
    hover_pose.translation().z() = transfer_z;

    try {
        transfer(hover_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }


    try {
        transfer(approach_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }

    // move down based on the grip height of the given entity
    grip_pose.translation().z() = z_goal - pick_offset(e.kind_);


    move_without_object_down_or_up(grip_pose);
    close_gripper(e.kind_);

    // pick entity from board
    try {
        move_without_object_down_or_up(approach_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }

    // move to transfer plane
    try {
        transfer(hover_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }
}

void abstract_robot::place(Entity e)
{
    const auto entity = e;

    auto approach_pose = tcp_T_entity(entity);
    const auto z_goal = approach_pose.translation().z();

    Eigen::Affine3d hover_pose = approach_pose;
    Eigen::Affine3d grip_pose = approach_pose;
    hover_pose.translation().z() = transfer_z;


    try {
        transfer(hover_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }


    try {
        transfer(approach_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }

    // move down based on the grip height of the given entity
    grip_pose.translation().z() = z_goal + place_offset(e.kind_) - pick_offset(e.kind_);

    move_without_object_down_or_up(grip_pose);

    open_gripper(e.kind_);

    // pick entity from board

    try {
        move_without_object_down_or_up(approach_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }

    // move to transfer plane

    try {
        transfer(hover_pose);
    } catch (const std::exception &e) {
        spdlog::trace("{}", e.what());
    }
}

std::vector<benchmark::Entity> abstract_robot::explore()
{
    auto world = world_model(0.025);

    std::filesystem::create_directory("./logs");
    std::filesystem::create_directory("./logs/exploration");

    using namespace std::literals;

    for (int i = 0; i < exploration_poses(); i++) {
        move_to_exploration_pose(i);
        for (int j = 0; j < 5; j++) camera_.record();

        auto [image, cloud] = camera_.record();
        const auto curr_world_T_camera = world_T_camera();
        pcl::transformPointCloud(*cloud, *cloud, curr_world_T_camera);

        cv::imwrite("./logs/exploration/image_"s + std::to_string(i) + ".png"s, image);
        pcl::io::savePCDFileASCII(
            "./logs/exploration/image_"s + std::to_string(i) + ".pcd"s, *cloud);
        std::ofstream file("./logs/exploration/image_"s + std::to_string(i) + ".archive");
        boost::archive::text_oarchive oa(file);
        oa << curr_world_T_camera.matrix();

        world.update(detection_.find_objects(image, cloud));
    }
    return world.entities();
}

void abstract_robot::move_to(const benchmark::Entity &e)
{
    auto pose = tcp_T_entity(e);
    pose.translation().z() = transfer_z;
    transfer(pose);

    auto goal = tcp_T_entity(e);
    transfer(goal.translation() + Eigen::Vector3d(0, 0, 0.1));
    transfer(goal.translation() + Eigen::Vector3d(0, 0, 0.02));
    // transfer(goal);
}

}// namespace benchmark