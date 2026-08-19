#include "camera_realsense.h"

namespace hardware {


camera_realsense::camera_realsense() : cam_("./data/realsense.json") {}

std::pair<cv::Mat, std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>>> camera_realsense::record()
{
    return { cam_.capture_rgb_image(), cam_.capture_image() };
}

}// namespace hardware


