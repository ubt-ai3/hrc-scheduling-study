import QtQuick
import QtQuick3D

Node{
    Model {
        id: borderMarker_001
        rotation: Qt.quaternion(0.707107, 0, 0.707107, 0)
        scale.x: 5
        scale.y: 1
        scale.z: 5
        source: "./meshes/borderMarker_001.mesh"
    
        PrincipledMaterial {
            id: grey_material
            baseColor: "#ff808080"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
    
        PrincipledMaterial {
            id: green_material
            baseColor: "#ff00cc01"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
        materials: [
            grey_material,
            green_material
        ]
    }
}