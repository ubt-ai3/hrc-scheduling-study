#pragma once

#include <opencv2/core/core.hpp>

#include <Eigen/Core>
#include <pcl/common/common.h>


namespace util {

/** @brief Converts the RGB values of a colored point cloud into a cv::Mat image. */
cv::Mat cv_mat_from_pointcloud(const pcl::PointCloud<pcl::PointXYZRGB> &cloud);

/**
 * @brief Fits a plane to a set of 3D points using SVD.
 * @return Plane coefficients (a, b, c, d) such that a*x + b*y + c*z = d.
 */
Eigen::Vector4d fit_plane_svd(const Eigen::Matrix<double, Eigen::Dynamic, 3> &points);

/**
 * @brief Fits a line to a set of 3D points using SVD.
 * @return Line as a 6-vector (point on line [0:3], direction [3:6]).
 */
Eigen::Matrix<double, 6, 1> fit_line_svd(const Eigen::Matrix<double, Eigen::Dynamic, 3> &points);

/**
 * @brief Computes the intersection of a parametric line and a plane.
 * @param line  6-vector (point [0:3], direction [3:6]) as returned by fit_line_svd.
 * @param plane 4-vector (a, b, c, d) as returned by fit_plane_svd.
 */
Eigen::Vector3d intersect_line_plane(const Eigen::Matrix<double, 6, 1> &line,
    const Eigen::Vector4d &plane);

/**
 * @brief Returns the camera ray through a pixel as a line in world coordinates.
 * @param pixel                  Pixel coordinates (u, v).
 * @param camera_matrix          Camera projection matrix (3x4 or 4x4).
 * @param inverse_camera_matrix  Pre-computed inverse of camera_matrix.
 */
Eigen::Matrix<double, 6, 1> get_camera_ray(const Eigen::Vector2d &pixel,
    const Eigen::Matrix4d &camera_matrix,
    const Eigen::Matrix4d &inverse_camera_matrix);

/** @brief Extracts the camera centre (optical origin) from a camera projection matrix. */
Eigen::Vector3d get_camera_center(const Eigen::Matrix4d &camera_matrix);

/**
 * @brief Back-projects a pixel at a known depth to a 3D world point.
 * @param pixel                  Pixel coordinates (u, v).
 * @param depth                  Depth value in metres.
 * @param inverse_camera_matrix  Pre-computed inverse of the camera projection matrix.
 */
Eigen::Vector3d reproject_point(const Eigen::Vector2d &pixel,
    double depth,
    const Eigen::Matrix4d &inverse_camera_matrix);

/**
 * @brief Estimates a camera projection matrix from a coloured point cloud.
 *
 * Uses SVD-based plane and line fitting on the cloud geometry to recover the
 * intrinsic/extrinsic projection matrix.
 */
Eigen::Matrix4d estimate_camera_projection_matrix(const pcl::PointCloud<pcl::PointXYZRGB> &cloud);
}// namespace util