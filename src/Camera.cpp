#include "Camera.hpp"

#include <fstream>
#include <librealsense2/rs.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/h/rs_option.h>
#include <librealsense2/h/rs_frame.h>

#include <pcl/filters/passthrough.h>

#include <librealsense2/rs_advanced_mode.hpp>

realsense_camera::realsense_camera() : warmed_up_(false), take_special_conf_(true), align_to(nullptr)
{
    auto cfg = rs2::config();

    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480);
    cfg.enable_stream(RS2_STREAM_DEPTH, 848, 480);
    rs2::pipeline_profile selection = pipe_.start(cfg); 
    rs400::advanced_mode dev = selection.get_device();

    std::ifstream input("./data/realsense.json");
    dev.load_json(std::string(
                std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()));
}

realsense_camera::realsense_camera(std::vector<int> dev, std::vector<int> sensor, std::vector<int> prof) {}

realsense_camera::~realsense_camera()
{
    try {
        delete align_to;
    } catch (...) {
        std::cout << "massive error" << std::endl;
    }
}

std::pair<cv::Mat, std::shared_ptr<pcl::PointCloud<pcl::PointXYZRGB>>> realsense_camera::record()
{
    take_picture();
    return { get_color(), get_depth().back() };
}


std::tuple<int, int, int> RGB_Texture(rs2::video_frame texture, rs2::texture_coordinate Texture_XY)
{
    // Get Width and Height coordinates of texture
    int width = texture.get_width();// Frame width in pixels
    int height = texture.get_height();// Frame height in pixels

    // Normals to Texture Coordinates conversion
    int x_value = std::min(std::max(int(Texture_XY.u * width + .5f), 0), width - 1);
    int y_value = std::min(std::max(int(Texture_XY.v * height + .5f), 0), height - 1);

    int bytes = x_value * texture.get_bytes_per_pixel();// Get # of bytes per pixel
    int strides = y_value * texture.get_stride_in_bytes();// Get line width in bytes
    int Text_Index = (bytes + strides);

    const auto New_Texture = reinterpret_cast<const uint8_t *>(texture.get_data());

    // RGB components to save in tuple
    int NT1 = New_Texture[Text_Index];
    int NT2 = New_Texture[Text_Index + 1];
    int NT3 = New_Texture[Text_Index + 2];

    return std::tuple<int, int, int>(NT1, NT2, NT3);
}

cloud_pointer PCL_Conversion(const rs2::points &points, const rs2::video_frame &color)
{

    // Object Declaration (Point Cloud)
    cloud_pointer cloud(new point_cloud);

    // Declare Tuple for RGB value Storage (<t0>, <t1>, <t2>)
    std::tuple<uint8_t, uint8_t, uint8_t> RGB_Color;

    //================================
    // PCL Cloud Object Configuration
    //================================
    // Convert data captured from Realsense camera to Point Cloud
    auto sp = points.get_profile().as<rs2::video_stream_profile>();

    cloud->width = static_cast<uint32_t>(sp.width());
    cloud->height = static_cast<uint32_t>(sp.height());
    cloud->is_dense = false;
    cloud->points.resize(points.size());

    auto Texture_Coord = points.get_texture_coordinates();
    auto Vertex = points.get_vertices();

    // Iterating through all points and setting XYZ coordinates
    // and RGB values
    for (int i = 0; i < points.size(); i++) {
        //===================================
        // Mapping Depth Coordinates
        // - Depth data stored as XYZ values
        //===================================
        cloud->points[i].x = Vertex[i].x;
        cloud->points[i].y = Vertex[i].y;
        cloud->points[i].z = Vertex[i].z;

        // Obtain color texture for specific point
        RGB_Color = RGB_Texture(color, Texture_Coord[i]);

        // Mapping Color (BGR due to Camera Model)
        cloud->points[i].r = std::get<0>(RGB_Color);// Reference tuple<2>
        cloud->points[i].g = std::get<1>(RGB_Color);// Reference tuple<1>
        cloud->points[i].b = std::get<2>(RGB_Color);// Reference tuple<0>
    }

    return cloud;// PCL RGB Point Cloud generated
}

