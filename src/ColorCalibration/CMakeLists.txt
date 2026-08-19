qt_add_executable(color_calibration 
)

qt_add_qml_module(color_calibration
	URI ColorCalibration
	VERSION 1.0
	RESOURCE_PREFIX /qt/qml
	QML_FILES 
		main.qml
	SOURCES 
		main.cpp
		filtered_camera.cpp
		filtered_camera.h
		file_writer.h
		file_writer.cpp
	DESIGNER_SUPPORTED
)

target_link_libraries(color_calibration PRIVATE 
	componentsplugin
	Qt6::Gui Qt6::Qml Qt6::Core Qt6::Widgets Qt6::Quick Qt6::Quick3D
	hardware
)

add_custom_command(TARGET color_calibration
    POST_BUILD
    COMMAND Qt6::windeployqt "$<TARGET_FILE_DIR:color_calibration>/$<TARGET_FILE_NAME:color_calibration>"
)