#pragma once

#include <chrono>

#include <Eigen/Geometry>
#include <opencv2/core.hpp>
#include <pcl/common/common.h>


/**
 * @brief Abstract camera interface that returns a registered color+depth pair.
 *
 * Concrete implementations (realsense_camera, camera_realsense) wrap a physical
 * Intel RealSense sensor.  The point cloud is in the camera's optical frame;
 * callers must apply abstract_robot::world_T_camera() to obtain world-frame points.
 */
class camera
{
  public:
    /**
     * @brief Captures a single aligned color-and-depth frame.
     *
     * @return A pair of:
     *   - cv::Mat: BGR color image
     *   - PointCloud<PointXYZRGB>: depth point cloud in camera coordinates,
     *     pixel-aligned with the color image so each point maps to the same
     *     real-world location as its corresponding color pixel.
     */
    virtual std::pair<cv::Mat, std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>>> record() = 0;
    virtual ~camera() = default;
};
