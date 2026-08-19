# Integrating a New Robot or Camera

This guide is for developers who want to replace the Franka Panda arm (or the Intel RealSense camera) with different hardware.
The application is designed around two abstract base classes -- `benchmark::abstract_robot` and `hardware::camera` -- so swapping hardware means implementing those interfaces and wiring the new class into `hardware_robot_handler`.

---

## Coordinate Frame Conventions

Every pose and point cloud in this codebase uses one of two frames. Getting these wrong silently produces incorrect grasps and wrong object positions.

| Name | Definition | Axis layout |
|---|---|---|
| **world** | Robot base frame (origin at center of mounting flange face of the robot base) | Z up, X forward (toward the workspace) |
| **camera** | Camera optical frame | Z forward (into the scene), X right, Y down -- the RealSense convention |

All Eigen transforms are named `A_T_B`, meaning "the pose of frame B expressed in frame A", i.e., `p_A = A_T_B * p_B`.

Key transforms:
- `world_T_camera()` -- the only transform a new robot **must** provide. It maps camera-frame points into the world frame.
- `flange_T_tcp` -- the static offset from the robot's flange (output flange face) to the Tool Center Point.
- `current_world_T_flange()` -- the forward kinematics result; the robot computes this internally.

The exploration loop in `abstract_robot::explore()` calls `world_T_camera()` once per viewpoint, transforms the captured point cloud with `pcl::transformPointCloud`, and feeds it to the object detector. If `world_T_camera()` is wrong by even a few centimetres, the detected entity positions will be wrong and pick/place motions will fail.

---

## The Flange-to-TCP Offset

The TCP is the physical fingertip (or gripping point) of the end effector. The robot's kinematic model ends at the flange face; everything beyond that is part of the tool and must be specified as `flange_T_tcp`.

In `franka_robot.cpp` the transform is built as:

```cpp
flange_T_tcp =
    Eigen::Affine3d(Eigen::AngleAxisd(M_PI / 4, Eigen::Vector3d::UnitZ()))
    * Eigen::Translation3d(
        0, 0, 0.124   // measured from flange face to Franka Hand fingertip
        // + 0.065    // optional: force-torque sensor adapter between flange and gripper
    );
```

Two physical quantities are encoded here:

1. **0.124 m** -- the measured distance along the flange Z-axis from the flange face to the tip of the Franka Hand fingers (with the fingers at their home width). Measure this with calipers on your specific gripper.
2. **0.065 m** (commented out) -- the height of a force-torque sensor adapter that was inserted between the flange and the gripper in a parallel experiment. If your setup includes an adapter plate, sensor, or spacer, add its height here and uncomment the line.
3. **pi/4 rotation about Z** -- the Franka Hand is mounted at 45 degrees relative to the flange. Adjust or remove this for a different end effector.

When implementing a new robot, replicate this pattern: construct `flange_T_tcp` from the actual measured geometry of your tool, including any adapter between the robot flange and the gripping surface. If your kinematic library already handles the TCP offset (e.g., via a configuration file), set `flange_T_tcp` to identity and delegate the offset there -- but be consistent.

---

## Implementing `benchmark::abstract_robot`

Declare your class in a new header under `src/`:

```cpp
#include "abstract_robot.h"

namespace benchmark {
class my_robot final : public abstract_robot
{
  public:
    my_robot();
    // ... override declarations
};
}
```

### Pure-virtual methods to implement

All of the following are pure-virtual in `src/abstract_robot.h` and must be overridden.

#### Query methods

```cpp
Eigen::Vector3d current_tcp_position() override;
double current_tcp_rotation() override;
```

- `current_tcp_position()` -- return the current TCP position in **world frame** (metres).
- `current_tcp_rotation()` -- return the current rotation of the TCP about the world Z-axis (radians). For a 7-DOF arm this is typically joint-7 angle. Only the Z component matters; this value is used to align the gripper with the detected object orientation before grasping.

#### Motion primitives -- public

```cpp
void move_to_start_pose() override;
void open_gripper(const Entity::kind kind) override;
void close_gripper(const Entity::kind kind) override;
```

- `move_to_start_pose()` -- bring the robot to a known safe home configuration. In `franka_robot` this is the first exploration pose. The function is called once at the start of a study run.
- `open_gripper(kind)` / `close_gripper(kind)` -- open or close the gripper. `Entity::kind` is an enum (`gear`, `conductor`, `cube`, `long_cube`). If your gripper does not distinguish object types, ignore the argument and use a single aperture.

