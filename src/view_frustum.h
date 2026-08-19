#pragma once

#include <vector>

#include <Eigen/Dense>

namespace hardware {

/**
 * @brief Axis-aligned view frustum for camera visibility tests.
 *
 * Stores a set of half-space planes that define the camera frustum.  Each plane
 * has its inward normal encoded as a 4-vector (a, b, c, d) satisfying:
 *   a*x + b*y + c*z - d = 0
 * A point is inside the frustum if it is on the positive (inward) side of all planes.
 *
 * The frustum is expressed in world coordinates; call update_camera_pose() whenever
 * the camera moves.
 */
class view_frustum
{
  public:
    /**
     * @brief Constructs the frustum from a set of half-space planes and an initial camera pose.
     *
     * Plane normals must point inward.  Planes need not be normalised.
     * @param limiting_planes  Inward-facing planes in camera frame.
     * @param cam_T_world      Transform from world frame to camera frame.
     */
    view_frustum(std::vector<Eigen::Vector4d> limiting_planes, const Eigen::Affine3d &cam_T_world);

    /** @brief Updates the frustum when the camera has moved to a new pose. */
    void update_camera_pose(const Eigen::Affine3d &cam_T_world);

    /**
     * @brief Tests whether an axis-aligned bounding box intersects the frustum.
     * @param bbox_min   Minimum corner of the AABB in world coordinates.
     * @param bbox_max   Maximum corner of the AABB in world coordinates.
     * @param precision  Margin to expand the frustum planes (metres).
     * @return true if the AABB is (partially) inside the frustum.
     */
    bool contains(const Eigen::Vector3d &bbox_min,
        const Eigen::Vector3d &bbox_max,
        double precision = 0) const;

    /** @brief Returns the camera optical origin in world coordinates. */
    Eigen::Vector3d camera_origin() const;

  private:
    std::vector<Eigen::Vector4d> m_limiting_planes;

    Eigen::Affine3d m_cam_T_world;
};
}// namespace hardware
