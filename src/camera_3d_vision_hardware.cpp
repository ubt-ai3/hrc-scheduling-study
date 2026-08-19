#include "camera_3d_vision_hardware.h"

#include <fstream>
#include <iterator>
#include <utility>

#include <librealsense2/rs_advanced_mode.hpp>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include "camera_util.h"

namespace hardware {
realsense_pcl_grabber::realsense_pcl_grabber(const Eigen::Vector2i &color_resolution,
    const Eigen::Vector2i &depth_resolution,
    const int framerate,
    std::string config_file_path,
    std::string serial_number)
    : color_resolution_(color_resolution), depth_resolution_(depth_resolution),
      clipped_resolution_(clipped_resolution_from_depth_resoltuion()), framerate_(framerate),
      config_file_path_(std::move(config_file_path)), serial_number_(std::move(serial_number))
{
    init_pipe();
}

realsense_pcl_grabber::~realsense_pcl_grabber() { pipe_.stop(); }

void realsense_pcl_grabber::reset_pipe(const Eigen::Vector2i &color_resolution,
    const Eigen::Vector2i &depth_resolution,
    const int framerate,
    std::string config_file_path,
    std::string serial_number)
{
    pipe_.stop();

    color_resolution_ = color_resolution;
    depth_resolution_ = depth_resolution;
    clipped_resolution_ = clipped_resolution_from_depth_resoltuion();
    framerate_ = framerate;
    config_file_path_ = std::move(config_file_path);
    serial_number_ = std::move(serial_number);

    init_pipe();
}

rs2::frameset realsense_pcl_grabber::fetch_raw_data() const
{
    std::lock_guard<std::mutex> lock(capturing_mutex_);
    return pipe_.wait_for_frames();
}

point_cloud *realsense_pcl_grabber::cloud_from_frameset(rs2::frameset &frames,
    const float max_z) const
{
    const rs2::depth_frame depth = frames.get_depth_frame();

    rs2::pointcloud rs_pointcloud;
    return points_to_depth_cloud(rs_pointcloud.calculate(depth), max_z);
}

point_cloud_colored *realsense_pcl_grabber::colored_cloud_from_frameset(rs2::frameset &frames,
    const float max_z) const
{
    const rs2::depth_frame depth = frames.get_depth_frame();
    const rs2::video_frame color = frames.get_color_frame();

    rs2::pointcloud rs_pointcloud;
    rs_pointcloud.map_to(color);
    return points_to_colored_cloud(rs_pointcloud.calculate(depth), color, max_z);
}

point_cloud_colored *realsense_pcl_grabber::clipped_colored_cloud_from_frameset(
    rs2::frameset &frames,
    float max_z) const
{
    const rs2::depth_frame depth = frames.get_depth_frame();
    const rs2::video_frame color = frames.get_color_frame();

    rs2::pointcloud rs_pointcloud;
    rs_pointcloud.map_to(color);
    return points_to_clipped_colored_cloud(rs_pointcloud.calculate(depth), color, max_z);
}

point_cloud *realsense_pcl_grabber::fetch_cloud(const float max_z) const
{
    rs2::frameset frameset(fetch_raw_data());
    return cloud_from_frameset(frameset, max_z);
}

point_cloud_colored *realsense_pcl_grabber::fetch_colored_cloud(const float max_z) const
{
    rs2::frameset frameset(fetch_raw_data());
    return colored_cloud_from_frameset(frameset, max_z);
}

point_cloud_colored *realsense_pcl_grabber::fetch_clipped_colored_cloud(const float max_z) const
{
    rs2::frameset frameset(fetch_raw_data());
    return clipped_colored_cloud_from_frameset(frameset, max_z);
}

Eigen::Vector2i realsense_pcl_grabber::color_resolution() const { return color_resolution_; }

Eigen::Vector2i realsense_pcl_grabber::depth_resolution() const { return depth_resolution_; }

Eigen::Vector2i realsense_pcl_grabber::clipped_resolution() const { return clipped_resolution_; }

int realsense_pcl_grabber::framerate() const { return framerate_; }

std::string realsense_pcl_grabber::config_file_path() const { return config_file_path_; }

std::string realsense_pcl_grabber::serial_number() const { return serial_number_; }

void realsense_pcl_grabber::init_pipe()
{
    rs2::config config;

    config.enable_stream(RS2_STREAM_COLOR,
        color_resolution_.x(),
        color_resolution_.y(),
        RS2_FORMAT_RGB8,
        framerate_);
    config.enable_stream(
        RS2_STREAM_DEPTH, depth_resolution_.x(), depth_resolution_.y(), RS2_FORMAT_ANY, framerate_);
    // config.enable_record_to_file("C:/Users/huemmer/OneDrive - Universit�t
    // Bayreuth/BA/Projektverzeichnis/05.Rohdaten/Realsense.Viewer.Aufnahmen/cubes.pin.1.bag");
    //config.enable_device_from_file("C:/Users/huemmer/OneDrive - Universit�t Bayreuth/uni/MSc_Projekt_Jonathan_Huemmer/05.Rohdaten/realsense recordings/demo_scene.bag");

    if (serial_number_.empty()) {
        auto profile = pipe_.start(config);

        if (!config_file_path().empty()) {
            rs400::advanced_mode dev = profile.get_device();
            std::ifstream input(config_file_path_);
            dev.load_json(std::string(
                std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()));
        }
        serial_number_ =
            pipe_.get_active_profile().get_device().get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
    } else {
        config.enable_device(serial_number_);

        auto profile = pipe_.start(config);

        if (!config_file_path().empty()) {
            rs400::advanced_mode dev = profile.get_device();
            std::ifstream input(config_file_path_);
            dev.load_json(std::string(
                std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()));
        }
    }
}

point_cloud *realsense_pcl_grabber::points_to_depth_cloud(const rs2::points &points,
    const float max_z)
{
    const auto stream_profile = points.get_profile().as<rs2::video_stream_profile>();
    const auto *const vertices = points.get_vertices();

    auto *cloud(new point_cloud);
    cloud->width = stream_profile.width();
    cloud->height = stream_profile.height();
    cloud->is_dense = false;
    cloud->points.resize(points.size());

    for (size_t i = 0; i < cloud->points.size(); ++i) {
        auto &p = cloud->points[i];
        if (vertices[i].z <= 0 || vertices[i].z > max_z) {
            p.x = p.y = p.z = std::numeric_limits<float>::quiet_NaN();
        } else {
            p.x = vertices[i].x;
            p.y = vertices[i].y;
            p.z = vertices[i].z;
        }
    }

    return cloud;
}

point_cloud_colored *realsense_pcl_grabber::points_to_colored_cloud(const rs2::points &points,
    const rs2::video_frame &color,
    const float max_z) const
{
    const auto stream_profile = points.get_profile().as<rs2::video_stream_profile>();
    const auto vertices = points.get_vertices();
    const auto tex_coords = points.get_texture_coordinates();

    auto *cloud(new point_cloud_colored);
    cloud->width = stream_profile.width();
    cloud->height = stream_profile.height();
    cloud->is_dense = false;
    cloud->points.resize(points.size());

    const auto *rgb_data = static_cast<const unsigned char *>(color.get_data());

    for (size_t i = 0; i < cloud->points.size(); ++i) {
        auto &p = cloud->points[i];

        // get the color position
        int valx = lround(color.get_width() * tex_coords[i].u);
        int valy = lround(color.get_height() * tex_coords[i].v);

        if (vertices[i].z <= 0 || vertices[i].z > max_z || valx < 0 || valx >= color.get_width()
            || valy < 0 || valy >= color.get_height()) {
            // point is invalid
            p.r = p.g = p.b = 0;
            p.x = p.y = p.z = std::numeric_limits<float>::quiet_NaN();
        } else {
            p.x = vertices[i].x;
            p.y = vertices[i].y;
            p.z = vertices[i].z;

            valx = std::min(std::max(valx, 0), color.get_width() - 1);
            valy = std::min(std::max(valy, 0), color.get_height() - 1);
            const int idx = valx * color.get_bytes_per_pixel() + valy * color.get_stride_in_bytes();

            p.r = rgb_data[idx];
            p.g = rgb_data[idx + 1];
            p.b = rgb_data[idx + 2];
        }
    }

    return cloud;
}

point_cloud_colored *realsense_pcl_grabber::points_to_clipped_colored_cloud(
    const rs2::points &points,
    const rs2::video_frame &color,
    float max_z) const
{
    Eigen::Vector2i offset(160, 100);// depth_resolution_ == Eigen::Vector2i(1280, 720)
    if (depth_resolution_ == Eigen::Vector2i(848, 480)) offset = { 100, 70 };

    const auto stream_profile = points.get_profile().as<rs2::video_stream_profile>();
    const auto vertices = points.get_vertices();
    const auto tex_coords = points.get_texture_coordinates();

    auto *cloud(new point_cloud_colored);
    cloud->width = clipped_resolution_.x();
    cloud->height = clipped_resolution_.y();
    cloud->is_dense = false;
    cloud->points.resize(cloud->width * cloud->height);

    const auto *rgb_data = static_cast<const unsigned char *>(color.get_data());

    const auto find_not_nan_index_with_radius =
        [](const rs2::vertex *vertices, int x_in, int y_in, int radius, int width, int height)
        -> int {
        for (int x = std::max(0, x_in - radius); x <= std::min(x_in + radius, width - 1); ++x)
            for (int y = std::max(0, y_in - radius); y <= std::min(y_in + radius, height - 1);
                 ++y) {
                if (abs(x_in - x) + abs(y_in - y) != radius) continue;
                if (vertices[x + y * width].z > 0) return x + y * width;
            }

        // Return nan
        return x_in + y_in * width;
    };

    const auto find_closest_not_nan_index =
        [&find_not_nan_index_with_radius](
            const rs2::vertex *vertices, int x_in, int y_in, int max_radius, int width, int height)
        -> int {
        for (int i = 0; i <= max_radius; ++i) {
            int idx = find_not_nan_index_with_radius(vertices, x_in, y_in, i, width, height);
            if (vertices[idx].z > 0) return idx;
        }

        // Return nan
        return x_in + y_in * width;
    };

    for (size_t y = 0; y < cloud->height; ++y)
        for (size_t x = 0; x < cloud->width; ++x) {
            const size_t i = x + cloud->width * y;
            const size_t j = (x + offset.x()) + stream_profile.width() * (y + offset.y());

            auto &p = cloud->points[i];

            // get the color position
            int valx = lround(color.get_width() * tex_coords[j].u);
            int valy = lround(color.get_height() * tex_coords[j].v);

            if (valx < 0 || valx >= color.get_width() || valy < 0 || valy >= color.get_height()) {
                // point is invalid
                p.r = p.g = p.b = 0;
                p.x = p.y = p.z = std::numeric_limits<float>::quiet_NaN();
            } else if (vertices[j].z <= 0 || vertices[j].z > max_z) {
                // point is invalid, but has color

                const auto closest_point = find_closest_not_nan_index(vertices,
                    x + offset.x(),
                    y + offset.y(),
                    5,
                    stream_profile.width(),
                    stream_profile.height());


                if (closest_point == x + offset.x() + (y + offset.y()) * stream_profile.width()) {
                    p.x = p.y = p.z = std::numeric_limits<float>::quiet_NaN();
                    p.r = p.g = p.b = 0;
                    continue;
                }

                valx = lround(color.get_width() * tex_coords[closest_point].u);
                valy = lround(color.get_height() * tex_coords[closest_point].v);
                valx = std::min(std::max(valx, 0), color.get_width() - 1);
                valy = std::min(std::max(valy, 0), color.get_height() - 1);

                const int idx =
                    valx * color.get_bytes_per_pixel() + valy * color.get_stride_in_bytes();

                p.x = p.y = p.z = std::numeric_limits<float>::quiet_NaN();

                p.r = rgb_data[idx];
                p.g = rgb_data[idx + 1];
                p.b = rgb_data[idx + 2];

            } else {
                p.x = vertices[j].x;
                p.y = vertices[j].y;
                p.z = vertices[j].z;

                valx = std::min(std::max(valx, 0), color.get_width() - 1);
                valy = std::min(std::max(valy, 0), color.get_height() - 1);
                const int idx =
                    valx * color.get_bytes_per_pixel() + valy * color.get_stride_in_bytes();

                p.r = rgb_data[idx];
                p.g = rgb_data[idx + 1];
                p.b = rgb_data[idx + 2];
            }
        }

    return cloud;
}

Eigen::Vector2i realsense_pcl_grabber::clipped_resolution_from_depth_resoltuion() const
{
    if (depth_resolution_ == Eigen::Vector2i(1280, 720)) return { 920, 520 };
    if (depth_resolution_ == Eigen::Vector2i(848, 480)) return { 620, 350 };

    return depth_resolution_;
}


const Eigen::Matrix4d camera_matrix_realsense_720p_16to9((Eigen::Matrix4d() << 637.2820,
    0.0,
    652.6480,
    0.0,
    0.0,
    637.2820,
    360.6860,
    0.0,
    0.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0)
                                                             .finished());

const Eigen::Matrix4d camera_matrix_realsense_720p_16to9_clipped((Eigen::Matrix4d() << 637.2820,
    0.0,
    492.6480,
    0.0,
    0.0,
    637.2820,
    260.6860,
    0.0,
    0.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0)
                                                                     .finished());

const Eigen::Matrix4d camera_matrix_realsense_480p_16to9((Eigen::Matrix4d() << 422.2,
    0.0,
    432.38,
    0.0,
    0.0,
    422.2,
    240.45,
    0.0,
    0.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0)
                                                             .finished());

const Eigen::Matrix4d camera_matrix_realsense_480p_16to9_clipped((Eigen::Matrix4d() << 422.2,
    0.0,
    332.38,
    0.0,
    0.0,
    422.2,
    170.45,
    0.0,
    0.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    1.0,
    0.0)
                                                                     .finished());


camera_3d_vision_hardware::camera_3d_vision_hardware(const std::string &config_path, double max_z)
    : m_grabber(new realsense_pcl_grabber({ 1920, 1080 }, { 848, 480 }, 30, config_path)),
      m_camera_matrix(camera_matrix_realsense_480p_16to9_clipped), m_min_z(0.2), m_max_z(max_z),
      m_view_frustum(generate_frustum_planes(), Eigen::Affine3d::Identity())
{
    // throw away first few frames
    for (int i = 0; i < 30; ++i) auto frameset = m_grabber->fetch_raw_data();
}

camera_3d_vision_hardware::~camera_3d_vision_hardware() { delete m_grabber; }

point_cloud_colored::Ptr camera_3d_vision_hardware::capture_image()
{
    return point_cloud_colored::Ptr(
        m_grabber->fetch_clipped_colored_cloud(static_cast<float>(m_max_z)));
}

Eigen::Vector2i camera_3d_vision_hardware::resolution() const
{
    return m_grabber->clipped_resolution();
}

view_frustum camera_3d_vision_hardware::frustum() const { return m_view_frustum; }

std::vector<Eigen::Vector4d> camera_3d_vision_hardware::generate_frustum_planes() const
// todo
{
    Eigen::Vector2d resolution = m_grabber->clipped_resolution().cast<double>();

    const double min_x_near =
        reproject_point(m_camera_matrix.coeff(0, 0), m_camera_matrix.coeff(0, 2), 0.0, m_min_z);
    const double max_x_near = reproject_point(
        m_camera_matrix.coeff(0, 0), m_camera_matrix.coeff(0, 2), resolution.x(), m_min_z);
    const double min_y_near =
        reproject_point(m_camera_matrix.coeff(1, 1), m_camera_matrix.coeff(1, 2), 0.0, m_min_z);
    const double max_y_near = reproject_point(
        m_camera_matrix.coeff(1, 1), m_camera_matrix.coeff(1, 2), resolution.y(), m_min_z);
    const double min_x_far =
        reproject_point(m_camera_matrix.coeff(0, 0), m_camera_matrix.coeff(0, 2), 0.0, m_max_z);
    const double max_x_far = reproject_point(
        m_camera_matrix.coeff(0, 0), m_camera_matrix.coeff(0, 2), resolution.x(), m_max_z);
    const double min_y_far =
        reproject_point(m_camera_matrix.coeff(1, 1), m_camera_matrix.coeff(1, 2), 0.0, m_max_z);
    const double max_y_far = reproject_point(
        m_camera_matrix.coeff(1, 1), m_camera_matrix.coeff(1, 2), resolution.y(), m_max_z);

    const Eigen::Vector3d near_0_0(min_x_near, min_y_near, m_min_z);
    const Eigen::Vector3d near_920_0(max_x_near, min_y_near, m_min_z);
    const Eigen::Vector3d near_0_920(min_x_near, max_y_near, m_min_z);
    const Eigen::Vector3d near_920_920(max_x_near, max_y_near, m_min_z);
    const Eigen::Vector3d far_0_0(min_x_far, min_y_far, m_max_z);
    const Eigen::Vector3d far_920_0(max_x_far, min_y_far, m_max_z);
    const Eigen::Vector3d far_0_920(min_x_far, max_y_far, m_max_z);
    const Eigen::Vector3d far_920_920(max_x_far, max_y_far, m_max_z);

    Eigen::Vector4d near;
    near.head<3>() = (near_920_0 - near_0_0).cross(near_0_920 - near_0_0).normalized();
    near.w() = near_0_0.dot(near.head<3>());

    Eigen::Vector4d far;
    far.head<3>() = (far_0_920 - far_0_0).cross(far_920_0 - far_0_0).normalized();
    far.w() = far_0_0.dot(far.head<3>());

    Eigen::Vector4d up;
    up.head<3>() = (far_0_0 - near_0_0).cross(near_920_0 - near_0_0).normalized();
    up.w() = near_0_0.dot(up.head<3>());

    Eigen::Vector4d down;
    down.head<3>() = (near_920_920 - near_0_920).cross(far_0_920 - near_0_920).normalized();
    down.w() = near_0_920.dot(down.head<3>());

    Eigen::Vector4d left;
    left.head<3>() = (near_0_920 - near_0_0).cross(far_0_0 - near_0_0).normalized();
    left.w() = near_0_0.dot(left.head<3>());

    Eigen::Vector4d right;
    right.head<3>() = (far_920_0 - near_920_0).cross(near_920_920 - near_920_0).normalized();
    right.w() = near_920_0.dot(right.head<3>());

    // Eigen::Vector3d test(0.0, 0.0, 0.5);
    // std::cout << "near: " << near.transpose() << "  dist: " << near.head<3>().dot(test) -
    // near.w() << std::endl; std::cout << "far: " << far.transpose() << "  dist: " <<
    // far.head<3>().dot(test) - far.w() << std::endl; std::cout << "up: " << up.transpose() << "
    // dist: " << up.head<3>().dot(test) - up.w() << std::endl; std::cout << "down: " <<
    // down.transpose() << "  dist: " << down.head<3>().dot(test) - down.w() << std::endl; std::cout
    // << "left: " << left.transpose() << "  dist: " << left.head<3>().dot(test) - left.w() <<
    // std::endl; std::cout << "right: " << right.transpose() << "  dist: " <<
    // right.head<3>().dot(test) - right.w() << std::endl;

    return { near, far, up, down, left, right };
}

cv::Mat camera_3d_vision_hardware::capture_rgb_image()
{
    return std::move(util::cv_mat_from_pointcloud(*capture_image()));
}

realsense_pcl_grabber &camera_3d_vision_hardware::realsense_controller() const
{
    return *m_grabber;
}

}// namespace hardware