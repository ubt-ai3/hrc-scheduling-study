#include <catch2/catch_test_macros.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/visualization/pcl_visualizer.h>

#include "vision.h"
#include "serialization.h"
#include "visualization.h"


// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

#include "Camera.hpp"
#include <franka_robot.h>

// TEST_CASE("object detection")
//{
//     // load data and expected solution
//     auto detection = benchmark::Detection("data/tests/color_config_image5.json");
//
//     const auto img = cv::imread("data/tests/image_5.png", cv::IMREAD_COLOR);
//     auto cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>();
//
//     pcl::io::loadPCDFile<pcl::PointXYZRGB>("data/tests/image_5.pcd", *cloud);
//
//     std::ifstream ifs("data/tests/image_5.archive");
//     boost::archive::text_iarchive ia(ifs);
//
//     std::vector<benchmark::Entity> control;
//     ia >> control;
//
//
//     // perform test
//     const auto result = detection.find_objects(img, cloud);
//
//     REQUIRE(result.size() == 10);
//
//     for (const auto &e : control) {
//         INFO("" << e);
//         CHECK(std::count(result.begin(), result.end(), e) == 1);
//     }
//  }

 TEST_CASE("main")
{
     const auto color = benchmark::Entity::color::orange;

     auto franka = benchmark::franka_robot();
     auto detection = benchmark::Detection("data/thresholds.realsense.json");
     // auto img = cv::imread("data/tests/missing_conductors/image_0.png", cv::IMREAD_COLOR);
     // auto cloud = std::make_shared<pcl::PointCloud<pcl::PointXYZRGB>>();
     // pcl::io::loadPCDFile<pcl::PointXYZRGB>("data/tests/missing_conductors/image_0.pcd",
     //*cloud); 
     auto cam = realsense_camera(); auto [img, cloud] = cam.record();
     pcl::transformPointCloud(*cloud, *cloud, franka.world_T_camera());


     cv::Mat segmented;
     detection.segment(img, segmented, color);
     auto res = detection.find_objects(img, cloud);
     pcl::visualization::PCLVisualizer visu;
     visu.addPointCloud(cloud);
     cv::imshow("segmented", img);
     cv::waitKey();

     auto contours = detection.top_contours(img, *cloud);
     for (const auto &c : contours) {
         if (c.color == color) {
             benchmark::util::draw(visu, c.oobb);
             std::cout << "Contour center:" << c.oobb.position_;
             std::cout << "\t" << c.color;
             auto type = detection.estimate_type(c);
             if (type.has_value()) std::cout << "\t" << *type;
             std::cout << "\n";
         }
     }


     for (const auto &e : res)
         if (e.color_ == color) benchmark::util::draw(visu, e);
     visu.addCoordinateSystem(0.1);
     visu.spin();


 }

//TEST_CASE("main")
//{
//    auto cam = realsense_camera();
//    cam.take_picture();
//    auto [color, depth] = cam.record();
//    cv::imshow("img", color);
//
//    std::cout << "depth shape: " << depth->width << " " << depth->height << "\n";
//    std::cout << "color shape: " << color.size() << "\n";
//    cv::waitKey();
//
//    auto visu = pcl::visualization::PCLVisualizer();
//
//    visu.addCoordinateSystem(0.1);
//    visu.addPointCloud(depth);
//    visu.spin();
//}