import QtQuick
import QtQuick3D

Node{
    Model {
        id: models_ConductorBlue
        rotation: Qt.quaternion(-1.62921e-07, 0, 1, 0)
        scale.x: 9.51999 /2.0
        scale.y: 0.799999 /2.0
        scale.z: 1.42 /2.0
        source: "./meshes/models_ConductorBlue.mesh"
    
        PrincipledMaterial {
            id: red_material
            baseColor: "#ff9b0000"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
    
        PrincipledMaterial {
            id: blue_material
            baseColor: "#ff0002e6"
            roughness: 0.4
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
        materials: [
            red_material,
            blue_material,
            red_material
        ]
    }
}