 import QtQml

 import Core
 import HrcModels

 QtObject{
	function load_baseplate(kind, parent){
		var conductorBase = Qt.createComponent("../HrcModels/Circuits_Baseplate.qml");
        var conductorBattery = Qt.createComponent("../HrcModels/Circuits_BaseplateBattery.qml");
        var cubesBase = Qt.createComponent("../HrcModels/Cubes_Baseplate.qml");
        var gearsBase = Qt.createComponent("../HrcModels/Gears_Baseplate.qml");
        var corner = Qt.createComponent("../HrcModels/General_CornerMarker.qml");


        switch(kind){
            case BaseplateType.ConductorBase:
                return conductorBase.createObject(parent);
            case BaseplateType.ConductorBaseBattery:
                return conductorBattery.createObject(parent);
            case BaseplateType.GreenCorner:
                return corner.createObject(parent);
            case BaseplateType.GearBase:
                return gearsBase.createObject(parent);
            case BaseplateType.CubeBase:
                return cubesBase.createObject(parent);
            default: 
                console.log("../HrcModels/Kind ", plate.kind, " not handled");
                return;        
        }
	}

    function load_entity(type : EntityType, color :EntityColor, parent){
    	var gearBlue = Qt.createComponent("../HrcModels/Gears_GearBlue.qml");
        var gearGreen = Qt.createComponent("../HrcModels/Gears_GearGreen.qml");
        var gearOrange = Qt.createComponent("../HrcModels/Gears_GearOrange.qml");
        
        var conductorBlue= Qt.createComponent("../HrcModels/Circuits_ConductorBlue.qml");
        var conductorGreen= Qt.createComponent("../HrcModels/Circuits_ConductorGreen.qml");
        var conductorOrange= Qt.createComponent("../HrcModels/Circuits_ConductorOrange.qml");
        
        var cubeBlue= Qt.createComponent("../HrcModels/Cubes_bigCubeBlue.qml");
        var cubeGreen= Qt.createComponent("../HrcModels/Cubes_CubeGreen.qml");
        var cubeOrange= Qt.createComponent("../HrcModels/Cubes_CubeOrange.qml");


        switch(type){
            case EntityType.Gear:
                switch(color){
                    case EntityColor.Blue:
                        return gearBlue.createObject(parent)
                    case EntityColor.Green:
                        return gearGreen.createObject(parent)
                    case EntityColor.Orange:
                        return gearOrange.createObject(parent)
               }
               break;
            case EntityType.Conductor:
                switch(color){
                    case EntityColor.Blue:
                        return conductorBlue.createObject(parent)
                    case EntityColor.Green:
                        return conductorGreen.createObject(parent)
                    case EntityColor.Orange:
                        return conductorOrange.createObject(parent)
               }
               break;
            case EntityType.Cube:
            case EntityType.LongCube:
                switch(color){
                    case EntityColor.Blue:
                        return cubeBlue.createObject(parent)
                    case EntityColor.Green:
                        return cubeGreen.createObject(parent)
                    case EntityColor.Orange:
                        return cubeOrange.createObject(parent)
               }
        }
        console.log("../HrcModels/Did not load Kind: ", kind," Color: ", color);
    }

    //convert from z-Up to y-Up
    function to_gui_coordinates(pos){
        var scale = 1000;
        return Qt.vector3d(pos.y * scale, pos.z * scale, pos.x * scale);
    }

 }