void realsense_camera::take_picture()
{
    // get frame and extract color and depth information

    if (!warmed_up_) {

        for (int i = 0; i < 100; i++) {
            rs2::frameset fr = pipe_.wait_for_frames();
        }
        warmed_up_ = true;
    }
    rs2::align align_to_color(RS2_STREAM_COLOR);

    fr_ = pipe_.wait_for_frames();
    fr_ = align_to_color.process(fr_);

    // get intrinsics
    rs2::video_stream_profile prof =
        pipe_.get_active_profile().get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
    intr_ = prof.get_intrinsics();

    rs2::frame color = fr_.get_color_frame();
    rs2::frame depth = fr_.get_depth_frame();

    const int width_col = color.as<rs2::video_frame>().get_width();
    const int height_col = color.as<rs2::video_frame>().get_height();
    cv::Mat temp(
        cv::Size(width_col, height_col), CV_8UC3, (void *)color.get_data(), cv::Mat::AUTO_STEP);
    cv::cvtColor(temp, color_mat_, cv::COLOR_BGR2RGB);


    rs2::pointcloud pc;
    pc.map_to(color);
    auto points = pc.calculate(depth);

    auto cloud = PCL_Conversion(points, color);
    for (auto& vertex: *cloud)
        if (vertex.z < 0.07 || vertex.z > 1.0) {
            vertex.x = vertex.y = vertex.z = std::numeric_limits<float>::quiet_NaN();
         }


    std::vector<cloud_pointer> layers;
    layers.push_back(cloud);

    depth_mat_ = layers;
}


/*void camera::calibrate_camera(int px, int py, double p_size)
{
        cv::Size patternsize(px, py);
        std::vector<cv::Point3f> real_points;
        std::vector<cv::Point2f> image_points;
        std::vector<cv::Point2f> image_points2;
        std::vector<std::vector<cv::Point3f>> final_real;
        std::vector<std::vector<cv::Point2f>> final_image;

        std::vector<cv::String> images;
        std::string path("./images/*.jpg");

        cv::glob(path, images);


        //prepare real world points
        for (int i = 0; i < py; i++)
        {
                for (int j = 0; j < px; j++)
                {
                        real_points.push_back(cv::Point3f(j * p_size, i * p_size, 0));
                        std::cout << real_points.back() << std::endl;
                }
        }


        cv::Mat frame, gray, und;
        bool found;

        for (int i = 0; i < images.size(); i++)
        {
                frame = cv::imread(images[i]);
                cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

                found= findChessboardCorners(gray, patternsize, image_points,
cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);

                if (found)
                {
                        cv::TermCriteria criteria(cv::TermCriteria::MAX_ITER |
cv::TermCriteria::EPS, 30, 0.001);

                        cv::cornerSubPix(gray, image_points, cv::Size(11, 11), cv::Size(-1, -1),
criteria);

                        cv::drawChessboardCorners(gray, patternsize, image_points, found);

                        std::string filename(std::to_string(i) + "_corn.jpg");
                        std::cout << filename << std::endl;

                        cv::imwrite(filename, gray);

                        final_real.push_back(real_points);
                        final_image.push_back(image_points);

                        cv::imshow("Image", gray);
                        cv::waitKey(200);

                        std::cout << "herer" << std::endl;

                }

        }

        cv::destroyAllWindows();

        cv::Mat cameraMatrix, distCoeffs, R, T;
        cv::calibrateCamera(final_real, final_image, frame.size(), cameraMatrix, distCoeffs, R, T);

        double fovy, fovx, focal_length, ratio;

        cv::Point2d principal;

        cv::calibrationMatrixValues(cameraMatrix, frame.size(), 5.5, 5.5, fovx, fovy, focal_length,
principal, ratio);

        std::cout << "cameraMatrix : " << cameraMatrix << std::endl;
        std::cout << "distCoeffs : " << distCoeffs << std::endl;
        std::cout << "Rotation vector : " << R << std::endl;
        std::cout << "Translation vector : " << T << std::endl;

        std::cout << "new fovx " << fovx << " | fovy " << fovy << std::endl;
        std::cout << "principal " << principal.x << " | " << principal.y << std::endl;

        for (int i = 0; i < images.size(); i++)
        {
                frame = cv::imread(images[i]);
                //cv::cvtColor(frame, gray, cv::COLOR_BGR2RGB);

                cv::undistort(frame, und, cameraMatrix, distCoeffs);

                std::string filename(std::to_string(i) + "_und.jpg");
                std::cout << filename << std::endl;

                cv::imwrite(filename, und);

        }



}*/

