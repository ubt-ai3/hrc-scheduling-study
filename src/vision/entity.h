#pragma once

#include <iostream>
#include <map>

#include <Eigen/Dense>

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>

#include <opencv2/core/mat.hpp>


namespace benchmark {

/**
 * @brief Represents a detected assembly part with its color, kind, and 6-DOF world pose.
 *
 * Poses are stored as Eigen::Affine3d transforms (world_T_entity).  The Z-axis of the
 * entity frame points upward; angle_z() returns the yaw angle around the world Z-axis.
 *
 * pcl_T_web / transform_pcl_T_web() / transform_web_T_pcl() convert between the
 * AI3 Benchmark Builder coordinate system (Y-up, used in task_description.json) and
 * the robot coordinate system (Z-up, used at runtime).
 */
struct Entity
{
    friend std::ostream &operator<<(std::ostream &os, const Entity &obj);

    enum class color { orange, green, blue };
    enum class kind {
        gear,
        gear_slot,
        circuit_connector,
        conductor,
        corner_marker,
        cube,
        long_cube,
        pin
    };

    color color_;
    kind kind_;
    Eigen::Affine3d pose_;

    /**
     * @brief Returns the rotation around the z-Axis. Assumed to be in worldcoordinates.
     *
     */
    double angle_z() const;
    Entity() = default;
    Entity(kind k, color c, Eigen::Vector3d position, double rotation_z_axis);
    Entity(kind k, color c, const Eigen::Affine3d &pose);

    int symmetry() const;
    bool operator==(const Entity &other) const;

    // gear_slot and circuit connectors have the same shape, so we need to differentiate based on
    // color
    static constexpr color gear_slot_color = color::blue;
    static constexpr color circuit_connector_color = color::orange;
    static constexpr color pin_color = color::green;

    void transform_pcl_T_web();
    void transform_web_T_pcl();
    static const std::map<Entity::kind, Eigen::Affine3d> pcl_T_web;
};


std::ostream &operator<<(std::ostream &os, const Entity::kind &kind);
std::ostream &operator<<(std::ostream &os, const Entity::color &color);


/**
 * @brief Object-oriented bounding box (OBB) of a segmented part's point cloud.
 *
 * Computed via PCA on the points that fall within a contour mask.  Used by pose
 * estimators to determine part dimensions and orientation.
 */
struct OOBB
{
    const pcl::PointXYZ min_point_;
    const pcl::PointXYZ max_point_;
    const pcl::PointXYZ position_;
    const Eigen::Matrix3f rotational_matrix_;

    [[nodiscard]] Eigen::Vector3f eigen_position() const;
    [[nodiscard]] float x_length() const;
    [[nodiscard]] float y_length() const;
    [[nodiscard]] float z_length() const;

    /** @brief Returns the three dimension lengths sorted in ascending order. */
    [[nodiscard]] std::vector<float> sorted_dims() const;

    /**
     * @brief Computes the OOBB of the points in @p cloud that overlap with @p contour_img.
     * @param contour_img  Binary mask image; non-zero pixels select points from @p cloud.
     */
    static OOBB calculate(const pcl::PointCloud<pcl::PointXYZRGB> &cloud,
        const cv::Mat &contour_img);
};

}// namespace benchmark