#include "franka_robot.h"

#include <iostream>

#include <franka/exception.h>

#include <Eigen/Dense>

#include <fmt/ostream.h>
#include <spdlog/spdlog.h>
#include "motion_generator_seq_cart_vel_tau.h"
#include "motion_generator_joint_max_accel.h"


namespace benchmark {

franka_robot::franka_robot()
    : controller_("192.168.1.1", franka::RealtimeConfig::kIgnore), gripper_("192.168.1.1"),
      flange_T_tcp([]() {
          // A force-torque sensor (adapter height 0.065 m) was mounted between the flange
          // and gripper for a parallel experiment. The sensor offset is commented out here;
          // restore "+ 0.065" and adjust the value for your sensor or adapter geometry.
          auto result =
              Eigen::Affine3d(Eigen::AngleAxisd(M_PI / 4, Eigen::Vector3d::UnitZ()))
              * Eigen::Translation3d(
                  0, 0, 0.124 /* measured offset from flange to tip of gripper */
                  // + 0.065 /* force-torque sensor adapter between flange and gripper */
              );

          return result;
      }()),
      j7_T_flange(Eigen::Translation3d(0, 0, 0.107))
{
    controller_.setCollisionBehavior({ { 20.0, 20.0, 18.0, 18.0, 16.0, 14.0, 12.0 } },
        { { 40.0, 40.0, 38.0, 38.0, 36.0, 34.0, 32.0 } },
        { { 20.0, 20.0, 18.0, 18.0, 16.0, 14.0, 12.0 } },
        { { 40.0, 40.0, 38.0, 38.0, 36.0, 34.0, 32.0 } },
        { { 20.0, 20.0, 20.0, 25.0, 25.0, 25.0 } },
        { { 40.0, 40.0, 40.0, 45.0, 45.0, 45.0 } },
        { { 20.0, 20.0, 20.0, 25.0, 25.0, 25.0 } },
        { { 40.0, 40.0, 40.0, 45.0, 45.0, 45.0 } });

    controller_.setJointImpedance({ { 3000, 3000, 3000, 2500, 2500, 2000, 2000 } });
    controller_.setCartesianImpedance({ { 3000, 3000, 3000, 300, 300, 300 } });

    gripper_.homing();

    // setup exploration poses
    auto orientation = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitX())
                       * Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ());

    auto orientation2 = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitX())
                        * Eigen::AngleAxisd(M_PI / 2, Eigen::Vector3d::UnitZ());

    poses_.emplace_back(orientation2).translation() << 0.56, 0.3, 0.38;
    poses_.emplace_back(orientation2).translation() << 0.4, 0.28, 0.37;
    // poses_.emplace_back(orientation).translation() << 0.4, -0.05, 0.35;
    poses_.emplace_back(orientation).translation() << 0.4, -0.18, 0.35;
    poses_.emplace_back(orientation).translation() << 0.27, -0.47, 0.3;
    poses_.emplace_back(orientation).translation() << 0.45, -0.48, 0.3;


    for (const auto &pose : poses_) {
        exploration_configs.push_back(inverse_kinematic(pose));
    }
}


franka_robot::~franka_robot() { gripper_.stop(); }

void franka_robot::transfer(const Eigen::Vector3d &target)
{
    auto tcp = current_world_T_tcp();
    tcp.translation() = target;
    transfer(tcp);
}

void franka_robot::transfer(const Eigen::Affine3d &target) { move_p2p(target, 0.4); }


void franka_robot::move_without_object_down_or_up(const Eigen::Affine3d &target)
{
    move_p2p(target, 0.1);
}

void franka_robot::move_with_object_down(const Eigen::Affine3d &target, Entity::kind)
{
    move_without_object_down_or_up(target);
}


void franka_robot::move_p2p(const Eigen::Affine3d &target, const double speed)
{
    const auto goal = inverse_kinematic(target);
    const Eigen::Vector3d target_translation = target.translation();
    const Eigen::Vector3d actual_translation = (fk(goal) * flange_T_tcp).translation();
    spdlog::warn("IK error due to wrong DH parameters: {} Target: {} Actual:  {}",
        fmt::streamed((target_translation - actual_translation).transpose()),
        fmt::streamed(target_translation.transpose()),
        fmt::streamed(actual_translation.transpose()));
    spdlog::trace("Moving to: {}\n", fmt::streamed(target.translation().transpose()));

    move_p2p(goal, speed);
}


