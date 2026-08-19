import QtQuick
import QtQuick3D

Node{
    Model {
        id: models_Conductor_with_LED
        rotation: Qt.quaternion(0.707107, 0, 0.707107, 0)
        scale.x: 1
        scale.y: 1
        scale.z: 1
        source: "./meshes/models_Conductor_with_LED.mesh"
    
        PrincipledMaterial {
            id: red_material
            baseColor: "#ff9b0000"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
    
        PrincipledMaterial {
            id: orange_material
            baseColor: "#ffcc4d00"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
        materials: [
            red_material,
            orange_material,
            red_material
        ]
    }
}