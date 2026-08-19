#include <iostream>

#include <franka/robot.h>
#include <franka/exception.h>

#include <pcl/common/transforms.h>
#include <pcl/visualization/pcl_visualizer.h>

#include <opencv2/opencv.hpp>

#include <boost/archive/text_iarchive.hpp>

#include "camera_realsense.h"
#include "vision.h"

#include "serialization.h"
#include "visualization.h"
#include "world_model.h"

#include "franka_robot.h"

using namespace benchmark;


int main()
{
    try {
        using namespace std::literals;

        benchmark::franka_robot bot;

        auto data =
            std::vector<std::pair<cv::Mat, std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>>>>();

        const auto entities = bot.explore();

        {
            pcl::visualization::PCLVisualizer visu;
            visu.getRenderWindow()->GlobalWarningDisplayOff();
            for (int i = 0; i < bot.exploration_poses(); i++) {
                data.push_back(
                    { cv::Mat(), std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>() });

                pcl::io::loadPCDFile(
                    "./logs/exploration/image_"s + std::to_string(i) + ".pcd"s, *data[i].second);


                visu.addPointCloud(data[i].second, "cloud_"s + std::to_string(i));
            }
            visu.addCoordinateSystem(0.5);
            for (const auto &e : entities) util::draw(visu, e);
            for (const auto &e : entities) std::cout << e << "\n";
            visu.spin();
        }
        if (entities.size() > 0) {
        bot.pick(entities.front());
        } else  {
            std::cout << "No objects found\n";
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
    }
}
