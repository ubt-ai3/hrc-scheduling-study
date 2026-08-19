#include "vision.h"

#include <filesystem>
#include <fstream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>


#include <nlohmann/json.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/common/point_tests.h>
#include <pcl/common/centroid.h>
#include <pcl/common/intersections.h>
#include <pcl/common/transforms.h>
#include <pcl/registration/icp.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/features/moment_of_inertia_estimation.h>
#include <pcl/filters/filter.h>

#include <spdlog/spdlog.h>

#include "visualization.h"
#include "pose_estimation.h"

namespace benchmark {

Detection::Detection(const std::filesystem::path &camera_config)
{
    auto config = nlohmann::json();
    auto file = std::ifstream(camera_config);
    config << file;

    hsv_blue_lo_ = { config["hsv_blue_lo"][0], config["hsv_blue_lo"][1], config["hsv_blue_lo"][2] };
    hsv_blue_hi_ = { config["hsv_blue_hi"][0], config["hsv_blue_hi"][1], config["hsv_blue_hi"][2] };

    hsv_orange_lo_ = {
        config["hsv_orange_lo"][0], config["hsv_orange_lo"][1], config["hsv_orange_lo"][2]
    };
    hsv_orange_hi_ = {
        config["hsv_orange_hi"][0], config["hsv_orange_hi"][1], config["hsv_orange_hi"][2]
    };

    hsv_green_lo_ = {
        config["hsv_green_lo"][0], config["hsv_green_lo"][1], config["hsv_green_lo"][2]
    };
    hsv_green_hi_ = {
        config["hsv_green_hi"][0], config["hsv_green_hi"][1], config["hsv_green_hi"][2]
    };
}


// image assumend to be in RGB
void Detection::segment(const cv::Mat &image, cv::Mat &out_img, Entity::color c) const
{
    // Convert from BGR to HSV color space
    cv::cvtColor(image, out_img, cv::COLOR_BGR2HSV);
    // Detect the object based on HSV Range Values
    switch (c) {
    case Entity::color::blue:
        inRange(out_img, hsv_blue_lo_, hsv_blue_hi_, out_img);
        break;
    case Entity::color::orange:
        inRange(out_img, hsv_orange_lo_, hsv_orange_hi_, out_img);
        break;
    case Entity::color::green:
        inRange(out_img, hsv_green_lo_, hsv_green_hi_, out_img);
    }

    // remove individual points
    cv::erode(out_img,
        out_img,
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2), cv::Point(1, 1)));

    cv::dilate(out_img,
        out_img,
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2), cv::Point(1, 1)));

    // close holes in contours
    cv::dilate(out_img,
        out_img,
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5), cv::Point(1, 1)));
    
    cv::erode(out_img,
        out_img,
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5), cv::Point(1, 1)));
}


std::optional<Entity::kind> Detection::estimate_type(const Contour &contour) const
{

    constexpr float eps = 0.01f;// cm
    // sort dimensions ascending
    auto dimensions =
        std::vector{ contour.oobb.x_length(), contour.oobb.y_length(), contour.oobb.z_length() };
    std::sort(dimensions.begin(), dimensions.end());

    const auto rect = cv::minAreaRect(contour.points);

    const bool ratio =
        rect.size.width
        == std::clamp(rect.size.width, rect.size.height * 0.8f, rect.size.height * 1.4f);
    {
        const bool size = dimensions[1] == std::clamp(dimensions[1], 0.035f, 0.1f);
        if (ratio && size && contour.children.size() != 0) return Entity::kind::cube;
    }

    if (contour.color == Entity::color::blue
        && dimensions[2] == std::clamp(dimensions[2], 0.07f, 0.16f)
        && dimensions[1] == std::clamp(dimensions[1], 0.03f, 0.5f)
        && conductor_or_gear(contour.points) != Entity::kind::gear) {
        return Entity::kind::long_cube;
    }

    // differentiate type based on real world size
    if (dimensions[2] == std::clamp(dimensions[2], 0.021f - eps, 0.021f + eps)
        && dimensions[1] == std::clamp(dimensions[1], 0.021f - eps, 0.021f + eps)) {
        return baseplate_marker(contour.color);

    } else if (dimensions[2] == std::clamp(dimensions[2], 0.025f, 0.05f * 1.5f)
               && dimensions[1] == std::clamp(dimensions[1], 0.0f, 0.028f * 2.0f)) {
        return conductor_or_gear(contour.points);

    } else if (dimensions[2] == std::clamp(dimensions[2], 0.1414f - eps, 0.1414f + eps)
               && dimensions[1] == std::clamp(dimensions[1], 0.085f - eps, 0.085f + eps)) {
        return Entity::kind::corner_marker;
    }


    return std::nullopt;
}


