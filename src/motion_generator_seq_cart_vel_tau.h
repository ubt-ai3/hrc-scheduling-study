/**
 *************************************************************************
 *
 * @file motion_generator_seq_cart_vel_tau.hpp
 *
 * Cartesian impedance torque controller for guided placement moves.
 *
 ************************************************************************/
#pragma once

#include <atomic>
#include <vector>

#include <Eigen/Geometry>

#include <franka/robot.h>
#include <franka/model.h>


/**
 * @brief Cartesian impedance torque controller that moves toward a target joint configuration
 *        while applying a desired end-effector wrench.
 *
 * Used for force-guided placement: the robot approaches a joint target (q) and
 * simultaneously tracks a Cartesian force/torque setpoint (f).  A low-pass
 * filter on both joint velocities and force/torque measurements reduces noise.
 *
 * Throw stop_motion_trigger or contact_stop_trigger (via the franka control callback)
 * to terminate the torque controller loop from within the real-time thread.
 */
class seq_cart_vel_tau_generator
{
  public:
    /** @brief Exception thrown by the control callback to terminate the motion normally. */
    class stop_motion_trigger
    {
    };
    /** @brief Exception thrown by the control callback when unexpected contact is detected. */
    class contact_stop_trigger
    {
    };

    /**
     * @brief Constructs the controller and initialises stiffness/damping matrices.
     *
     * @param robot  Reference to the libfranka robot (used to load the dynamic model).
     * @param q      Target joint configuration (7 values, radians).
     * @param f      Desired end-effector wrench [fx, fy, fz, tx, ty, tz] in robot base frame.
     */
    seq_cart_vel_tau_generator(franka::Robot &robot,
        std::array<double, 7> q,
        std::array<double, 6> f);

    /**
     * @brief Real-time control callback — computes and returns joint torques for one time step.
     *
     * Called by libfranka at 1 kHz inside the robot's real-time loop.  Filters
     * joint velocities and measured forces, then applies a Cartesian impedance
     * law to drive toward the target pose and force.
     */
    franka::Torques step(const franka::RobotState &robot_state, franka::Duration period);

  private:
    void update_dq_filter(const franka::RobotState &robot_state);
    Eigen::Matrix<double, 7, 1> compute_dq_filtered();


    void update_ft_filter(const Eigen::Matrix<double, 6, 1> &current_ft);
    Eigen::Matrix<double, 6, 1> compute_ft_filtered();


    using eigen_vector7d = Eigen::Matrix<double, 7, 1>;


    franka::Model model_;


    double time_ = 0.0;

    const std::array<double, 7> q_;
    const std::array<double, 6> f_;


    size_t dq_current_filter_position_ = 0;
    size_t dq_filter_size_ = 10;
    std::vector<eigen_vector7d> dq_buffer_;


    size_t ft_current_filter_position_ = 0;
    size_t ft_filter_size_ = 20;
    std::vector<Eigen::Matrix<double, 6, 1>> ft_buffer_;


    const double translational_stiffness_{ 3000.0 };
    const double rotational_stiffness_{ 300.0 };
    Eigen::MatrixXd stiffness_;
    Eigen::MatrixXd damping_;

    const double target_mass{ 0.0 };
    double desired_mass_{ 0.0 };
    double filter_gain{ 0.05 };
    Eigen::Matrix<double, 6, 1> force_error_integral_{ Eigen::Matrix<double, 6, 1>::Zero() };

    double f_z_error_integral_{ 0.0 };


    const Eigen::Vector<double, 7>
        joint_limits_min{ 2.8973, 1.7628, 2.8973, -0.0698, 2.8973, 3.7525, 2.8973 };
    const Eigen::Vector<double, 7>
        joint_limits_max{ -2.8973, -1.7628, -2.8973, -3.0718, -2.8973, -0.0175, -2.8973 };
};