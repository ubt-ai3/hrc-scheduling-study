#include "view_frustum.h"

#include <utility>


namespace hardware {
view_frustum::view_frustum(std::vector<Eigen::Vector4d> limiting_planes,
    const Eigen::Affine3d &cam_T_world)
    : m_limiting_planes(std::move(limiting_planes)), m_cam_T_world(cam_T_world)
{
    for (auto &plane : m_limiting_planes) {
        const auto normal_vector(plane.head(3));
        plane /= normal_vector.norm();
    }
}

void view_frustum::update_camera_pose(const Eigen::Affine3d &cam_T_world)
{
    m_cam_T_world = cam_T_world;
}

Eigen::Vector3d view_frustum::camera_origin() const { return m_cam_T_world.translation(); }

bool view_frustum::contains(const Eigen::Vector3d &bbox_min,
    const Eigen::Vector3d &bbox_max,
    double precision) const
{
    const double extent_x = std::abs(bbox_max.x() - bbox_min.x());
    const double extent_y = std::abs(bbox_max.y() - bbox_min.y());

    std::vector<Eigen::Vector3d> all_bbox_points;
    all_bbox_points.emplace_back(bbox_min);
    all_bbox_points.emplace_back(bbox_min + Eigen::Vector3d(extent_x, 0, 0));
    all_bbox_points.emplace_back(bbox_min + Eigen::Vector3d(0, extent_y, 0));
    all_bbox_points.emplace_back(bbox_min + Eigen::Vector3d(extent_x, extent_y, 0));
    all_bbox_points.emplace_back(bbox_max);
    all_bbox_points.emplace_back(bbox_max - Eigen::Vector3d(extent_x, 0, 0));
    all_bbox_points.emplace_back(bbox_max - Eigen::Vector3d(0, extent_y, 0));
    all_bbox_points.emplace_back(bbox_max - Eigen::Vector3d(extent_x, extent_y, 0));

    for (auto &point : all_bbox_points) {
        {
            point = m_cam_T_world * point;
        }
    }

    for (const auto &point : all_bbox_points) {
        for (const auto &plane : m_limiting_planes) {
            const double dist = point.dot(plane.head(3)) - plane(3);
            if (dist <= precision) {
                return false;
            }
        }
    }

    return true;
}
}// namespace hardware