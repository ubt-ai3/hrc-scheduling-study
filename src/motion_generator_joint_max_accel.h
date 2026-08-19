#include <array>

#include <franka/robot.h>
#include <Eigen/Dense>
/**
 *************************************************************************
 *
 * @class franka_joint_motion_generator
 *
 * An example showing how to generate a joint pose motion to a goal
 * position. Adapted from:
 * Wisama Khalil and Etienne Dombre. 2002. Modeling, Identification and
 * Control of Robots (Kogan Page Science Paper edition).
 *
 ************************************************************************/
class franka_joint_motion_generator
{
  public:
    /**
     * Thrown from motion_generators to terminate it.
     */
    class stop_motion_trigger
    {
    };
    class contact_stop_trigger
    {
    };


    /**
     * Creates a new joint_motion_generator instance for a target q.
     *
     * @param[in] speed_factor General speed factor in range [0, 1].
     * @param[in] q_goal Target joint positions.
     */
    franka_joint_motion_generator(double speed_factor,
        std::array<double, 7> q_goal,
        bool stop_on_contact);

    /**
     * Sends joint position calculations
     *
     * @param[in] robot_state Current state of the robot.
     * @param[in] period Duration of execution.
     *
     * @return Joint positions for use inside a control loop.
     */
    franka::JointPositions operator()(const franka::RobotState &robot_state,
        franka::Duration period);


  private:
    using Vector7d = Eigen::Matrix<double, 7, 1, Eigen::ColMajor>;
    using Vector7i = Eigen::Matrix<int, 7, 1, Eigen::ColMajor>;

    bool calculateDesiredValues(double t, Vector7d *delta_q_d) const;
    void calculateSynchronizedValues();

    static bool colliding(const franka::RobotState &state);

    static constexpr double kDeltaQMotionFinished = 1e-6;
    const Vector7d q_goal_;

    Vector7d q_start_;
    Vector7d delta_q_;

    Vector7d dq_max_sync_;
    Vector7d t_1_sync_;
    Vector7d t_2_sync_;
    Vector7d t_f_sync_;
    Vector7d q_1_;

    double time_ = 0.0;

    Vector7d dq_max_ = (Vector7d() << 2.0, 2.0, 2.0, 2.0, 2.5, 2.5, 2.5).finished();
    Vector7d ddq_max_start_ = (Vector7d() << 5, 5, 5, 5, 5, 5, 5).finished();
    Vector7d ddq_max_goal_ = (Vector7d() << 5, 5, 5, 5, 5, 5, 5).finished();


    const bool stop_on_contact_;
};