
add_subdirectory(Components)
add_subdirectory(HrcModels)
add_subdirectory(Mock)

qt_add_executable(application main.cpp)


qt_add_qml_module(application
	URI App
	VERSION 1.0
	RESOURCE_PREFIX /qt/qml
	QML_FILES 
		main.qml
		Baseplates.qml
		Demographics.qml
		Experiment.qml
		Operations.qml
		ModelLoader.qml
		NASA_TLX.qml
		TLXButton.qml
		TLXRatings.qml
		TLXWeights.qml
		Scene.qml
		SUS.qml
		Highlight.qml
		SettingsDialog.qml
		InterruptDialog.qml
		Experience.qml
		ATI.qml
		PerformanceInteraction.qml
	DESIGNER_SUPPORTED
)

target_link_libraries(application PRIVATE 
	componentsplugin
	hrcmodelsplugin
	mockplugin
	coreplugin
	core
	Qt6::Quick3D
)

if (MSVC)
	add_custom_command( #to copy python dlls.
    	  TARGET application POST_BUILD
		  COMMAND ${CMAKE_COMMAND} -E copy
               $<TARGET_RUNTIME_DLLS:application> $<TARGET_FILE_DIR:application>
      COMMAND_EXPAND_LISTS
    )
	add_custom_command(TARGET application
    	POST_BUILD
    	COMMAND Qt6::windeployqt "$<TARGET_FILE_DIR:application>/$<TARGET_FILE_NAME:application>"
	)
endif()