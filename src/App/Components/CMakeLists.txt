
set_source_files_properties(Style.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties( ${PROJECT_SOURCE_DIR}/data/icons/MaterialIconsOutlined-Regular.otf PROPERTIES
	QT_RESOURCE_ALIAS MaterialIconsOutlined-Regular.otf
)

qt_add_qml_module(components
    URI "Components"
	VERSION 1.0
	STATIC
	RESOURCE_PREFIX /qt/qml
    QML_FILES
		ButtonDefault.qml
		DebugRectangle.qml
		Style.qml
		MyDialog.qml
		MySlider.qml
		TLXSlider.qml
	RESOURCES
		${PROJECT_SOURCE_DIR}/data/icons/MaterialIconsOutlined-Regular.otf
	DESIGNER_SUPPORTED
)

