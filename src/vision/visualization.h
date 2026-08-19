#pragma once

#include <string>
#include <chrono>

#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/common/transforms.h>
#include <pcl/io/pcd_io.h>

#include <Eigen/Core>

#include "vision.h"

/** @brief Debug visualization helpers that draw entities and geometry into a PCLVisualizer. */
namespace benchmark::util {

/**
 * @brief Loads a mesh from a PLY/STL file and adds it to @p viewer at @p pose.
 * @param color  RGB colour in [0, 255] range.
 */
void draw_mesh_from_file(pcl::visualization::PCLVisualizer &viewer,
    const Eigen::Affine3d &pose,
    const std::string &path,
    const std::array<double, 3> &color);

/** @brief Adds a small sphere marker at @p position to @p viewer. */
void draw_small_sphere(pcl::visualization::PCLVisualizer &viewer, const pcl::PointXYZ &position);
/** @copydoc draw_small_sphere(pcl::visualization::PCLVisualizer&, const pcl::PointXYZ&) */
void draw_small_sphere(pcl::visualization::PCLVisualizer &viewer, const Eigen::Vector3d &position);
/** @brief Adds a small sphere marker at the translation of @p pose to @p viewer. */
void draw_small_sphere(pcl::visualization::PCLVisualizer &viewer, const Eigen::Affine3d &pose);

/**
 * @brief Draws an entity (coordinate frame + sphere) at its pose in @p viewer.
 * @param color  RGB colour in [0, 255] range.
 */
void draw(pcl::visualization::PCLVisualizer &viewer,
    const Entity &entity,
    const std::array<double, 3> &color = { 255, 255, 255 });

/** @brief Draws the axes and extents of an OOBB in @p viewer. */
void draw(pcl::visualization::PCLVisualizer &viewer, const benchmark::OOBB &oobb);


}// namespace benchmark::util