std::vector<Contour> Detection::top_contours(const cv::Mat &cloud_img,
    const pcl::PointCloud<pcl::PointXYZRGB> &cloud)
{
    auto flat_cloud = pcl::PointCloud<pcl::PointXYZRGB>(cloud);
    for (auto &point : flat_cloud) {
        point.z = 0;
    }


    std::vector<Contour> res;

    for (const auto color : { Entity::color::blue, Entity::color::orange, Entity::color::green }) {
        cv::Mat segmented;
        segment(cloud_img, segmented, color);

        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        auto local_entities = std::vector<Entity>();


        cv::findContours(segmented, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
        // if we find no contours, segment the next color
        if (contours.empty()) {
            continue;
        }


        for (int idx = 0; idx >= 0; idx = hierarchy[idx][0]) {
            res.emplace_back(contours, hierarchy, idx, color, flat_cloud, cloud_img.size());
        }
    }

    return res;
}


std::vector<Entity> Detection::find_objects(const cv::Mat &cloud_img,
    const pcl::PointCloud<pcl::PointXYZRGB>::Ptr &cloud)
{
    using namespace std::literals;

    std::vector<Entity> entities;


    const auto contours = top_contours(cloud_img, *cloud);

    for (const auto &c : contours) {
        const auto type = estimate_type(c);
        if (!type.has_value()) {
            continue;
        }
        const auto entity = estimate_pose(*type, cloud, c);
        if (!entity.has_value()) {
            continue;
        }
        entities.push_back(*entity);
    }
    return entities;
}


std::ostream &operator<<(std::ostream &os, const Entity &obj)
{
    return os << "Entity: [kind_: " << std::left << std::setw(20) << obj.kind_
              << ", color_: " << std::left << std::setw(8) << obj.color_
              << ", rotation: " << std::left << std::setw(3)
              << static_cast<int>(obj.angle_z() * (180 / M_PI))
              << ", translation: " << obj.pose_.translation().transpose() << "]";
}

std::ostream &operator<<(std::ostream &os, const OOBB &obj)
{
    return os << "Object Oriented Bounding Box:\n"
              << " position_: " << obj.position_ << "\n"
              << " x_length: " << obj.x_length() << " y_length: " << obj.y_length()
              << " z_length: " << obj.z_length();
}


std::optional<Entity> Detection::estimate_pose(Entity::kind type,
    const pcl::PointCloud<pcl::PointXYZRGB>::Ptr &cloud,
    const Contour &contour) const
{
    try {
    switch (type) {
    case Entity::kind::gear:
        return estimate_gear_pose(cloud, contour);
    case Entity::kind::gear_slot:
    case Entity::kind::circuit_connector: {
        auto res = estimate_hole_pose(*cloud, contour);
        res.kind_ = type;
        return res;
    }
    case Entity::kind::conductor:
        return estimate_conductor_pose(cloud, contour);
    case Entity::kind::corner_marker:
        return estimate_corner_marker_pose(cloud, contour);
    case Entity::kind::cube:
        return estimate_cube_pose(cloud, contour);
    case Entity::kind::long_cube:
        return estimate_long_cube_pose(cloud, contour);
    case Entity::kind::pin:
        return estimate_pin_pose(cloud, contour);
    }

    } catch (const std::exception &e) {
        spdlog::trace("[vision] Could not estimate pose: {}", e.what());
    }
    return std::nullopt;
}


}// namespace benchmark