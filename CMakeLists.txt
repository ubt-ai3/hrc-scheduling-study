cmake_minimum_required(VERSION 3.21)
message("Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

project(experiment
	VERSION 0.1.0
	LANGUAGES CXX
)


# Dependencies
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED COMPONENTS serialization)
find_package(Eigen3 CONFIG REQUIRED)
find_package(PCL CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS  Core Gui Quick3D Qml Widgets Quick QuickControls2)
find_package(Catch2 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(realsense2 CONFIG REQUIRED) 
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python  REQUIRED COMPONENTS Development Interpreter)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(BUILD_DOCS OFF)


#Compiler options
add_library(project_warnings INTERFACE)
if (MSVC)
    target_compile_options(project_warnings INTERFACE 
		/W4
		/wd4127 # suppress warning in eigen3
		/wd4324 # suppress alignment warning in pcl
		/wd4996 # disable derive from iterator deprecated in vtk
		/wd4263 # some opencv warning
	) 
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -pedantic)
endif()



# manually pull and configure franka dependencies
include(FetchContent)
set (BUILD_TESTS  OFF CACHE INTERNAL "Turn off libfranka tests")
set (BUILD_EXAMPLES  OFF CACHE INTERNAL "Turn off libfranka examples")

FetchContent_Declare(_libfranka
	GIT_REPOSITORY https://github.com/frankaemika/libfranka.git
	GIT_TAG f3b8d775a9c847cab32684c8a316f67867761674
)

FetchContent_Declare(_pybind
	GIT_REPOSITORY https://github.com/pybind/pybind11.git
	GIT_TAG v3.0.4
)
FetchContent_MakeAvailable(_pybind _libfranka)


add_library(ikfast src/ikfast.cpp )

add_library(vision 
	src/vision/vision.cpp	
	src/vision/pose_estimation.cpp
	src/vision/visualization.cpp
	src/vision/entity.cpp
)

target_link_libraries(vision PUBLIC
	project_warnings
	Eigen3::Eigen
	Boost::serialization
	spdlog::spdlog
	${OpenCV_LIBS}
	${PCL_LIBRARIES}
)
target_include_directories(vision PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/vision)



add_library(hardware 
	src/world_model.cpp
	src/abstract_robot.cpp
	src/franka_robot.cpp
	src/franka_util.cpp
	src/motion_generator_joint_max_accel.cpp
	src/motion_generator_seq_cart_vel_tau.cpp
	src/view_frustum.cpp
	src/camera_realsense.cpp
	src/camera_3d_vision_hardware.cpp
	src/Camera.cpp
	src/camera_util.cpp
	src/hardware.cpp
	src/serialization.cpp
)

target_link_libraries(hardware PUBLIC 
	ikfast
	project_warnings
	vision
	Eigen3::Eigen
	Franka::Franka
	realsense2::realsense2
	nlohmann_json::nlohmann_json
	spdlog::spdlog
	Qt6::QuickControls2
	${PCL_LIBRARIES}
)
target_include_directories(hardware PUBLIC ${PROJECT_SOURCE_DIR}/src)

add_executable(tests src/tests/main.cpp)
target_link_libraries(tests PRIVATE 
	project_warnings
	Catch2::Catch2WithMain 
	hardware
)
include(CTest)
include(Catch)
catch_discover_tests(tests)

add_subdirectory(src/core)
add_subdirectory(src/App)
add_subdirectory(src/ColorCalibration)

add_executable(hardware_test src/test_franka.cpp)
target_link_libraries(hardware_test PRIVATE
	project_warnings
	hardware
)

if(BUILD_DOCS)
	add_subdirectory ("docs")
	#build docs when building exetutables
	add_dependencies(application Sphinx)
	add_dependencies(hardware_test Sphinx)
endif()

file(COPY ${PROJECT_SOURCE_DIR}/data 
	DESTINATION ${PROJECT_BINARY_DIR}
)

add_custom_command(
        TARGET hardware POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
                ${PROJECT_SOURCE_DIR}/data/color_thresholds_config.json
                ${PROJECT_BINARY_DIR}/data/color_thresholds_config.json
)
