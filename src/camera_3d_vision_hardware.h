#pragma once
#include <mutex>

#include <pcl/common/common.h>
#include <librealsense2/rs.hpp>

#include <opencv2/core/mat.hpp>

#include "view_frustum.h"


namespace hardware {
using point_type = pcl::PointXYZ;
using point_type_colored = pcl::PointXYZRGB;
using point_cloud = pcl::PointCloud<point_type>;
using point_cloud_colored = pcl::PointCloud<point_type_colored>;

/**
 * \class realsense_pcl_grabber
 *
 * connects to realsense camera-device and grabs images and returns pointclouds in pcl format
 *
 * note:
 * - no running backround thread to get data without delay
 */
class realsense_pcl_grabber
{
  public:
    /**
     * \brief tries to connect to a realsense camera-device and
     *			initalizes pipe calling realsense_pcl_grabber::init_pipe()
     *	note:
     *	- if no connection with given params can be established, an exception is thrown from
     *realsense-api
     *	- some common resolutions are not supported
     *	- some combinations might not work
     *	- maybe not all possible resolutions and framerates are listed
     *
     * \param color_resolution tested values: 1920x1080, 1280x720, 640x480, 640x360
     * \param depth_resolution tested values: 1280x720, 640x480, 640x360
     * \param framerate tested values: 30, 15, 6
     * \param config_file_path path to a json config file
     * \param serial_number without a serial number connection to the first device is established
     *							and its serial number is set.
     *							with a serial number connection to this
     *device is established.
     */
    explicit realsense_pcl_grabber(const Eigen::Vector2i &color_resolution = { 1920, 1080 },
        const Eigen::Vector2i &depth_resolution = { 1280, 720 },
        int framerate = 30,
        std::string config_file_path = "",
        std::string serial_number = "");

    ~realsense_pcl_grabber();

    /**
     * \brief stops the pipe, and reestablishes it based on the given params
     *
     *	note:
     *	- same supported params and exceptions as the constructor
     */
    void reset_pipe(const Eigen::Vector2i &color_resolution = { 1920, 1080 },
        const Eigen::Vector2i &depth_resolution = { 1280, 720 },
        int framerate = 30,
        std::string config_file_path = "",
        std::string serial_number = "");

    /**
     * \brief fetches data without any conversion
     *
     * note:
     * - read realsense-api docu (e.g. it waits until a new set of frames becomes available)
     */
    rs2::frameset fetch_raw_data() const;

    /**
     * \brief converts data received from fetch_raw_data() to a pointcloud
     *
     * note:
     * - only use with same, not reseted device
     * - memory management by caller so everybody can use their preferred smart pointer
     */
    point_cloud *cloud_from_frameset(rs2::frameset &frames, float max_z = 10.f) const;

    /**
     * \brief converts data received from fetch_raw_data() to a colored pointcloud
     *
     * note:
     * - only use with same, not reseted device
     * - memory management by caller so everybody can use their preferred smart pointer
     */
    point_cloud_colored *colored_cloud_from_frameset(rs2::frameset &frames,
        float max_z = 10.f) const;

    /**
     * \brief converts data received from fetch_raw_data() to a clipped colored pointcloud
     *
     * note:
     * - only use with same, not reseted device
     * - only works with depth_resolution = {1280, 720}
     */
    point_cloud_colored *clipped_colored_cloud_from_frameset(rs2::frameset &frames,
        float max_z = 10.f) const;

    /**
     * \brief retrieves a organized pointcloud
     *
     * note:
     * - read realsense-api docu (e.g. it waits until a new set of frames becomes available)
     * - memory management by caller so everybody can use their preferred smart pointer
     */
    point_cloud *fetch_cloud(float max_z = 10.f) const;

    /**
     * \brief retrieves a colored, organized pointcloud
     *
     * note:
     * - if a point has no color-information, the xyz values are set to NaN
     * - read realsense-api docu (e.g. it waits until a new set of frames becomes available)
     * - memory management by caller so everybody can use their preferred smart pointer
     */
    point_cloud_colored *fetch_colored_cloud(float max_z = 10.f) const;