#### Motion primitives -- protected

These are called only from `abstract_robot::pick()` and `abstract_robot::place()`, never from outside the class hierarchy.

```cpp
void transfer(const Eigen::Vector3d &target) override;
void transfer(const Eigen::Affine3d &target) override;
void move_without_object_down_or_up(const Eigen::Affine3d &target) override;
void move_with_object_down(const Eigen::Affine3d &target, Entity::kind kind) override;
```

- `transfer(Vector3d)` -- move the TCP to `target` (world-frame XYZ) while keeping the current orientation. Used to sweep horizontally at transfer height (0.3 m above the table).
- `transfer(Affine3d)` -- move the TCP to a full 6-DOF pose. This is the main horizontal transit move; use a relatively fast motion (the Franka implementation uses speed factor 0.4).
- `move_without_object_down_or_up(Affine3d)` -- slow vertical descent or ascent to `target`. Called both when approaching an object before grasping and when retreating after placing. Use a slow speed (Franka uses 0.1) and avoid sudden accelerations.
- `move_with_object_down(Affine3d, kind)` -- same as above but the gripper is holding an object of type `kind`. The Franka implementation does not distinguish between the two (it calls `move_without_object_down_or_up` directly), but you may use this to apply different force profiles or compliance settings when carrying an object.

#### Exploration

```cpp
int exploration_poses() override;
void move_to_exploration_pose(int i) override;
```

- `exploration_poses()` -- return the number of camera viewpoints. The Franka implementation returns 5. A value of 1 is valid if a single top-down view covers the whole workspace.
- `move_to_exploration_pose(int i)` -- move to the i-th viewpoint (0-indexed). After this call, a camera frame captured by the base class will observe part of the workspace. The poses should collectively give full coverage of all object positions defined in the task description.

The viewpoints must be chosen such that the camera (mounted on or near the robot) can see the relevant workspace areas. After each move, `abstract_robot::explore()` calls `world_T_camera()` to get the current camera-to-world transform; make sure FK is up to date before returning from `move_to_exploration_pose`.

#### Grasp pose computation

```cpp
Eigen::Affine3d tcp_T_entity(const Entity &entity) const override;
```

This is the most complex method. It must return the TCP pose (in **world frame**) that correctly grasps `entity`. The returned pose is used directly as the `approach_pose` in `pick()` and `place()`.

Contract:
- `result.translation()` -- TCP position in world frame when the gripper is correctly positioned to grasp the object (before descending by `pick_offset`).
- `result.linear()` -- TCP orientation in world frame. The Z-axis of the TCP frame should point downward (into the table); the Franka Hand has its approach axis along -Z with the fingers in the XY plane.
- The entity's position is available as `entity.pose_.translation()` (world frame).
- The entity's detected rotation about world Z is available via `entity.angle_z()`.
- The entity's rotational symmetry (number of equivalent grasp orientations) is available via `entity.symmetry()`.

In `franka_robot`, `possible_tcp_T_entity()` generates all rotationally equivalent approach poses; `tcp_T_entity()` then picks the one whose IK solution is closest to the current joint configuration. For a robot with a different kinematic structure, reachability and preferred orientations will differ, so this method must be adapted accordingly.

#### Camera-to-world transform

```cpp
Eigen::Affine3d world_T_camera() const override;
```

Return the current static transform from camera optical frame to robot base frame. "Current" means: valid at the time the camera is observing the scene (i.e., after `move_to_exploration_pose(i)` has completed).

If the camera is wrist-mounted (as in this project), compute it as:

```cpp
// example for a wrist-mounted camera
Eigen::Affine3d world_T_camera() const override
{
    return current_world_T_flange() * flange_T_camera_;
}
```

`flange_T_camera_` is a static (constant) transform measured once through hand-eye calibration. It describes where the camera is mounted relative to the robot flange. The Franka implementation uses a 4x4 matrix measured during a dedicated calibration run (see `franka_robot.cpp` lines 185-191).

If the camera is fixed in the world (not on the robot), `world_T_camera()` is simply a constant:

```cpp
Eigen::Affine3d world_T_camera() const override
{
    return fixed_world_T_camera_;  // set once in the constructor
}
```

**How to calibrate `flange_T_camera`:**

A standard approach is to use a checkerboard target at a known world-frame position and move the robot to several poses, capturing the camera image at each. The calibration solves AX = XB (or equivalent). Tools such as `easy_handeye` (ROS), `ViSP`, or Kalibr's hand-eye module can produce the 4x4 matrix. Store the result as a constant in your robot class.

