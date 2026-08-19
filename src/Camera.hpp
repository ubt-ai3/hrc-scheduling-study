#pragma once

#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <Eigen/Geometry>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include "hardware.h"

typedef pcl::PointXYZRGB RGB_Cloud;
typedef pcl::PointCloud<RGB_Cloud> point_cloud;
typedef point_cloud::Ptr cloud_pointer;
typedef point_cloud::Ptr prevCloud;

/**
 * @brief Legacy RealSense camera wrapper (camera interface implementation).
 *
 * This class is the original camera implementation used by abstract_robot.
 * Prefer hardware::camera_realsense (which wraps camera_3d_vision_hardware) for
 * new code — it provides a cleaner API and frustum/clipping support.
 *
 * Captures XYZRGB point clouds and aligned BGR color images via librealsense2.
 */
class realsense_camera final : public camera
{
  public:
    /** @brief Opens the first available RealSense device at 1080p color / best depth profile. */
    realsense_camera();
    /** @brief Opens a specific device/sensor/profile combination by index. */
    realsense_camera(std::vector<int> dev, std::vector<int> sensor, std::vector<int> prof);

    /**
     * @brief Captures a frame with the current profile and stores it internally.
     *
     * Call get_color() / get_depth() after this to retrieve the data.
     */
    void take_picture();

    /** @brief Returns the last captured color image (shallow reference, not a copy). */
    cv::Mat get_color();
    /** @brief Returns the last captured depth point clouds. */
    std::vector<cloud_pointer> get_depth();

    void print_profiles();

    void take_picture_with_profile(int dev, int sensor, int prof);

    /** @brief Back-projects pixel (@p x, @p y) to a 3D point in the camera coordinate frame. */
    Eigen::Vector3d get_camera_coordinate_of_pixel(int x, int y);

    /** @brief Projects a 3D camera-frame point (@p x, @p y, @p z) to a 2D pixel coordinate. */
    cv::Point2f get_pixel_coordinate_of_camera_point(float x, float y, float z);

    virtual std::pair<cv::Mat, std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>>>
        record() override;

    ~realsense_camera();

  private:
    rs2::frameset fr_;
    cv::Mat color_mat_;
    std::vector<cloud_pointer> depth_mat_;
    rs2::pipeline pipe_;
    bool warmed_up_;
    bool take_special_conf_;
    rs2::config conf_;
    rs2::align *align_to;
    rs2_intrinsics intr_;

    // Inherited via camera
};
