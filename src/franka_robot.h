#pragma once
#include "abstract_robot.h"
#include <franka/robot.h>
#include <franka/gripper.h>

#include "franka_util.h"

namespace benchmark {

/**
 * @brief Concrete abstract_robot implementation for the Franka Panda arm.
 *
 * Connects to a physical Franka robot and its gripper over FCI (Franka Control Interface).
 * Motion primitives are built on two motion generators:
 *   - franka_joint_motion_generator  — joint-space point-to-point moves
 *   - seq_cart_vel_tau_generator     — Cartesian impedance control for force-guided placement
 *
 * The constant transforms that encode the robot's physical configuration are:
 *   - flange_T_tcp:  flange → TCP (finger tip); see README for adjustment instructions.
 *   - j7_T_flange:   joint-7 → flange; fixed by the Franka kinematic model.
 *
 * Exploration poses and their associated joint configurations are loaded from
 * a data file at construction; see exploration_configs / poses_.
 */
class franka_robot final : public abstract_robot
{
  public:
    franka_robot();
    ~franka_robot();

    void transfer(const Eigen::Vector3d &target) override;
    void transfer(const Eigen::Affine3d &target) override;

    void move_to_start_pose() override;
    void move_without_object_down_or_up(const Eigen::Affine3d &target) override;
    void move_with_object_down(const Eigen::Affine3d &target, Entity::kind kind) override;

    void open_gripper(const Entity::kind kind) override;
    void close_gripper(const Entity::kind kind) override;

    int exploration_poses() override;
    void move_to_exploration_pose(int i) override;

    virtual Eigen::Vector3d current_tcp_position() override;
    virtual double current_tcp_rotation() override;
    Eigen::Affine3d tcp_T_entity(const Entity &entitiy) const override;
    Eigen::Affine3d world_T_camera() const override;

    /** @brief Returns the current world-to-TCP transform from libfranka model. */
    Eigen::Affine3d current_world_T_tcp() const;
    /** @brief Returns the world-to-TCP transform computed from IKFast FK (used for debugging). */
    Eigen::Affine3d bad_fk_current_world_T_tcp() const;
    /** @brief Returns the current world-to-flange transform. */
    Eigen::Affine3d current_world_T_flange() const;
    /** @brief Returns the current 7-DOF joint configuration. */
    franka_util::robot_config_7dof current_config() const;

    /** @brief Executes a joint-space point-to-point move to @p joints at @p speed (0–1). */
    void move_p2p(const franka_util::robot_config_7dof &joints, const double speed);
    /** @brief Executes a joint-space point-to-point move to a Cartesian @p target_world_T_tcp at @p speed (0–1). */
    void move_p2p(const Eigen::Affine3d &target_world_T_tcp, const double speed);
    /** @brief Moves to @p target_world_T_tcp using Cartesian impedance control with force feedback. */
    void move_force(const Eigen::Affine3d &target_world_T_tcp);

  private:
    Eigen::Affine3d fk(const franka_util::robot_config_7dof &joints) const;
    std::vector<Eigen::Affine3d> poses_;
    std::vector<franka_util::robot_config_7dof> exploration_configs;

    std::vector<Eigen::Affine3d> possible_tcp_T_entity(const Entity &e) const;
    franka_util::robot_config_7dof inverse_kinematic(Eigen::Affine3d target) const;
    Eigen::Affine3d get_tf_mat(const std::array<double, 4> &dh) const;

    const Eigen::Affine3d flange_T_tcp;
    const Eigen::Affine3d j7_T_flange;

    mutable franka::Robot controller_;
    mutable franka::Gripper gripper_;
};

}// namespace benchmark