void realsense_camera::print_profiles()
{
    // The context represents the current platform with respect to connected devices
    rs2::context ctx;

    // Using the context we can get all connected devices in a device list
    rs2::device_list devices = ctx.query_devices();

    rs2::device selected_device;
    if (devices.size() == 0) {
        std::cerr << "No device connected, please connect a RealSense device" << std::endl;

        // To help with the boilerplate code of waiting for a device to connect
        // The SDK provides the rs2::device_hub class
        rs2::device_hub device_hub(ctx);

        // Using the device_hub we can block the program until a device connects
        selected_device = device_hub.wait_for_device();
    } else {
        std::cout << "Found the following devices:\n" << std::endl;

        // device_list is a "lazy" container of devices which allows
        // The device list provides 2 ways of iterating it
        // The first way is using an iterator (in this case hidden in the Range-based for loop)
        int index = 0;
        for (rs2::device device : devices) {
            std::cout << "  " << index++ << " : " << device.get_info(RS2_CAMERA_INFO_NAME)
                      << std::endl;

            std::vector<rs2::sensor> sensors = device.query_sensors();

            std::cout << "Device consists of " << sensors.size() << " sensors:\n" << std::endl;
            int sensor_index = 0;
            // We can now iterate the sensors and print their names
            for (rs2::sensor sensor : sensors) {
                std::cout << "  " << sensor_index++ << " : "
                          << sensor.get_info(RS2_CAMERA_INFO_NAME)
                          << std::endl;

                if (rs2::depth_sensor dpt_sensor = sensor.as<rs2::depth_sensor>()) {
                    float scale = dpt_sensor.get_depth_scale();
                    std::cout << "Scale factor for depth sensor is: " << scale << std::endl;
                } else
                    std::cout << "Given sensor is not a depth sensor" << std::endl;

                std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();

                // Usually a sensor provides one or more streams which are identifiable by their
                // stream_type and stream_index Each of these streams can have several profiles (e.g
                // FHD/HHD/VGA/QVGA resolution, or 90/60/30 fps, etc..)
                // The following code shows how to go over a sensor's stream profiles, and group the
                // profiles by streams.
                std::map<std::pair<rs2_stream, int>, int> unique_streams;
                for (auto &&sp : stream_profiles) {
                    unique_streams[std::make_pair(sp.stream_type(), sp.stream_index())]++;
                }
                std::cout << "Sensor consists of " << unique_streams.size()
                          << " streams: " << std::endl;
                for (size_t i = 0; i < unique_streams.size(); i++) {
                    auto it = unique_streams.begin();
                    std::advance(it, i);
                    std::cout << "  - " << it->first.first << " #" << it->first.second << std::endl;
                }

                // Next, we go over all the stream profiles and print the details of each one
                std::cout << "Sensor provides the following stream profiles:" << std::endl;
                int profile_num = 0;
                for (rs2::stream_profile stream_profile : stream_profiles) {
                    // A Stream is an abstraction for a sequence of data items of a
                    //  single data type, which are ordered according to their time
                    //  of creation or arrival.
                    // The stream's data types are represented using the rs2_stream
                    //  enumeration
                    rs2_stream stream_data_type = stream_profile.stream_type();

                    // The rs2_stream provides only types of data which are
                    //  supported by the RealSense SDK
                    // For example:
                    //    * rs2_stream::RS2_STREAM_DEPTH describes a stream of depth images
                    //    * rs2_stream::RS2_STREAM_COLOR describes a stream of color images
                    //    * rs2_stream::RS2_STREAM_INFRARED describes a stream of infrared images

                    // As mentioned, a sensor can have multiple streams.
                    // In order to distinguish between streams with the same
                    //  stream type we can use the following methods:

                    // 1) Each stream type can have multiple occurances.
                    //    All streams, of the same type, provided from a single
                    //     device have distinct indices:
                    int stream_index = stream_profile.stream_index();

                    // 2) Each stream has a user-friendly name.
                    //    The stream's name is not promised to be unique,
                    //     rather a human readable description of the stream
                    std::string stream_name = stream_profile.stream_name();

                    // 3) Each stream in the system, which derives from the same
                    //     rs2::context, has a unique identifier
                    //    This identifier is unique across all streams, regardless of the stream
                    //    type.
                    std::cout << std::setw(3) << profile_num << ": " << stream_data_type << " #"
                              << stream_index;

                    // As noted, a stream is an abstraction.
                    // In order to get additional data for the specific type of a
                    //  stream, a mechanism of "Is" and "As" is provided:
                    if (stream_profile
                            .is<rs2::video_stream_profile>())//"Is" will test if the type
                                                             // tested is of the type given
                    {
                        // "As" will try to convert the instance to the given type
                        rs2::video_stream_profile video_stream_profile =
                            stream_profile.as<rs2::video_stream_profile>();

                        // After using the "as" method we can use the new data type
                        //  for additinal operations:
                        std::cout << " (Video Stream: " << video_stream_profile.format() << " "
                                  << video_stream_profile.width() << "x"
                                  << video_stream_profile.height() << "@ "
                                  << video_stream_profile.fps() << "Hz)";
                    }
                    std::cout << std::endl;
                    profile_num++;
                }
            }
        }
    }
}

