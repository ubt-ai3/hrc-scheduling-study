

qt_add_qml_module(core 
	URI Core
	VERSION 1.0
	STATIC
	RESOURCE_PREFIX /qt/qml
	DESIGNER_SUPPORTED
	SOURCES
		baseplate.cpp
		enums.cpp
		operation.cpp
		util.cpp
		task.cpp
		hrc_schedule.cpp
		qml_logger.cpp
		json_logger.cpp
		settings.cpp
		robot_handler.cpp
)

target_link_libraries(core PUBLIC
	project_warnings
	Qt6::Gui Qt6::Qml Qt6::Core Qt6::Widgets Qt6::Quick Qt6::Quick3D
	pybind11::embed
	nlohmann_json::nlohmann_json
	Eigen3::Eigen
	spdlog::spdlog
	vision
	hardware
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_custom_command(TARGET core POST_BUILD
	COMMAND COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/scheduler_py
		${CMAKE_BINARY_DIR}
)