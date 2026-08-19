#include "pose_estimation.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include <pcl/common/centroid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/common/transforms.h>
#include <pcl/filters/filter.h>
#include <pcl/registration/icp.h>

#include <pcl/visualization/pcl_visualizer.h>
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace constants {
static constexpr double deg_to_rad = 0.0174532925199432957692369076848861;
static constexpr double rad_to_deg = 57.2957795130823208767981548141052;
}// namespace constants


template<typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }


namespace benchmark {


std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> load_model(const std::string &path)
{
    auto cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>();
    pcl::io::loadPCDFile(path, *cloud);

    for (auto &p : *cloud) {
        p.r = p.g = p.b = 255;
    }
    return cloud;
}


void pose_refinement(Entity &entity, const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud)
{

    const auto current_cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>();

    switch (entity.kind_) {
    case Entity::kind::gear: {
        const static auto gear_initial_cloud =
            load_model("./data/objectDatabase/GearRedMediumBevels.pcd");
        pcl::transformPointCloud(*gear_initial_cloud, *current_cloud, entity.pose_, true);
        break;
    }
    case Entity::kind::conductor: {
        const static auto conductor_initial_cloud =
            load_model("./data/objectDatabase/circuits-ConductorModel.pcd");
        pcl::transformPointCloud(*conductor_initial_cloud, *current_cloud, entity.pose_, true);
        break;
    }
    case Entity::kind::gear_slot:
    case Entity::kind::circuit_connector:
    case Entity::kind::corner_marker:
    case Entity::kind::cube:
    case Entity::kind::pin:
    case Entity::kind::long_cube:
        throw std::logic_error("pose refinement called with incorrect entity");
    }

    auto cloud_filtered = std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>();
    std::vector<int> indices;
    pcl::removeNaNFromPointCloud(*cloud, *cloud_filtered, indices);


    auto test = std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>();

    pcl::IterativeClosestPoint<pcl::PointXYZRGB, pcl::PointXYZRGB> icp;
    icp.setInputSource(current_cloud);
    icp.setMaximumIterations(5);
    icp.setInputTarget(cloud_filtered);
    icp.align(*test);

    entity.pose_ = Eigen::Affine3d(icp.getFinalTransformation().cast<double>()) * entity.pose_;

    // consider only z Rotation
    entity.pose_.matrix().block<3, 3>(0, 0) =
        Eigen::Matrix3d(Eigen::AngleAxisd(entity.angle_z(), Eigen::Vector3d::UnitZ()));
}


int find_not_nan_index_with_radius(const pcl::PointCloud<pcl::PointXYZRGB> &vertices,
    int x_in,
    int y_in,
    int radius)
{
    const int width = vertices.width;
    const int height = vertices.height;

    for (int x = std::max(0, x_in - radius); x <= std::min(x_in + radius, width - 1); ++x)
        for (int y = std::max(0, y_in - radius); y <= std::min(y_in + radius , height - 1); ++y) {
            if (abs(x_in - x) + abs(y_in - y) != radius) continue;
            if (vertices[x + y * width].z > 0) return x + y * width;
        }
    return x_in + y_in * width;
}

int find(const pcl::PointCloud<pcl::PointXYZRGB> &vertices, int x_in, int y_in, int max_radius)
{
    for (int i = 0; i <= max_radius; ++i) {
        const int idx = find_not_nan_index_with_radius(vertices, x_in, y_in, i);
        if (idx > 0 && idx < vertices.size() && vertices[idx].z > 0) return idx;
    }

    // Return nan
    throw std::runtime_error("no not nan index found");
}


float max_z_point(const pcl::PointCloud<pcl::PointXYZRGB> &cloud, const Contour &contour)
{
    std::vector<cv::Point> valid_points;
    cv::findNonZero(contour.image, valid_points);


    valid_points.erase(
        std::remove_if(valid_points.begin(),
            valid_points.end(),
            [&cloud](const cv::Point &p) { return !pcl::isFinite(cloud.at(p.x, p.y)); }),
        valid_points.end());

    if (valid_points.empty()) {
        throw std::runtime_error("No valid points found in image");
    }

    const auto &highest_point = *std::max_element(
        valid_points.begin(), valid_points.end(), [&cloud](const cv::Point &l, const cv::Point &r) {
            return cloud.at(l.x, l.y).z < cloud.at(r.x, r.y).z;
        });


    return cloud.at(highest_point.x, highest_point.y).z;
}


cv::Point2f
    point_on_line(const cv::Point2f &point, const cv::Point2f &start, const cv::Point2f &end)
{
    const auto sp = point - start;
    const auto se = end - start;

    const auto t = sp.dot(se) / se.dot(se);

    return start + t * se;
}

std::pair<size_t, size_t> prev_and_next_index(std::vector<cv::Point> polygon, size_t index)
{
    const auto prev_p = index != 0 ? index - 1 : polygon.size() - 1;
    const auto next_p = index != polygon.size() - 1 ? index + 1 : 0;

    return { prev_p, next_p };
}


// returns clockwise angle in degrees
int angle_at(size_t index, const std::vector<cv::Point> &polygon)
{
    const cv::Point2d curr_p = polygon[index];
    const cv::Point2d prev_p = polygon[index != 0 ? index - 1 : polygon.size() - 1];
    const cv::Point2d next_p = polygon[index != polygon.size() - 1 ? index + 1 : 0];


    auto x = cv::Vec2d{ prev_p - curr_p };
    auto y = cv::Vec2d{ next_p - curr_p };

    const auto dot = x.dot(y);
    const auto determinant = x[0] * y[1] - y[0] * x[1];

    // result in radiants
    const auto rad = atan2(determinant, dot);

    return static_cast<int>(rad * constants::rad_to_deg + 360.0) % 360;
}

double singed_distance(cv::Point point, cv::Point line_begin, cv::Point line_end)
{
    const auto v1 = line_end - line_begin;
    const auto v2 = point - line_begin;

    const auto magnitude = v1.cross(v2);
    const auto distance = magnitude / cv::norm(v1);
    return distance;
}


std::array<cv::Point, 2> grow_line(const std::vector<cv::Point> &contour,
    size_t line_start_i,
    size_t line_end_i,
    const cv::Size &img_size = { 620, 350 })
{
    cv::Mat drawing = cv::Mat::zeros(img_size, CV_8UC3);

    std::vector<cv::Point2d> line_aprox;
    line_aprox.push_back(contour[line_start_i]);
    line_aprox.push_back(contour[line_end_i]);

    const size_t i = line_start_i;
    const size_t j = line_end_i;

    const auto &s = contour[line_start_i];
    const auto &e = contour[line_end_i];

    for (const auto &pnt : { i, j }) {
        auto [p1, n1] = prev_and_next_index(contour, pnt);
        const auto prev_p1 = p1 != 0 ? p1 - 1 : contour.size() - 1;
        const auto next_n1 = n1 != contour.size() - 1 ? n1 + 1 : 0;

        const auto distance_prev = std::abs(singed_distance(contour[p1], s, e))
                                   + std::abs(singed_distance(contour[prev_p1], s, e));
        const auto distance_next = std::abs(singed_distance(contour[n1], s, e))
                                   + std::abs(singed_distance(contour[next_n1], s, e));

        if (distance_prev < distance_next) {
            line_aprox.push_back(contour[p1]);
            line_aprox.push_back(contour[prev_p1]);
        } else {
            line_aprox.push_back(contour[n1]);
            line_aprox.push_back(contour[next_n1]);
        }
    }
    auto line = cv::Vec4d();
    cv::fitLine(line_aprox, line, cv::DIST_L2, 0, 0.01, 0.01);

    const double scaling_factor = std::max(img_size.height, img_size.width);
    // calculate start point
    cv::Point startPoint;
    startPoint.x = line[2] - scaling_factor * line[0];// x0
    startPoint.y = line[3] - scaling_factor * line[1];// y0
    // calculate end point
    cv::Point endPoint;
    endPoint.x = line[2] + scaling_factor * line[0];// x[1]
    endPoint.y = line[3] + scaling_factor * line[1];// y[1]

    // TODO check return type
    cv::clipLine(img_size, startPoint, endPoint);

    for (const auto &p : line_aprox) {
        cv::circle(drawing, p, 2, cv::Scalar{ 255, 255, 255 }, cv::FILLED);
    }

    cv::line(drawing, startPoint, endPoint, cv::Scalar(0, 0, 255));


    return { startPoint, endPoint };
}


bool on_bisector(const cv::Point &point, const cv::Vec4i &line, double eps)
{
    // check if distance to both endpoints of the line is similar

    const auto distance = cv::norm(point - cv::Point(line[0], line[1]))
                          - cv::norm(point - cv::Point(line[2], line[3]));
    return std::abs(distance) < eps;
}


pcl::PointXYZRGB nearest_Point(const pcl::PointCloud<pcl::PointXYZRGB> &cloud,
    const cv::Mat &validPoints,
    const cv::Point &point)
{
    cv::Mat gray;

    std::vector<cv::Point> indices;
    cv::findNonZero(validPoints, indices);


    cv::Point neighbor = point;
    double distance = std::numeric_limits<double>::infinity();

    for (const auto &index : indices) {
        const auto current_distance = cv::norm(index - point);
        if (current_distance < distance && pcl::isFinite(cloud.at(index.x, index.y))) {
            distance = current_distance;
            neighbor = index;
        }
    }

    return cloud.at(neighbor.x, neighbor.y);
}

struct line2d
{
    size_t start, end;
    double length;
    line2d(size_t s, size_t e, double l) : start(s), end(e), length(l) {}
    static bool shorter(const line2d &lhs, const line2d &rhs) { return lhs.length < rhs.length; }
};


// throws if polygon does not have 3 points, or if the two longest lines don't have a point
// in common
std::optional<std::tuple<size_t, size_t, size_t>> two_longest_lines(
    const std::vector<cv::Point> &polygon)
{

    if (polygon.size() < 3) {
        return std::nullopt;
    }

    std::vector<line2d> lines;
    for (size_t i = 0; i < polygon.size() - 1; ++i) {
        lines.emplace_back(i, i + 1, cv::norm(polygon[i + 1] - polygon[i]));
    }
    lines.emplace_back(polygon.size() - 1, 0, cv::norm(polygon.front() - polygon.back()));

    std::sort(lines.begin(), lines.end(), line2d::shorter);

    const auto &l1 = lines[lines.size() - 2];
    const auto &l2 = lines[lines.size() - 1];


    if (l1.end == l2.start) return std::tuple{ l1.start, l2.start, l2.end };
    if (l2.end == l1.start) return std::tuple{ l2.start, l1.start, l1.end };
    return std::nullopt;
}

std::optional<Entity> estimate_corner_marker_pose(
    const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const Contour &contour)
{
    using namespace std::literals;

    std::vector<cv::Point> poly;

    for (int eps = 0; eps <= 5; eps++) {
        cv::approxPolyDP(contour.points, poly, eps, true);
        if (poly.size() == 6) {
            spdlog::trace("Approximated corner contour with epsilon of "s + std::to_string(eps));
            break;
        }
    }
    if (poly.size() != 6)
        spdlog::warn("Attempting corner pose estimation with "s + std::to_string(poly.size())
                     + " points, instead of 6"s);

    auto res = two_longest_lines(poly);
    if (res.has_value() == false) {
        return std::nullopt;
    }


    auto [start, middle, end] = *res;


    try {
        const auto &start3d = cloud->at(find(*cloud, poly[start].x, poly[start].y, 10));
        const auto &middle3d = cloud->at(find(*cloud, poly[middle].x, poly[middle].y, 10));
        const auto &end3d = cloud->at(find(*cloud, poly[end].x, poly[end].y, 10));


        const auto center = (point_on_line(
            { middle3d.x, middle3d.y }, { start3d.x, start3d.y }, { end3d.x, end3d.y }));

        const auto max_z = max_z_point(*cloud, contour);

        Eigen::Vector3d position{ center.x, center.y, max_z };

        const Eigen::Vector2d direction =
            Eigen::Vector2d(center.x, center.y) - Eigen::Vector2d(middle3d.x, middle3d.y);

        Eigen::Vector3d offset;
        offset << direction, 0;
        offset.normalize();
        offset *= 0.014;

        position += offset;

        const auto angle_z = atan2f(direction.y(), direction.x()) - atan2f(0.0, 1.0);

        return Entity{ Entity::kind::corner_marker, contour.color, position, angle_z };
    } catch (const std::exception &e) {
        // find() threw because it found only nans
        std::cerr << e.what();
    }
    return std::nullopt;
}


std::optional<Entity> estimate_pin_pose(
    const std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> &cloud,
    const Contour &contour)
{
    auto rect = minAreaRect(contour.points);

    cv::Point2f rect_points[4];
    rect.points(rect_points);

    // edges of rotated rectangle might be outside the image, push them inside
    for (auto &p : rect_points) {
        p.x = std::clamp(p.x, 0.f, static_cast<float>(cloud->width-1));
        p.y = std::clamp(p.y, 0.f, static_cast<float>(cloud->height-1));
    }

    std::vector<pcl::PointXYZRGB> points;
    for (const auto p : rect_points) {
        points.push_back(cloud->at(find(*cloud, p.x, p.y, 10)));
    }

    // averange center position from all 4 corners
    auto center3d = pcl::PointXYZRGB(0.0f, 0.0f, 0.0f);
    for (const auto &p : points) {
        center3d.x += p.x;
        center3d.y += p.y;
        center3d.z += p.z;
    }
    center3d.x /= 4.0f;
    center3d.y /= 4.0f;
    center3d.z /= 4.0f;


    const auto &corner = cloud->at(find(*cloud, rect_points[0].x, rect_points[0].y, 10));


    const auto angle_z = atan2f(corner.y - center3d.y, corner.x - center3d.x);


    std::vector<cv::Point> valid_points;
    cv::findNonZero(contour.image, valid_points);

    valid_points.erase(
        std::remove_if(valid_points.begin(),
            valid_points.end(),
            [&cloud](const cv::Point &p) { return !pcl::isFinite(cloud->at(p.x, p.y)); }),
        valid_points.end());

    if (valid_points.empty()) {
        throw std::runtime_error("No valid points found in image");
    }

    const auto lowest_point = *std::min_element(
        valid_points.begin(), valid_points.end(), [&cloud](const cv::Point &l, const cv::Point &r) {
            return cloud->at(l.x, l.y).z < cloud->at(r.x, r.y).z;
        });


    const auto height = cloud->at(lowest_point.x, lowest_point.y).z;
    /*{
        pcl::visualization::PCLVisualizer visu;
        using namespace std::literals;
        for (const auto &p : points) {
            const auto name = "sphere_"s + std::to_string(std::rand());
            visu.addSphere<pcl::PointXYZRGB>(p, 0.001, name);
        }
        visu.addPointCloud(cloud);
        visu.spin();
    }*/


    return Entity{
        Entity::kind::pin, contour.color, { center3d.x, center3d.y, height + 0.019 }, angle_z
    };
}

std::optional<std::pair<Eigen::Vector3d, double>> get_pin_pose(
    const std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> &cloud,
    const Contour &contour)
{
    const auto pin = estimate_pin_pose(cloud, contour);
    if (!pin) return std::nullopt;

    const auto t = pin->pose_.translation();
    return { { { t.x(), t.y(), t.z() }, pin->angle_z() } };
}

std::optional<Entity> estimate_cube_pose(
    const std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> &cloud,
    const Contour &contour)
{

    const Contour *biggest_child = nullptr;
    auto max_size = cv::Size2f{ 0, 0 };
    for (const auto &child : contour.children) {
        const auto area = minAreaRect(child.points).size.area();
        if (area > max_size.area()) {
            biggest_child = &child;
        }
    }
    if (!biggest_child) return std::nullopt;

    const auto pin = get_pin_pose(cloud, *biggest_child);
    if (!pin) return std::nullopt;

    auto [loc, rot] = *pin;
    const auto height = max_z_point(*cloud, contour);
    loc.z() = height + 0.019;
    return Entity{ Entity::kind::cube, contour.color, loc, rot };
}

std::optional<Entity> estimate_long_cube_pose(
    const std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>> &cloud,
    const Contour &contour)
{
    std::optional<Entity> res = std::nullopt;


    std::vector<std::pair<const Contour *, double>> childs;
    for (const auto &child : contour.children) {
        childs.push_back({ &child, minAreaRect(child.points).size.area() });
    }

    std::sort(childs.begin(),
        childs.end(),
        [](const std::pair<const Contour *, double> &a,
            const std::pair<const Contour *, double> &b) { return a.second > b.second; });

    if (childs.size() >= 2) {
        const auto height = max_z_point(*cloud, contour);

        const auto pin1 = get_pin_pose(cloud, *childs[0].first);
        if (!pin1) return std::nullopt;
        auto [loc1, rot1] = *pin1;
        loc1.z() = height + 0.019;

        const auto pin2 = get_pin_pose(cloud, *childs[1].first);
        if (!pin2) return std::nullopt;
        auto [loc2, rot2] = *pin2;
        loc2.z() = height + 0.019;


        const auto dir = (loc2 - loc1) / 2;
        const auto dir_n = dir.normalized();
        const auto angle_z = atan2f(dir_n.y(), dir_n.x());
        res = Entity{ Entity::kind::long_cube, contour.color, loc1 + dir, angle_z };
    }

    if (childs.size() == 1) {
        const auto pin1 = get_pin_pose(cloud, *childs[0].first);
        if (!pin1) return std::nullopt;
        auto [loc1, rot1] = *pin1;
        const auto height = max_z_point(*cloud, contour);
        loc1.z() = height + 0.019;


        auto rect = cv::minAreaRect(childs[0].first->points);
        if (rect.size == cv::Size2f(0, 0)) {
            std::cout << "empty contour\n";
        }

        const auto &center3d = cloud->at(find(*cloud, rect.center.x, rect.center.y, 10));
        const Eigen::Vector3d contour_center = { center3d.x, center3d.y, center3d.z };


        const auto dir = (contour_center - loc1) / 2;
        const auto dir_n = dir.normalized();
        const auto dir_plane_n = Eigen::Vector3d(dir_n.x(), dir_n.y(), 0).normalized();
        const auto angle_z = atan2f(dir_n.y(), dir_n.x());
        res =
            Entity{ Entity::kind::long_cube, contour.color, loc1 + (dir_plane_n * 0.041), angle_z };
    }
    if (res) {
        auto rect = cv::minAreaRect(contour.points);
        const auto &contour_center = cloud->at(find(*cloud, rect.center.x, rect.center.y, 10));

        res->pose_.translation().z() = contour_center.z + +0.019;
    }


    return res;
}


Contour::Contour(const std::vector<std::vector<cv::Point>> &contours,
    const std::vector<cv::Vec4i> &hierarchy,
    int idx,
    Entity::color c,
    const pcl::PointCloud<pcl::PointXYZRGB> &flat_cloud,
    const cv::Size &image_size)
    : image([&]() {
          cv::Mat img = cv::Mat::zeros(image_size, CV_8UC1);
          cv::drawContours(img, contours, idx, 255, cv::FILLED, cv::LINE_8, hierarchy, 100);
          return img;
      }()), contours_(contours), color(c), points(contours[idx]),
      oobb(OOBB::calculate(flat_cloud, image))
{
    // if we have children
    if (hierarchy[idx][2] >= 0) {
        // then, for each child
        for (int child_idx = hierarchy[idx][2]; child_idx >= 0;
             child_idx = hierarchy[child_idx][0]) {
            children.push_back({contours, hierarchy, child_idx, color, flat_cloud, image_size});
        }
    }    
}


std::optional<Entity> estimate_gear_pose(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const Contour &c)
{
    std::vector<cv::Point> poly;
    cv::approxPolyDP(c.points, poly, 2, true);

    std::vector<cv::Point> concave_points;
    std::vector<cv::Point> non_concave_points;
    for (size_t i = 0; i < poly.size(); ++i) {
        auto angle = angle_at(i, poly);
        if (angle > 225) {
            concave_points.push_back(poly[i]);
        } else {
            non_concave_points.push_back(poly[i]);
        }
    }

    if (concave_points.size() != 2) {
        std::cout << "We identified " << concave_points.size() << " point(s) as bisectors\n";
        return std::nullopt;
    }

    cv::Mat labels;
    std::vector<cv::Point2f> non_concave_f;
    for (const auto &p : non_concave_points) {
        non_concave_f.emplace_back(p);// convert int to double
    }

    if (non_concave_f.size() < 3) {
        return std::nullopt;
    }

    cv::kmeans(non_concave_f,
        3,
        labels,
        cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
        5,
        cv::KMEANS_PP_CENTERS);

    auto c0_in_hull = std::find(c.points.begin(), c.points.end(), concave_points[0]);
    auto c1_in_hull = std::find(c.points.begin(), c.points.end(), concave_points[1]);

    if (c0_in_hull > c1_in_hull) {
        std::swap(c0_in_hull, c1_in_hull);
    }

    std::vector<std::vector<cv::Point>> segs(2);

    // range within points
    auto &seg1 = segs[0];
    std::copy(c0_in_hull, c1_in_hull + 1, std::back_inserter(seg1));

    auto &seg2 = segs[1];
    // range outside of points
    std::copy(c.points.cbegin(), c0_in_hull + 1, std::back_inserter(seg2));
    std::copy(c1_in_hull, c.points.cend(), std::back_inserter(seg2));
    bool found_long, found_small, swap = false;
    found_long = found_small = false;

    for (size_t i = 0; i < segs.size(); i++) {
        std::array<int, 3> seen_labels;
        seen_labels.fill(0);
        for (const auto &p : segs[i]) {
            auto it = std::find(non_concave_points.begin(), non_concave_points.end(), p);
            if (it != non_concave_points.end()) {
                size_t index = it - non_concave_points.begin();
                auto label = labels.at<int>(index);
                seen_labels[label] = true;
            }
        }
        auto count = std::accumulate(seen_labels.begin(), seen_labels.end(), 0);

        if (count == 2) {
            found_long = true;
            if (i == 0) swap = true;
        } else if (count == 1) {
            found_small = true;
        } else {
            std::cout << "found " << count << " labeled endpoints\n";
            return std::nullopt;
        }
    }
    if ((found_long & found_small) == false) {
        std::cout << "did not find both T segments\n";
        return std::nullopt;
    }
    if (swap) std::swap(segs[0], segs[1]);
    auto long_rect = cv::minAreaRect(segs[1]);

    cv::Point2f rect_points[4];
    long_rect.points(rect_points);


    cv::Vec2f one_side = rect_points[2] - rect_points[1];
    cv::Vec2f other_side = rect_points[0] - rect_points[1];

    const cv::Point2f longer_vec =
        cv::norm(one_side) > cv::norm(other_side) ? one_side : other_side;

    const auto bisect_line_s = long_rect.center - (longer_vec * 0.5);
    const auto bisect_line_e = long_rect.center + (longer_vec * 0.5);

    const auto midpoint = (concave_points[0] + concave_points[1]) / 2;

    const auto center = point_on_line(midpoint, bisect_line_s, bisect_line_e);

    try {

        const auto &center3d = cloud->at(find(*cloud, center.x, center.y, 10));


        const auto center_point = Eigen::Vector3d{ center3d.x, center3d.y, center3d.z };

        const auto &endpoint2d =
            singed_distance(bisect_line_s, center, midpoint) > 0 ? bisect_line_s : bisect_line_e;
        const auto &endpoint3d = cloud->at(find(*cloud, endpoint2d.x, endpoint2d.y, 10));

        const auto endpoint = Eigen::Vector3d{ endpoint3d.x, endpoint3d.y, endpoint3d.z };

        Eigen::Vector3d vec_small_line = endpoint - center_point;
        vec_small_line.normalize();

        const auto angle_z =
            atan2f(vec_small_line.y(), vec_small_line.x()) - atan2f(0.0, 1.0);


        auto gear =
            Entity{ Entity::kind::gear, c.color, { center3d.x, center3d.y, center3d.z }, angle_z };
        pose_refinement(gear, cloud);
        return gear;
    }

    catch (const std::exception &e) {
        std::cerr << e.what();
    }
    return std::nullopt;
}


Entity estimate_hole_pose(const pcl::PointCloud<pcl::PointXYZRGB> scene, const Contour &contour)
{
    std::vector<cv::Point> pixels;
    cv::findNonZero(contour.image, pixels);


    auto centroid = pcl::CentroidPoint<pcl::PointXYZRGB>();

    float max_z = std::numeric_limits<double>::min();
    for (const auto &pixel : pixels) {
        const auto point = scene.at(pixel.x, pixel.y);
        if (pcl::isFinite(point)) {
            centroid.add(point);
            max_z = std::max(max_z, point.z);
        }
    }
    auto center_3d = pcl::PointXYZRGB();
    centroid.get(center_3d);

    return {
        Entity::kind::circuit_connector, contour.color, { center_3d.x, center_3d.y, max_z }, 0
    };
}


std::optional<Entity::kind> baseplate_marker(Entity::color color)
{
    switch (color) {
    case Entity::gear_slot_color:
        return Entity::kind::gear_slot;
    case Entity::circuit_connector_color:
        return Entity::kind::circuit_connector;
    case Entity::pin_color:
        return Entity::kind::pin;

    default:
        return std::nullopt;
    }
}


std::optional<Entity::kind> conductor_or_gear(const std::vector<cv::Point> &contour)
{
    std::vector<cv::Point> poly;
    cv::approxPolyDP(contour, poly, 2, true);

    std::vector<cv::Point> concave_points;
    std::vector<cv::Point> non_concave_points;
    for (size_t i = 0; i < poly.size(); ++i) {
        const auto angle = angle_at(i, poly);
        if (angle > 225) {
            concave_points.push_back(poly[i]);
        } else {
            non_concave_points.push_back(poly[i]);
        }
    }

    switch (concave_points.size()) {
    case 0:
        return Entity::kind::conductor;
    case 2:
        return Entity::kind::gear;
    default:
        return std::nullopt;
    }
}


std::optional<Entity> estimate_conductor_pose(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
    const Contour &contour)
{
    const auto long_rect = cv::minAreaRect(contour.points);

    cv::Point2f rect_points[4];
    long_rect.points(rect_points);


    {
        const cv::Vec2f one_side = rect_points[2] - rect_points[1];
        const cv::Vec2f other_side = rect_points[0] - rect_points[1];

        const cv::Point2f longer_vec =
            cv::norm(one_side) > cv::norm(other_side) ? one_side : other_side;

        try {

            const auto &center3d =
                cloud->at(find(*cloud, long_rect.center.x, long_rect.center.y, 10));


            const auto center_point = Eigen::Vector3d{ center3d.x, center3d.y, center3d.z };


            const auto &endpoint2d = long_rect.center + (longer_vec * 0.5);
            const auto &endpoint3d = cloud->at(find(*cloud, endpoint2d.x, endpoint2d.y, 10));

            const auto endpoint = Eigen::Vector3d{ endpoint3d.x, endpoint3d.y, endpoint3d.z };

            Eigen::Vector3d vec_big_line = endpoint - center_point;
            vec_big_line.normalize();

            const auto angle_z = atan2f(vec_big_line.y(), vec_big_line.x());


            auto conductor = Entity{ Entity::kind::conductor,
                contour.color,
                { center3d.x, center3d.y, max_z_point(*cloud, contour) },
                angle_z };
            pose_refinement(conductor, cloud);
            return conductor;

        } catch (const std::exception &e) {
            // find() threw because it found only nans
            std::cerr << e.what();
        }
    }
    return std::nullopt;
}

}// namespace benchmark