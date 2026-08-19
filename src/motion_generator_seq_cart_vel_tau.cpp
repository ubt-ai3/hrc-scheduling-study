/**
 *************************************************************************
 *
 * @file motion_generator_seq_cart_vel_tau.cpp
 *
 * ..., implementation.
 *
 ************************************************************************/


#include "motion_generator_seq_cart_vel_tau.h"

#include <utility>
#include <iostream>
#include <fstream>

#include <Eigen/Dense>

#include <franka/model.h>


//////////////////////////////////////////////////////////////////////////
//
// seq_cart_vel_tau_generator
//
//////////////////////////////////////////////////////////////////////////


seq_cart_vel_tau_generator::seq_cart_vel_tau_generator(franka::Robot &robot,
    std::array<double, 7> q,
    std::array<double, 6> f)
    : model_(robot.loadModel()), q_(q), f_(f), dq_buffer_(dq_filter_size_, eigen_vector7d::Zero()),
      ft_buffer_(ft_filter_size_, Eigen::Matrix<double, 6, 1>::Zero()), stiffness_(6, 6),
      damping_(6, 6)
{
    stiffness_.setZero();
    stiffness_.topLeftCorner(3, 3) = translational_stiffness_ * Eigen::MatrixXd::Identity(3, 3);
    stiffness_.bottomRightCorner(3, 3) = rotational_stiffness_ * Eigen::MatrixXd::Identity(3, 3);

    damping_.setZero();
    damping_.topLeftCorner(3, 3) =
        .5 * sqrt(translational_stiffness_) * Eigen::MatrixXd::Identity(3, 3);
    damping_.bottomRightCorner(3, 3) =
        .5 * sqrt(rotational_stiffness_) * Eigen::MatrixXd::Identity(3, 3);
}


