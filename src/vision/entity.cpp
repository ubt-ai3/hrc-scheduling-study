#include "entity.h"
#include "entity.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <pcl/common/io.h>
#include <pcl/common/point_tests.h>
#include <pcl/features/moment_of_inertia_estimation.h>


#include <pcl/visualization/pcl_visualizer.h>
#include "visualization.h"

namespace benchmark {

double Entity::angle_z() const
{
    const auto angle = std::atan2(pose_.matrix()(1, 0), pose_.matrix()(0, 0));

    return std::fmod(angle + 2 * M_PI, 2 * M_PI);
}

Entity::Entity(Entity::kind k, Entity::color c, Eigen::Vector3d position, double rotation_z_axis)
    : color_(c), kind_(k), pose_(Eigen::Affine3d::Identity())
{
    pose_.translation() = position;
    pose_.rotate(Eigen::AngleAxisd(rotation_z_axis, Eigen::Vector3d::UnitZ()));
}

Entity::Entity(Entity::kind k, Entity::color c, const Eigen::Affine3d &pose)
    : color_(c), kind_(k), pose_(pose)
{}


int Entity::symmetry() const
{
    switch (kind_) {
    case benchmark::Entity::kind::conductor:
    case benchmark::Entity::kind::long_cube:
        return 2;
    case benchmark::Entity::kind::cube: 
        return 2;// avoid rotation by 90° since then gripper might collide width other block

    case benchmark::Entity::kind::gear:
    case benchmark::Entity::kind::corner_marker:
    case benchmark::Entity::kind::pin:
    case benchmark::Entity::kind::gear_slot:
    case benchmark::Entity::kind::circuit_connector:
    default:
        return 1;
    }
}

bool Entity::operator==(const Entity &other) const
{
    return color_ == other.color_ && kind_ == other.kind_ && pose_.isApprox(other.pose_);
}

void Entity::transform_pcl_T_web() { pose_ = pcl_T_web.at(kind_) * pose_; }

void Entity::transform_web_T_pcl() { pose_ = pcl_T_web.at(kind_).inverse() * pose_; }


std::ostream &operator<<(std::ostream &os, const Entity::kind &kind)
{
    switch (kind) {
    case Entity::kind::gear:
        os << "gear";
        break;
    case Entity::kind::gear_slot:
        os << "gear_slot";
        break;
    case Entity::kind::circuit_connector:
        os << "circuit_connector";
        break;
    case Entity::kind::conductor:
        os << "conductor";
        break;
    case Entity::kind::corner_marker:
        os << "corner_marker";
        break;
    case Entity::kind::cube:
        os << "cube";
        break;
    case Entity::kind::pin:
        os << "pin";
        break;
    case Entity::kind::long_cube:
        os << "long cube";
        break;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const Entity::color &color)
{
    switch (color) {
    case Entity::color::orange:
        os << "orange";
        break;
    case Entity::color::green:
        os << "green";
        break;
    case Entity::color::blue:
        os << "blue";
        break;
    }
    return os;
}

const std::map<Entity::kind, Eigen::Affine3d> Entity::pcl_T_web = []() {
    std::map<kind, Eigen::Affine3d> temp;
    temp[kind::gear_slot] = Eigen::Affine3d::Identity();
    temp[kind::circuit_connector] = Eigen::Affine3d::Identity();
    temp[kind::pin] = Eigen::Affine3d::Identity();
    temp[kind::gear] = Eigen::Translation3d(0, 0, 0.04);
    temp[kind::conductor] = Eigen::Translation3d(0, 0, 0.028);
    temp[kind::corner_marker] = Eigen::Translation3d(0, 0, 0.02);

    temp[kind::cube] = Eigen::Translation3d(0, 0, 0.059);
    temp[kind::long_cube] = Eigen::Translation3d(0, 0, 0.059);
    return temp;
}();


OOBB OOBB::calculate(const pcl::PointCloud<pcl::PointXYZRGB> &cloud, const cv::Mat &contour_img)
{
    std::vector<cv::Point2i> locations;
    cv::findNonZero(contour_img, locations);

    auto indices = std::make_shared<pcl::Indices>();
    for (const auto &point : locations) {
        indices->push_back(point.y * cloud.width + point.x);
    }
    auto gray_cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    pcl::copyPointCloud(cloud, *gray_cloud);

    // remove NaN values
    indices->erase(
        std::remove_if(indices->begin(),
            indices->end(),
            [&gray_cloud](size_t index) { return pcl::isFinite(gray_cloud->at(index)) == false; }),
        indices->end());


    pcl::MomentOfInertiaEstimation<pcl::PointXYZ> feature_extractor;
    feature_extractor.setInputCloud(gray_cloud);
    feature_extractor.setIndices(indices);
    feature_extractor.compute();

    pcl::PointXYZ min_point_OBB;
    pcl::PointXYZ max_point_OBB;
    pcl::PointXYZ position_OBB;
    Eigen::Matrix3f rotational_matrix_OBB;

    feature_extractor.getOBB(min_point_OBB, max_point_OBB, position_OBB, rotational_matrix_OBB);
    return { min_point_OBB, max_point_OBB, position_OBB, rotational_matrix_OBB };
}


Eigen::Vector3f OOBB::eigen_position() const { return { position_.x, position_.y, position_.z }; }

float OOBB::x_length() const { return max_point_.x - min_point_.x; }

float OOBB::y_length() const { return max_point_.y - min_point_.y; }

float OOBB::z_length() const { return max_point_.z - min_point_.z; }

std::vector<float> OOBB::sorted_dims() const
{
    std::vector<float> res{ x_length(), y_length(), z_length() };
    std::sort(res.begin(), res.end());
    return res;
}
}// namespace benchmark