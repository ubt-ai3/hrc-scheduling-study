#include "camera_util.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <pcl/common/eigen.h>
#include <pcl/common/point_tests.h>

namespace util {
cv::Mat cv_mat_from_pointcloud(const pcl::PointCloud<pcl::PointXYZRGB> &cloud)
{
    cv::Mat image(cloud.height, cloud.width, CV_8UC4);

#pragma omp parallel for
    for (int y = 0; y < image.rows; y++) {
        for (int x = 0; x < image.cols; x++) {
            auto &point = cloud.at(x, y);
            image.at<cv::Vec4b>(y, x)[0] = point.b;
            image.at<cv::Vec4b>(y, x)[1] = point.g;
            image.at<cv::Vec4b>(y, x)[2] = point.r;
            image.at<cv::Vec4b>(y, x)[3] = point.a;
        }
    }

    return image;
}

Eigen::Vector4d fit_plane_svd(const Eigen::Matrix<double, Eigen::Dynamic, 3> &points)
{
    const Eigen::Vector3d clust_centroid = points.colwise().mean();
    const Eigen::Matrix<double, Eigen::Dynamic, 3> centered =
        points.rowwise() - clust_centroid.transpose();
    const Eigen::Matrix3d clust_cov =
        centered.adjoint() * centered / static_cast<double>(points.rows() - 1);

    // calc smallest eigen value and eigen vectors of cov matrix
    double eigen_value;
    Eigen::Vector3d eigen_vector;
    pcl::eigen33(clust_cov, eigen_value, eigen_vector);

    // bring together plane
    Eigen::Vector4d plane;
    if (clust_centroid.dot(eigen_vector))// plane normal points to view
        plane.head<3>() = -1 * eigen_vector;
    else
        plane.head<3>() = eigen_vector;
    plane[3] = -clust_centroid.dot(plane.head<3>());

    return plane.cast<double>();
}

Eigen::Matrix<double, 6, 1> fit_line_svd(const Eigen::Matrix<double, Eigen::Dynamic, 3> &points)
{
    const Eigen::Vector3d clust_centroid = points.colwise().mean();
    const Eigen::Matrix<double, Eigen::Dynamic, 3> centered =
        points.rowwise() - clust_centroid.transpose();
    const Eigen::Matrix3d clust_cov =
        (centered.adjoint() * centered) / static_cast<double>(points.rows() - 1);

    // calc eigen values and eigen vectors of cov matrix
    Eigen::Vector3d eigen_values;
    Eigen::Matrix3d eigen_vectors;
    pcl::eigen33(clust_cov, eigen_vectors, eigen_values);

    // bring together plane
    Eigen::Matrix<double, 6, 1> line;
    line.head<3>() = clust_centroid;
    line.tail<3>() = eigen_vectors.row(2);
    return line;
}

Eigen::Vector3d intersect_line_plane(const Eigen::Matrix<double, 6, 1> &line,
    const Eigen::Vector4d &plane)
{
    // A - u * ((n * A + d) / n * u)
    return line.head<3>()
           - ((plane.head<3>().dot(line.head<3>()) + plane[3])
                 / plane.head<3>().dot(line.tail<3>()))
                 * line.tail<3>();
}

Eigen::Matrix<double, 6, 1> get_camera_ray(const Eigen::Vector2d &pixel,
    const Eigen::Matrix4d &camera_matrix,
    const Eigen::Matrix4d &inverse_camera_matrix)
{
    Eigen::Matrix<double, 6, 1> line;
    line.head<3>() = get_camera_center(camera_matrix);
    line.tail<3>() = reproject_point(pixel, 1.0, inverse_camera_matrix);
    line.tail<3>() = (line.tail<3>() - line.head<3>()).normalized();
    return line;
}

Eigen::Vector3d get_camera_center(const Eigen::Matrix4d &camera_matrix)
{
    Eigen::Matrix4d tmp(camera_matrix);
    tmp.row(2) = tmp.row(3);
    return -1.0 * tmp.topLeftCorner<3, 3>().inverse() * tmp.col(3).head<3>();
}

Eigen::Vector3d reproject_point(const Eigen::Vector2d &pixel,
    double depth,
    const Eigen::Matrix4d &inverse_camera_matrix)
{
    const Eigen::Vector4d in(pixel.x() * depth, pixel.y() * depth, 1, depth);
    return ((inverse_camera_matrix * in).hnormalized());
}

Eigen::Matrix4d estimate_camera_projection_matrix(const pcl::PointCloud<pcl::PointXYZRGB> &cloud)
{
    // cloud not organized
    if (cloud.height <= 1) return Eigen::Matrix4d::Zero();

    // count valid points
    int valid_points = 0;
    for (int x = 0; x < static_cast<int>(cloud.width); ++x)
        for (int y = 0; y < static_cast<int>(cloud.height); ++y) {
            const auto &p = cloud.at(x, y);
            if (pcl::isFinite(p)) ++valid_points;
        }

    // setup 2 linear systems: A * x_1 = b_1 , A * x_2 = b_2
    // with x_1 = (f_x, s_x, c_x, d)^T
    // and x_2 = (s_y, f_y, c_y, h)^T
    // row of A: (X, Y, Z, 1)
    // row of b_1: (x * Z)
    // row of b_2: (y * Z)
    Eigen::MatrixXd A(valid_points, 4);
    Eigen::VectorXd b_x(valid_points);
    Eigen::VectorXd b_y(valid_points);

    // fill system
    valid_points = 0;
    for (int x = 0; x < static_cast<int>(cloud.width); ++x)
        for (int y = 0; y < static_cast<int>(cloud.height); ++y) {
            const auto &p = cloud.at(x, y);
            if (pcl::isFinite(p)) {
                A.row(valid_points) = p.getVector4fMap().cast<double>().transpose();
                b_x(valid_points) = x * p.z;
                b_y(valid_points) = y * p.z;
                ++valid_points;
            }
        }

    // solve
    Eigen::Vector4d result_x = A.colPivHouseholderQr().solve(b_x);
    Eigen::Vector4d result_y = A.colPivHouseholderQr().solve(b_y);

    // extract
    Eigen::Matrix4d intrinsics = Eigen::Matrix4d::Zero();
    intrinsics.row(0) = result_x.transpose();
    intrinsics.row(1) = result_y.transpose();
    intrinsics(2, 3) = intrinsics(3, 2) = 1;

    return intrinsics;
}
}// namespace util