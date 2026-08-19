#pragma once

#include <optional>
#include <vector>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <opencv2/core/mat.hpp>

#include "entity.h"

namespace benchmark {

/**
 * @brief Aggregates all OpenCV contour data for a single segmented color region.
 *
 * Built from the raw OpenCV contour hierarchy; stores the bounding OOBB, the
 * concave/non-concave point classification used for type disambiguation, and
 * any child contours (holes inside the outer boundary).
 */
struct Contour
{
    Contour(const std::vector<std::vector<cv::Point>> &contours,
        const std::vector<cv::Vec4i> &hierarchy,
        int idx,
        Entity::color color,
        const pcl::PointCloud<pcl::PointXYZRGB> &flat_cloud,
        const cv::Size &image_size);

    cv::Mat image;
    std::vector<std::vector<cv::Point>> contours_;
    Entity::color color;
    std::vector<cv::Point> points;
    OOBB oobb;

    std::vector<cv::Point> concave_points;
    std::vector<cv::Point> non_concave_points;

    std::vector<Contour> children;
};

/**
 * @brief Refines an initial entity pose estimate by fitting a plane to the
 *        point cloud region under the contour.
 */
void pose_refinement(Entity &entity, pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud);

/** @brief Estimates the 6-DOF pose of a cube from its point cloud and contour. */
std::optional<Entity> estimate_cube_pose(
    const std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> &cloud,
    const Contour &contour);

/** @brief Estimates the pose of the corner reference marker used to localise the assembly area. */
std::optional<Entity> estimate_corner_marker_pose(
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const Contour &contour);

/** @brief Estimates the 6-DOF pose of a long (2x1) cube from its point cloud and contour. */
std::optional<Entity> estimate_long_cube_pose(
    const std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> &cloud,
    const Contour &contour);

/** @brief Estimates the 6-DOF pose of a pin from its point cloud and contour. */
std::optional<Entity> estimate_pin_pose(
    const std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> &cloud,
    const Contour &contour);

/** @brief Estimates the 6-DOF pose of a gear from its point cloud and contour. */
std::optional<Entity> estimate_gear_pose(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const Contour &c);

/** @brief Estimates the pose of a baseplate hole (gear slot / circuit connector socket). */
Entity estimate_hole_pose(pcl::PointCloud<pcl::PointXYZRGB> scene, const Contour &contour);

/** @brief Estimates the 6-DOF pose of a conductor from its point cloud and contour. */
std::optional<Entity> estimate_conductor_pose(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const Contour &contour);

/**
 * @brief Infers the baseplate kind from the color of the corner marker detected on it.
 * @return The baseplate entity kind, or nullopt if the color does not correspond to a baseplate marker.
 */
std::optional<Entity::kind> baseplate_marker(Entity::color color);

/**
 * @brief Disambiguates a contour as either a conductor or a gear based on its shape.
 * @return Entity::kind::conductor or Entity::kind::gear, or nullopt if neither matches.
 */
std::optional<Entity::kind> conductor_or_gear(const std::vector<cv::Point> &contour);

}// namespace benchmark