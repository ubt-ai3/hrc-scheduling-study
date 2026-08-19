import QtQuick3D
import QtQuick3D.Helpers
import QtQuick.Controls.Universal
import QtQuick
import QtQuick.Layouts

View3D {
    Layout.fillHeight: true
    Layout.fillWidth: true

    environment: SceneEnvironment {
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
    }
    camera: PerspectiveCamera {
        id: camera
        fieldOfView: 60
        position: Qt.vector3d(50, 700, 900)
        Component.onCompleted: lookAt(Qt.vector3d(50, 0, -40))
    }

    //
    //Lighting
    //

    WasdController {
        controlledObject: camera
    }
    
    Model {
        scale: Qt.vector3d(800, 800, 800)
        eulerRotation.x: 90
        geometry: GridGeometry {
            horizontalLines: 20
            verticalLines: 20
        }
        materials: [ DefaultMaterial { } ]
    }


    DirectionalLight {
        eulerRotation.x: -120
        brightness: 0.6
        ambientColor: Qt.tint("white", "lightgray")
    }

    //
    // Objects
    //
    Baseplates{
        id: baseplates
        plates: task.baseplates
    }

    Operations{
        id: operations
        operationsData: task.operations

    }
}