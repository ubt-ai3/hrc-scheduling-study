/**
 *************************************************************************
 *
 * @file franka_util.hpp
 *
 * Static franka utilities.
 *
 ************************************************************************/
#pragma once

#include <vector>

#include <Eigen/Geometry>


/** @brief Stores the minimum and maximum allowed value for a single Franka joint. */
struct joint_limit
{
    joint_limit(double min, double max);

    double min;
    double max;
};


/**
 * @brief Static utility functions for the Franka Panda kinematics and dynamics.
 *
 * Provides joint limits, kinematic limits, forward kinematics via IKFast, and
 * inverse kinematics via IKFast with multiple fallback strategies.  All methods
 * are stateless and thread-safe.
 *
 * Coordinate convention: the target pose for IK is expressed as world_T_nsa,
 * i.e., the transform from the world (robot base) frame to the Franka NSA frame
 * (joint-7 frame as defined by the Franka DH parameters).
 */
class franka_util
{
  public:
    using robot_config_7dof = Eigen::Matrix<double, 7, 1>;

    /** @brief Returns per-joint position limits [min, max] in radians. */
    static std::vector<joint_limit> joint_limits();
    /** @brief Returns the maximum joint velocity (rad/s) for each of the 7 joints. */
    static robot_config_7dof max_speed_per_joint();
    /** @brief Returns the maximum joint acceleration (rad/s²) for each of the 7 joints. */
    static robot_config_7dof max_acc_per_joint();
    /** @brief Returns the maximum joint jerk (rad/s³) for each of the 7 joints. */
    static robot_config_7dof max_jerk_per_joint();

    /** @brief Returns true if all joints of @p target lie within their position limits. */
    static bool is_reachable(const robot_config_7dof &target);

    /**
     * @brief Computes forward kinematics for all 7 joint frames.
     * @return Transforms world_T_joint_i for i = 0…6.
     */
    static std::vector<Eigen::Affine3d> fk(const robot_config_7dof &configuration);

    /**
     * @brief Computes all IKFast solutions for a target NSA pose with a fixed joint-4 value.
     * @param world_T_nsa    Desired end-effector pose in world frame.
     * @param joint_4_value  Fixed value (radians) for joint 4 (reduces solution space).
     * @return All reachable IK solutions; empty if none exist.
     */
    static std::vector<robot_config_7dof> ik_fast(const Eigen::Affine3d &world_T_nsa,
        double joint_4_value = 0.);

    /**
     * @brief Sweeps joint-4 over its range to find IK solutions at multiple values.
     * @param step_size  Increment for joint-4 sweep (radians); default ~10 degrees.
     * @return All reachable IK solutions across the sweep; empty if none exist.
     */
    static std::vector<robot_config_7dof> ik_fast_robust(const Eigen::Affine3d &world_T_nsa,
        double step_size = 0.174533);

    /**
     * @brief Returns the IK solution closest to @p current_configuration in joint space.
     *
     * Uses ik_fast_robust internally; throws if no solution is found.
     */
    static robot_config_7dof ik_fast_closest(const Eigen::Affine3d &target_world_T_nsa,
        const robot_config_7dof &current_configuration,
        double step_size = 0.174533);

    /**
     * @brief Tool inertial properties for load estimation.
     *
     * The values reflect the bare Franka Hand (no sensor adapter).  If a
     * force-torque sensor or other adapter is mounted between the flange and
     * the gripper, uncomment and adjust the m_fts contributions in franka_util.cpp
     * and update the Franka web interface accordingly.
     */
    static double tool_mass();
    static Eigen::Vector3d tool_center_of_mass();
    static Eigen::Matrix3d tool_inertia();


    static const joint_limit joint_limits_[];


    static constexpr double pi = 3.14159265358979323846;
    static constexpr double deg_to_rad = pi / 180;
    static constexpr double rad_to_deg = 180 / pi;


  private:
    static Eigen::Matrix3d a_tilde(const Eigen::Vector3d &a);
};