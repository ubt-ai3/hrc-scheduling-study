#pragma once
#include <chrono>

#include <Eigen/Geometry>

#include "vision.h"
#include "Camera.hpp"

namespace benchmark {

/**
 * @brief Hardware-independent robot interface for HRC assembly tasks.
 *
 * Provides high-level pick/place/explore operations that are independent of
 * the concrete robot platform. Subclasses implement the platform-specific
 * motions; this class owns the camera and the object detector and uses them
 * to locate entities during exploration.
 *
 * The fixed coordinate frame convention is:
 *   - world frame: robot base frame
 *   - camera frame: RealSense optical frame (Z forward)
 *   - world_T_camera() gives the static transform between them
 */
class abstract_robot
{
  public:
    /** @brief Picks up the given entity (move above, descend, grasp, lift). */
    virtual void pick(Entity e) final;
    /** @brief Places the given entity at its target pose (move above, descend, release, lift). */
    virtual void place(Entity e) final;
    /**
     * @brief Sweeps through all exploration poses and detects visible entities.
     * @return All entities observed across all exploration viewpoints.
     */
    virtual std::vector<benchmark::Entity> explore() final;

    /** @brief Moves the TCP above the given entity's position without grasping. */
    virtual void move_to(const benchmark::Entity &e) final;

    /** @brief Returns the current TCP position in world coordinates. */
    virtual Eigen::Vector3d current_tcp_position() = 0;
    /** @brief Returns the current TCP rotation around the world Z-axis (radians). */
    virtual double current_tcp_rotation() = 0;
    /** @brief Moves the robot to its predefined start/home pose. */
    virtual void move_to_start_pose() = 0;

    /** @brief Opens the gripper with the aperture appropriate for @p kind. */
    virtual void open_gripper(const Entity::kind kind) = 0;
    /** @brief Closes the gripper around an object of type @p kind. */
    virtual void close_gripper(const Entity::kind kind) = 0;

  protected:
    /** @brief Moves the TCP to @p target position (XYZ in world frame) at transfer height. */
    virtual void transfer(const Eigen::Vector3d &target) = 0;
    /** @brief Moves the TCP to @p target pose (full 6-DOF) at transfer height. */
    virtual void transfer(const Eigen::Affine3d &target) = 0;

    /** @brief Descends to or ascends from @p target without a grasped object. */
    virtual void move_without_object_down_or_up(const Eigen::Affine3d &target) = 0;
    /** @brief Descends to @p target while holding an object of type @p kind. */
    virtual void move_with_object_down(const Eigen::Affine3d &target, Entity::kind kind) = 0;

    /** @brief Returns the number of exploration viewpoints. */
    virtual int exploration_poses() = 0;
    /** @brief Moves the robot to the i-th exploration pose. */
    virtual void move_to_exploration_pose(int i) = 0;

    /**
     * @brief Returns the transform from TCP frame to entity grasp frame for @p entity.
     *
     * Used to compute the TCP target pose that correctly grasps a specific entity type.
     */
    virtual Eigen::Affine3d tcp_T_entity(const Entity &entitiy) const = 0;
    /** @brief Returns the static transform from camera frame to world frame. */
    virtual Eigen::Affine3d world_T_camera() const = 0;

  private:
    Detection detection_ = Detection("./data/thresholds.realsense.json");
    realsense_camera camera_;

    const double transfer_z = 0.3;
};

}// namespace benchmark