franka::Torques seq_cart_vel_tau_generator::step(const franka::RobotState &robot_state,
    franka::Duration period)
{
    const auto selection_vector = std::array<double, 6>{ 1, 1, 0, 1, 1, 1 };

    time_ += period.toSec();
    bool motion_finished = false;

    // get state variables
    update_dq_filter(robot_state);
    Eigen::Map<const eigen_vector7d> tau_measured(robot_state.tau_J.data());

    std::array<double, 42> jacobian_array =
        model_.zeroJacobian(franka::Frame::kEndEffector, robot_state);
    Eigen::Map<const Eigen::Matrix<double, 6, 7>> jacobian(jacobian_array.data());

    std::array<double, 49> mass_array = model_.mass(robot_state);
    Eigen::Map<eigen_vector7d> mass(mass_array.data());
    std::array<double, 7> coriolis_array = model_.coriolis(robot_state);
    Eigen::Map<eigen_vector7d> coriolis(coriolis_array.data());
    std::array<double, 7> gravity_array = model_.gravity(robot_state);
    Eigen::Map<eigen_vector7d> const gravity(gravity_array.data());


    // --- force motion ---

    Eigen::Matrix<double, 6, 1> ft_desired(f_.data());
    Eigen::Map<const Eigen::Matrix<double, 6, 1>> ft_existing(robot_state.O_F_ext_hat_K.data());

    // ff
    Eigen::Matrix<double, 6, 1> ft_force = ft_desired;


    // pi controller using fts for neg z-direction
    if (selection_vector[2] == 0) {
        double error_fz = (ft_desired(2) - (ft_existing(2)));
        f_z_error_integral_ += error_fz * period.toSec();
        ft_force(2) += 0.3 * error_fz + 5.0 * f_z_error_integral_;
        // ft_force(2) = -3.0;// std::clamp(ft_force(2), -8.0, 0.0);
        static int counter = 0;
        if (counter++ % 20 == 0) std::cout << ft_force[2] << "\t" << ft_existing[2] << "\n";
    }

    update_ft_filter(ft_force);// todo use selection vector
    ft_force = compute_ft_filtered();
    // --- force motion end ---


    // --- cartesian motion ---
    auto current_pose = model_.pose(
        franka::Frame::kEndEffector, robot_state.q, robot_state.F_T_EE, robot_state.EE_T_K);
    Eigen::Affine3d transform(Eigen::Matrix4d::Map(current_pose.data()));
    // Eigen::Affine3d transform(Eigen::Matrix4d::Map(robot_state.O_T_EE.data()));
    Eigen::Vector3d position(transform.translation());
    Eigen::Quaterniond orientation(transform.linear());


    // calculate pose from desired joints
    auto desired_pose =
        model_.pose(franka::Frame::kEndEffector, q_, robot_state.F_T_EE, robot_state.EE_T_K);

    Eigen::Affine3d transform_d(Eigen::Matrix4d::Map(desired_pose.data()));
    Eigen::Vector3d position_d(transform_d.translation());
    Eigen::Quaterniond orientation_d(transform_d.linear());


    // compute error to desired pose
    Eigen::Matrix<double, 6, 1> error;

    // position error
    error.head(3) = position - position_d;

    // orientation error
    if (orientation_d.coeffs().dot(orientation.coeffs()) < 0.0)
        orientation.coeffs() = -orientation.coeffs();
    Eigen::Quaterniond error_quaternion(orientation.inverse() * orientation_d);

    error.tail(3) << error_quaternion.x(), error_quaternion.y(), error_quaternion.z();

    // only rotate if we exert pressure
    static double rot_dir = -1.0;
    const auto j_7 = robot_state.q[6];
    constexpr auto min_offset = 10 * (M_PI / 180);
    if (std::abs(j_7 - joint_limits_max[6]) < min_offset)
        rot_dir = 1.0;
    else if (std::abs(j_7 - joint_limits_min[6]) < min_offset)
        rot_dir = -1.0;
    error[5] = rot_dir * 0.3 * (M_PI / 180);


    // transform to base frame
    error.tail(3) = -transform_d.linear() * error.tail(3);


    if (error[2] < 0.003) motion_finished = true;


    if (error.head(2).norm() > 0.005) {
        error.head(2) = error.head(2).normalized() * 0.005;
    }


    if (error.tail(3).norm() > 0.005) {
        error.tail(3) = error.tail(3).normalized() * 0.005;
    }


    // spring damper system with damping ratio=1 and filtered dq
    Eigen::Matrix<double, 6, 1> ft_cartesian_motion =
        -stiffness_ * error - damping_ * (jacobian * compute_dq_filtered());
    // --- cartesian motion end ---


    Eigen::Matrix<double, 6, 1> ft_task;
    for (int i = 0; i < 6; ++i)
        ft_task[i] =
            selection_vector[i] * ft_cartesian_motion[i] + (1 - selection_vector[i]) * ft_force[i];


    // --- compute control ---
    Eigen::Matrix<double, 7, 1> tau_task = jacobian.transpose() * ft_task;
    Eigen::Matrix<double, 7, 1> tau_d = tau_task + coriolis;

    std::array<double, 7> tau_d_array{};
    Eigen::VectorXd::Map(&tau_d_array[0], 7) = tau_d;
    franka::Torques output(tau_d_array);
    output.motion_finished = motion_finished;
    return output;
}


void seq_cart_vel_tau_generator::update_dq_filter(const franka::RobotState &robot_state)
{
    dq_buffer_[dq_current_filter_position_] = eigen_vector7d(robot_state.dq.data());

    dq_current_filter_position_ = (dq_current_filter_position_ + 1) % dq_filter_size_;
}


Eigen::Matrix<double, 7, 1> seq_cart_vel_tau_generator::compute_dq_filtered()
{
    eigen_vector7d value(eigen_vector7d::Zero());

    for (size_t i = 0; i < dq_filter_size_; ++i) value += dq_buffer_[i];

    return value / dq_filter_size_;
}


void seq_cart_vel_tau_generator::update_ft_filter(const Eigen::Matrix<double, 6, 1> &current_ft)
{
    ft_buffer_[ft_current_filter_position_] = current_ft;

    ft_current_filter_position_ = (ft_current_filter_position_ + 1) % ft_filter_size_;
}


Eigen::Matrix<double, 6, 1> seq_cart_vel_tau_generator::compute_ft_filtered()
{
    Eigen::Matrix<double, 6, 1> value(Eigen::Matrix<double, 6, 1>::Zero());

    for (size_t i = 0; i < ft_filter_size_; ++i) value += ft_buffer_[i];

    return value / ft_filter_size_;
}
