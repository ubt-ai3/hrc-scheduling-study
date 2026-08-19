qt_add_library(hrcmodels STATIC)

qt_add_qml_module(hrcmodels
    URI HrcModels
	VERSION 1.0
	RESOURCE_PREFIX /qt/qml
    QML_FILES
		Circuits_Baseplate.qml
		Circuits_BaseplateBattery.qml
		Circuits_ConductorBlue.qml
		Circuits_ConductorGreen.qml
		Circuits_ConductorOrange.qml
		Cubes_Baseplate.qml
		Cubes_bigCubeBlue.qml
		Cubes_CubeGreen.qml
		Cubes_CubeOrange.qml
		Gears_Baseplate.qml
		Gears_GearBlue.qml
		Gears_GearGreen.qml
		Gears_GearOrange.qml
		General_CornerMarker.qml
	RESOURCES		
		meshes/basePlate_001.mesh
		meshes/basePlate_002.mesh
		meshes/basePlate_003.mesh
		meshes/bigCube.mesh
		meshes/borderMarker_001.mesh
		meshes/gear_001.mesh
		meshes/gear_002.mesh
		meshes/gear_003.mesh
		meshes/models_ConductorBlue.mesh
		meshes/models_ConductorGreen.mesh
		meshes/models_Conductor_with_LED.mesh
		meshes/pick_Place_BasePlate.mesh
		meshes/pick_Place_Cube.mesh
		meshes/pick_Place_Cube_001.mesh
)