void franka_robot::move_p2p(const franka_util::robot_config_7dof &joints, const double speed)
{
    std::array<double, 7> goal_arr{};
    Eigen::Matrix<double, 7, 1>::Map(goal_arr.data()) = joints;
    franka_joint_motion_generator motion_generator(speed, goal_arr, false);

    bool finished = false;
    while (!finished) {
        try {
            controller_.control(
                motion_generator, franka::ControllerMode::kJointImpedance, true, 100.);
            finished = true;
        } catch (const franka::ControlException &e) {
            std::cerr << e.what() << "\n";
            controller_.automaticErrorRecovery();
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}


void franka_robot::move_force(const Eigen::Affine3d &target)
{
    const auto goal = inverse_kinematic(target);

    std::array<double, 7> goal_arr{};
    Eigen::Matrix<double, 7, 1>::Map(goal_arr.data()) = goal;

    auto motion_generator = seq_cart_vel_tau_generator(controller_, goal_arr, { 0, 0, -3, 0, 0 });

    bool finished = false;
    while (!finished) {
        try {
            controller_.control(
                [&](const franka::RobotState &robot_state, franka::Duration period)
                    -> franka::Torques { return motion_generator.step(robot_state, period); },
                true,
                1000.);

            finished = true;
        } catch (const franka::ControlException &e) {
            std::cerr << e.what() << "\n";
        }
    }
}

Eigen::Vector3d franka_robot::current_tcp_position() {
    auto joints = current_config();
    const auto dh_params = std::array<std::array<double, 4>, 8>{ {
        { 0, 0.333, 0, joints[0] },
        { 0, -0, -M_PI/2, joints[1]  },
        { 0, 0.316, M_PI / 2, joints[2] },
        { 0.0825, 0, M_PI / 2, joints[3] },
        { -0.0825, 0.384, -M_PI / 2, joints[4] },
        { 0, 0, M_PI / 2, joints[5] },
        { 0.088, 0, M_PI / 2, joints[6] },
        { 0, 0.107, 0, 0 }// flange
    } };

    // Initialize the transformation matrix as an identity matrix
    auto T = Eigen::Affine3d::Identity();

    // Multiply the transformation matrices for each joint
    for (size_t i = 0; i < 8; ++i) {
        T = T * get_tf_mat(dh_params[i]);
    }
    auto tcp_pose = T * flange_T_tcp;
    
    return tcp_pose.translation(); 


}


double franka_robot::current_tcp_rotation() { return current_config()[6]; }

Eigen::Affine3d franka_robot::world_T_camera() const
{
    const static auto flange_T_camera = []() {
        Eigen::Affine3d transform = Eigen::Affine3d::Identity();
        transform.matrix() << 0.668734, -0.742899, 0.0299181, -0.0422515 - 0.0348507, 0.743397,
            0.668774, -0.0101355, 0.0348507 - 0.0348507, -0.0124788, 0.029019, 0.999501, 0.13, 0, 0,
            0, 1;
        return transform;
    }();
    auto world_T_cam = current_world_T_flange() * flange_T_camera;

    return world_T_cam;
}

Eigen::Affine3d franka_robot::current_world_T_tcp() const
{

    return current_world_T_flange() * flange_T_tcp;
}

Eigen::Affine3d franka_robot::get_tf_mat(const std::array<double, 4> &dh) const
{
    const double a = dh[0];
    const double d = dh[1];
    const double alpha = dh[2];
    const double theta = dh[3];

    // Construct the transformation matrix using the Denavit-Hartenberg parameters
    Eigen::Affine3d tf_matrix;
    tf_matrix.matrix() << std::cos(theta), -std::sin(theta), 0, a,
        std::sin(theta) * std::cos(alpha), std::cos(theta) * std::cos(alpha), -std::sin(alpha),
        -std::sin(alpha) * d, std::sin(theta) * std::sin(alpha), std::cos(theta) * std::sin(alpha),
        std::cos(alpha), std::cos(alpha) * d, 0, 0, 0, 1;
    return tf_matrix;
}

Eigen::Affine3d franka_robot::current_world_T_flange() const { return fk(current_config()); }

Eigen::Affine3d franka_robot::fk(const franka_util::robot_config_7dof &joints) const
{
    const auto dh_params = std::array<std::array<double, 4>, 8>{ {
        { 0, 0.333, 0.00350, joints[0] - 0.00481 },
        { 0.00581, -0.00202, -1.55281, joints[1] - 0.00415 },
        { -0.00138, 0.31110, 1.57020, joints[2] + 0.00157 },
        { 0.08231, -0.00025, 1.57188, joints[3] - 0.00712 },
        { -0.08154, 0.37795, -1.56456, joints[4] + 0.00834 },
        { 0.00123, -0.00217, 1.56363, joints[5] - 0.04197 },
        { 0.088, -0.00150, 1.56363, joints[6] },
        { -0.00008, 0.107, -0.00195, 0 }// flange
    } };

    // Initialize the transformation matrix as an identity matrix
    auto T = Eigen::Affine3d::Identity();

    // Multiply the transformation matrices for each joint
    for (size_t i = 0; i < 8; ++i) {
        T = T * get_tf_mat(dh_params[i]);
    }
    return T;
}


franka_util::robot_config_7dof franka_robot::current_config() const
{
    const auto current_joints = controller_.readOnce().q;
    return Eigen::Map<const franka_util::robot_config_7dof>(current_joints.data());
}


// void grab(const benchmark::Entity &entity)
//{
//     auto goal = current_world_T_tcp();
//     if (entity.kind_ == benchmark::Entity::kind::conductor) {
//         goal.translation().z() -= 0.015;
//     } else if (entity.kind_ == benchmark::Entity::kind::gear) {
//         goal.translation().z() -= 0.025;
//     }
//     move_p2p(goal);
//     close_gripper();
// }

void franka_robot::close_gripper(const Entity::kind) { gripper_.grasp(0.01, 0.05, 5, 0.01, 0.05); }

int franka_robot::exploration_poses() { return exploration_configs.size(); }

void franka_robot::move_to_exploration_pose(int i)
{
    if (i < 0 || i >= exploration_configs.size())
        throw std::logic_error("Invalid exploration pose");

    move_p2p(exploration_configs[i], 0.5);
}

void franka_robot::open_gripper(const Entity::kind)
{
    const auto static max_width = gripper_.readOnce().max_width;
    gripper_.move(max_width - 0.01, 0.05);
}


void franka_robot::move_to_start_pose() { move_to_exploration_pose(0); }

std::vector<Eigen::Affine3d> franka_robot::possible_tcp_T_entity(const Entity &e) const
{
    using namespace Eigen;

    // tcp facing in negative z-direction
    auto tcp = Affine3d(AngleAxisd(M_PI, Vector3d::UnitX()));

    // align gripper fingers with enitity
    tcp.rotate(Eigen::AngleAxisd(M_PI / 2, Vector3d::UnitZ()));


    // create multiple approach poses, based on rotational symmetry and current rotation
    auto approach_poses = std::vector<Affine3d>();
    for (int i = 0; i < e.symmetry(); i++) {
        approach_poses.push_back(
            tcp * AngleAxisd(-e.angle_z() + (M_PI * 2 / e.symmetry() * i), Vector3d::UnitZ()));
    }

    // set goal tcp position for all poses
    for (auto &pose : approach_poses) pose.translation() = e.pose_.translation();
    return approach_poses;
}

franka_util::robot_config_7dof franka_robot::inverse_kinematic(Eigen::Affine3d target) const
{
    // pose is world_T_flange or world_T_j7 * j7_tcp, but the ik uses only world_T_j7,
    // so we multiply by the inverse of j7_T_tcp
    auto target_config = franka_util::ik_fast_closest(
        target * (j7_T_flange * flange_T_tcp).inverse(), current_config());

    return target_config;
}


Eigen::Affine3d franka_robot::tcp_T_entity(const benchmark::Entity &entitiy) const
{
    using solution_pair = std::pair<Eigen::Affine3d, franka_util::robot_config_7dof>;
    std::vector<solution_pair> ik_solutions;

    // calculate ik for all possible approach poses
    for (const auto &pose : possible_tcp_T_entity(entitiy)) {
        try {
            ik_solutions.push_back(std::make_pair(pose, inverse_kinematic(pose)));
        } catch (...) {
        }
    }

    if (ik_solutions.empty()) {
        spdlog::error("Failed to find ic solution");
        throw std::runtime_error("Could not calculate inverse kinematic");
    }


    // find closest pose to current joint positions
    const auto current_joints(current_config());
    auto mind_distance = std::numeric_limits<double>::max();
    auto best_solution = solution_pair();

    for (const auto &solution : ik_solutions) {
        const auto dist = (solution.second - current_joints).squaredNorm();
        if (dist < mind_distance) {
            mind_distance = dist;
            best_solution = solution;
        }
    }
    spdlog::trace(fmt::format("Joints for best approach pose: {}",
        fmt::streamed(best_solution.second.matrix().transpose())));
    return best_solution.first;
}


}// namespace benchmark