void realsense_camera::take_picture_with_profile(int dev, int sen, int prof)
{
    rs2::context ctx;
    rs2::pipeline pip;
    // Using the context we can get all connected devices in a device list
    rs2::device_list devices = ctx.query_devices();

    rs2::config conf;

    if (dev >= devices.size()) {
        std::cout << "printf choose smaller device number" << std::endl;
        return;
    }
    rs2::device device = devices[dev];

    // choose sensor
    std::vector<rs2::sensor> sensors = device.query_sensors();

    if (sen >= sensors.size()) {
        std::cout << "choose smaller sensor number" << std::endl;
        return;
    }
    rs2::sensor sensor = sensors[sen];

    // choose stream profile
    std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();

    if (prof >= stream_profiles.size()) {
        std::cout << "choose smaller profile number" << std::endl;
    }
    rs2::stream_profile profile = stream_profiles[prof];
    rs2_stream type = profile.stream_type();

    conf.enable_device(device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

    std::cout << device.get_info(RS2_CAMERA_INFO_NAME) << " | "
              << sensor.get_info(RS2_CAMERA_INFO_NAME) << std::endl;

    // conf.enable_stream(type, -1);
    conf.enable_stream(type, -1);

    if (conf.can_resolve(pip)) {
        std::cout << "hell yeah" << std::endl;
    }

    pip.start(conf);

    std::cout << pip.get_active_profile().get_streams().size() << std::endl;

    std::cout << "hdre1" << std::endl;

    for (int i = 0; i < 100; i++) {
        rs2::frameset fr = pip.wait_for_frames();
    }

    rs2::frameset fr = pip.wait_for_frames();
    rs2::frame frame = fr.get_infrared_frame();
    std::cout << "hdre" << std::endl;
    pip.stop();

    const int width_col = frame.as<rs2::video_frame>().get_width();
    const int height_col = frame.as<rs2::video_frame>().get_height();
    cv::Mat temp(
        cv::Size(width_col, height_col), CV_8UC1, (void *)frame.get_data(), cv::Mat::AUTO_STEP);

    // std::cout << temp << std::endl;

    // cv::cvtColor(temp, temp, cv::COLOR_BGR2RGB);

    cv::imwrite(std::string("infra_lrft.jpg"), temp);
}

cv::Mat realsense_camera::get_color() { return color_mat_; }

std::vector<cloud_pointer> realsense_camera::get_depth() { return depth_mat_; }

Eigen::Vector3d realsense_camera::get_camera_coordinate_of_pixel(int x, int y)
{
    // get first depth frame
    rs2::depth_frame depth = fr_.get_depth_frame();

    // get distance
    auto dist = depth.get_distance(x, y);

    float real_point[3];
    float pixel[2];

    // set pixel
    pixel[0] = x;
    pixel[1] = y;

    rs2_deproject_pixel_to_point(real_point, &intr_, pixel, dist);

    // std::cout << "(" << real_point[0] << "|" << real_point[1] << "|" << real_point[2] << ")" <<
    // std::endl;

    return Eigen::Vector3d(real_point[0], real_point[1], real_point[2]);
}

cv::Point2f realsense_camera::get_pixel_coordinate_of_camera_point(float x, float y, float z)
{
    float real_point[3];
    float pixel[2];

    // set real point
    real_point[0] = x;
    real_point[1] = y;
    real_point[2] = z;

    // rs2_deproject_pixel_to_point(real_point, &intr, pixel, dist);

    auto stream =
        pipe_.get_active_profile().get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
    auto intrin = stream.get_intrinsics();

    rs2_project_point_to_pixel(pixel, &intrin, real_point);

    // std::cout << "(" << real_point[0] << "|" << real_point[1] << "|" << real_point[2] << ")" <<
    // std::endl;

    return cv::Point2f(pixel[0], pixel[1]);
}