    /**
     * \brief retrieves a colored, organized pointcloud
     *
     * note:
     * - only works with depth_resolution = {1280, 720}
     * - if a point has no color-information, the xyz values are set to NaN
     * - read realsense-api docu (e.g. it waits until a new set of frames becomes available)
     * - memory management by caller so everybody can use their preferred smart pointer
     */
    point_cloud_colored *fetch_clipped_colored_cloud(float max_z = 10.f) const;

    Eigen::Vector2i color_resolution() const;
    Eigen::Vector2i depth_resolution() const;
    Eigen::Vector2i clipped_resolution() const;
    int framerate() const;
    std::string config_file_path() const;
    std::string serial_number() const;

  private:
    void init_pipe();

    static point_cloud *points_to_depth_cloud(const rs2::points &points, float max_z) ;

    point_cloud_colored *points_to_colored_cloud(const rs2::points &points,
        const rs2::video_frame &color,
        float max_z) const;

    point_cloud_colored *points_to_clipped_colored_cloud(const rs2::points &points,
        const rs2::video_frame &color,
        float max_z) const;

    Eigen::Vector2i clipped_resolution_from_depth_resoltuion() const;

    rs2::pipeline pipe_;//< connection to the realsense camera-device
    mutable std::mutex capturing_mutex_;

    Eigen::Vector2i color_resolution_;
    Eigen::Vector2i depth_resolution_;
    Eigen::Vector2i clipped_resolution_;
    int framerate_;
    std::string config_file_path_;
    std::string serial_number_;
};


extern const Eigen::Matrix4d camera_matrix_realsense_720p_16to9;
extern const Eigen::Matrix4d camera_matrix_realsense_720p_16to9_clipped;
extern const Eigen::Matrix4d camera_matrix_realsense_480p_16to9;
extern const Eigen::Matrix4d camera_matrix_realsense_480p_16to9_clipped;


/**
 * @brief High-level RealSense camera facade for 3D vision.
 *
 * Owns a realsense_pcl_grabber and adds:
 *   - a view frustum for visibility queries
 *   - depth clamping (max_z)
 *   - convenience methods for capturing colored point clouds and RGB images
 *
 * The camera matrix constants (camera_matrix_realsense_*) at the bottom of
 * this header correspond to the supported resolution/clip combinations and
 * must match the sensor configuration passed to the grabber.
 */
class camera_3d_vision_hardware
{
  public:
    typedef std::shared_ptr<camera_3d_vision_hardware> ptr;

    /**
     * @param config_path Path to a RealSense JSON config file (empty = default settings).
     * @param max_z       Maximum depth (metres) — points beyond this are discarded.
     */
    camera_3d_vision_hardware(const std::string &config_path = "", double max_z = 1.);

    virtual ~camera_3d_vision_hardware();

    /** @brief Captures and returns a depth-clamped colored point cloud in camera coordinates. */
    std::shared_ptr<point_cloud_colored> capture_image();
    /** @brief Returns the view frustum computed from the camera intrinsics and current pose. */
    view_frustum frustum() const;

    /** @brief Returns the color stream resolution as (width, height). */
    Eigen::Vector2i resolution() const;

    /** @brief Captures and returns a BGR color image (no depth). */
    cv::Mat capture_rgb_image();

    /** @brief Provides direct access to the underlying grabber (e.g. for pipe reset). */
    realsense_pcl_grabber &realsense_controller() const;

  protected:
    std::vector<Eigen::Vector4d> generate_frustum_planes() const;

    /** @brief Back-projects a pixel coordinate @p u at depth @p z using focal length @p f and principal point @p c. */
    static double reproject_point(double f, double c, double u, double z)
    {
        return ((u - c) * z) / f;
    };

    realsense_pcl_grabber *m_grabber;

    Eigen::Matrix4d m_camera_matrix;
    double m_min_z;
    double m_max_z;
    view_frustum m_view_frustum;
};
}// namespace hardware