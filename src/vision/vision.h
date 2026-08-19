#pragma once

#include <map>
#include <optional>
#include <ostream>
#include <filesystem>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <Eigen/Dense>
#include <opencv2/core/mat.hpp>

#include <entity.h>
#include <pose_estimation.h>

namespace benchmark {

/**
 * @brief Main entry point for object detection in the HRC assembly scene.
 *
 * Given an aligned RGB image and its corresponding colored point cloud, Detection
 * segments the image by color (orange, green, blue), extracts contours, and runs
 * per-kind pose estimators (gear, conductor, cube, etc.) on each contour in parallel.
 *
 * HSV thresholds are loaded from a JSON config file at construction and can be
 * tuned with the ColorCalibration tool.
 */
class Detection
{
  public:
    /**
     * @brief Detects all recognisable entities in @p cloud_img / @p cloud.
     *
     * Runs color segmentation and pose estimation in parallel for each color channel.
     * @param cloud_img  BGR color image aligned with the point cloud.
     * @param cloud      Colored point cloud in camera coordinates.
     * @return All successfully detected entities with estimated 6-DOF poses.
     */
    std::vector<Entity> find_objects(const cv::Mat &cloud_img,
        const pcl::PointCloud<pcl::PointXYZRGB>::Ptr &cloud);

    /**
     * @brief Loads HSV thresholds from @p camera_config (JSON).
     *
     * The config is typically `data/thresholds.realsense.json` or a file produced
     * by the ColorCalibration tool.
     */
    Detection(const std::filesystem::path &camera_config);

    /**
     * @brief Segments @p image by the HSV range for color @p c and writes a binary mask to @p out_img.
     *
     * The image is assumed to be in RGB byte order.
     */
    void segment(const cv::Mat &image, cv::Mat &out_img, Entity::color c) const;

    /**
     * @brief Extracts the top-level (outermost) color contours from @p cloud_img.
     * @return One Contour per detected outer contour, including its child contours.
     */
    std::vector<Contour> top_contours(const cv::Mat &cloud_img,
        const pcl::PointCloud<pcl::PointXYZRGB> &cloud);

    /**
     * @brief Estimates the 6-DOF pose of an entity of known @p type within @p contour.
     * @return The entity with pose filled in, or nullopt if estimation fails.
     */
    std::optional<Entity> estimate_pose(Entity::kind type,
        const pcl::PointCloud<pcl::PointXYZRGB>::Ptr &cloud,
        const Contour &contour) const;

    /**
     * @brief Classifies a contour as a specific entity kind based on its geometry.
     * @return The inferred kind, or nullopt if the contour does not match any known kind.
     */
    std::optional<Entity::kind> estimate_type(const Contour &contour) const;


  private:
    cv::Scalar hsv_blue_lo_ = cv::Scalar(104, 204, 88);
    cv::Scalar hsv_blue_hi_ = cv::Scalar(128, 255, 255);

    cv::Scalar hsv_orange_lo_ = cv::Scalar(5, 88, 130);
    cv::Scalar hsv_orange_hi_ = cv::Scalar(15, 255, 255);

    cv::Scalar hsv_green_lo_ = cv::Scalar(60, 60, 60);
    cv::Scalar hsv_green_hi_ = cv::Scalar(90, 210, 180);
};


}// namespace benchmark