---

## Implementing `hardware::camera`

The camera interface is declared in `src/hardware.h`:

```cpp
class camera
{
  public:
    virtual std::pair<cv::Mat, std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>>> record() = 0;
    virtual ~camera() = default;
};
```

`record()` must return:
- A BGR color image (`cv::Mat`, 8UC3).
- A point cloud in **camera optical frame** (`pcl::PointCloud<pcl::PointXYZRGB>`), pixel-aligned with the color image (each point corresponds to the same real-world location as its color pixel).

The base class `abstract_robot` owns a `realsense_camera` instance directly as a private member (`camera_` in `src/abstract_robot.h`). If you want to use a different camera you have two options:

1. **Replace the member** -- change `realsense_camera camera_` to your camera type in `abstract_robot.h` (simplest if you are also replacing the robot class).
2. **Subclass and shadow** -- override `explore()` in your robot subclass to use a different camera, then call the parent `abstract_robot::explore()` mechanism with a wrapper.

The `hardware::camera_realsense` class (`src/camera_realsense.h`) wraps the RealSense SDK and serves as a reference implementation.

---

## Wiring the New Robot into the Application

`hardware_robot_handler` (in `src/core/robot_handler.h`) owns a `std::unique_ptr<benchmark::abstract_robot> bot_` and creates it in its constructor (`src/core/robot_handler.cpp`). To use a new robot:

1. Include your robot header in `robot_handler.cpp`.
2. In `hardware_robot_handler::hardware_robot_handler()`, replace:
   ```cpp
   bot_ = std::make_unique<benchmark::franka_robot>();
   ```
   with:
   ```cpp
   bot_ = std::make_unique<benchmark::my_robot>();
   ```

No other changes are required; `explore()` and `handle_op()` in `hardware_robot_handler` call only the `abstract_robot` API.

For simulation (no hardware), `simulated_robot_handler` already provides a stub that sleeps instead of moving and returns an empty entity list from `explore()`. It can be selected via the `connect_to_hardware` property on the `Task` QML element (`src/App/Experiment.qml`).

---

## Motion Contract Summary

The table below summarises the speed and context assumptions made by `abstract_robot::pick()` and `abstract_robot::place()`. Your implementations must honour these.

| Method | When called | Expected behaviour |
|---|---|---|
| `transfer(Affine3d)` | Horizontal transit at 0.3 m height | Move at a brisk speed; no payload in gripper |
| `move_without_object_down_or_up` | Vertical descent/ascent | Slow, no payload (or unknown payload); compliant |
| `move_with_object_down` | Vertical descent while holding object | Slow; gripper is closed; handle carefully |
| `open_gripper` / `close_gripper` | Bottom of descent | Block until gripper action completes |

The transfer height (`transfer_z = 0.3` in `abstract_robot.h`) is the Z coordinate (world frame) used for all horizontal transits. The `pick_offset` and `place_offset` functions (in `abstract_robot.cpp`) define how far the TCP descends below the approach pose to engage with each object type:

| Entity kind | `pick_offset` | `place_offset` |
|---|---|---|
| gear | 0.025 m | -- (default 0) |
| conductor | 0.015 m | 0.020 m |
| cube / long_cube | 0.035 m | 0.025 m |

These values were tuned for the Franka Hand on the specific assembly board. If your end effector has a different jaw geometry, adjust these constants in `src/abstract_robot.cpp`.

---

## Checklist for a New Robot Adapter

- [ ] Measured and set `flange_T_tcp` (including any adapter/sensor offsets).
- [ ] FK is available and `current_world_T_flange()` is correct (or equivalent).
- [ ] `world_T_camera()` returns a verified transform (test by printing detected entity positions and checking against known physical locations).
- [ ] `exploration_poses()` returns at least 1; all viewpoints together cover the full workspace.
- [ ] `tcp_T_entity()` produces reachable poses for all entity kinds used in the task.
- [ ] `move_to_start_pose()` brings the robot to a safe configuration that does not occlude the workspace.
- [ ] `open_gripper` / `close_gripper` block until the gripper action is complete.
- [ ] Replaced `franka_robot` with your class in `hardware_robot_handler::hardware_robot_handler()`.
- [ ] Verified in simulation (`connect_to_hardware: false`) before running with hardware.
- [ ] Ran a full exploration pass and checked that `logs/exploration/image_*.png` show the expected workspace from each viewpoint.
