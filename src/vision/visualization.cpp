#include "visualization.h"
#include "visualization.h"

namespace benchmark::util {


void draw_mesh_from_file(pcl::visualization::PCLVisualizer &viewer,
    const Eigen::Affine3d &pose,
    const std::string &path,
    const std::array<double, 3> &color)
{
    using namespace std::literals;


    const auto cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    pcl::io::loadPCDFile(path, *cloud);

    pcl::transformPointCloud(*cloud, *cloud, pose);

    pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> single_color(
        cloud, color[0], color[1], color[2]);
    viewer.addPointCloud(cloud, single_color, "gear_"s + std::to_string(std::rand()));
}

void draw_small_sphere(pcl::visualization::PCLVisualizer &viewer, const pcl::PointXYZ &position)
{
    using namespace std::literals;
    const auto name = "sphere_"s + std::to_string(std::rand());
    viewer.addSphere<pcl::PointXYZ>(position, 0.007, name);
}


void draw_small_sphere(pcl::visualization::PCLVisualizer &viewer, const Eigen::Vector3d &position)
{

    draw_small_sphere(viewer, pcl::PointXYZ(position.x(), position.y(), position.z()));
}

void draw_small_sphere(pcl::visualization::PCLVisualizer &viewer, const Eigen::Affine3d &pose)
{
    draw_small_sphere(viewer, pose.translation());
}


void draw(pcl::visualization::PCLVisualizer &viewer,
    const Entity &entity,
    const std::array<double, 3> &color)
{
    switch (entity.kind_) {
    case Entity::kind::circuit_connector:
    case Entity::kind::gear_slot:
        draw_small_sphere(viewer, entity.pose_);
        break;
    case Entity::kind::gear:
        draw_mesh_from_file(
            viewer, entity.pose_, "./data/objectDatabase/GearRedMediumBevels.pcd", color);
        break;
    case Entity::kind::conductor:
        draw_mesh_from_file(
            viewer, entity.pose_, "./data/objectDatabase/circuits-ConductorModel.pcd", color);
        break;
    case Entity::kind::corner_marker:
        draw_mesh_from_file(viewer, entity.pose_, "./data/objectDatabase/model.pcd", color);
        break;
    case Entity::kind::pin:// TODO
    case Entity::kind::cube:
        draw_mesh_from_file(viewer, entity.pose_, "./data/objectDatabase/Cube.pcd", color);
        break;
    case Entity::kind::long_cube:
        draw_mesh_from_file(viewer, entity.pose_, "./data/objectDatabase/longCube.pcd", color);
        break;
    }
}

void draw(pcl::visualization::PCLVisualizer &viewer, const benchmark::OOBB &oobb)
{
    using namespace std::literals;
    const auto name = "sphere_"s + std::to_string(std::rand());
    const auto min = oobb.min_point_;
    const auto max = oobb.max_point_;

    viewer.addCube(Eigen::Vector3f(oobb.position_.x, oobb.position_.y, oobb.position_.z),
        Eigen::Quaternionf(oobb.rotational_matrix_),
        max.x - min.x,
        max.y - min.y,
        max.z - min.z,
        name);
    viewer.setShapeRenderingProperties(pcl::visualization::PCL_VISUALIZER_REPRESENTATION,
        pcl::visualization::PCL_VISUALIZER_REPRESENTATION_WIREFRAME,
        name);
}

}// namespace benchmark::util