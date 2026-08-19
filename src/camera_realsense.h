#pragma once

#include "hardware.h"
#include "camera_3d_vision_hardware.h"

namespace hardware {

/**
 * @brief Thin camera adapter that bridges camera_3d_vision_hardware to the camera interface.
 *
 * Wraps a camera_3d_vision_hardware instance and implements camera::record() by
 * capturing a colored point cloud and the corresponding RGB image in one call.
 * This is the camera implementation used by the hardware robot path in the HRC experiment.
 */
class camera_realsense final: public camera
{
  public:
    camera_realsense();
    ~camera_realsense() = default;

    virtual std::pair<cv::Mat, std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>>> record() override;

  private:
    camera_3d_vision_hardware cam_;
};
}// namespace hardware