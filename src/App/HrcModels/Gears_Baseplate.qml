import QtQuick
import QtQuick3D

Node{
    Model {
        id: basePlate_002
        rotation: Qt.quaternion(0.707107, 0, 0.707107, 0)
        scale.x: 40
        scale.y: 10
        scale.z: 40
        source: "./meshes/basePlate_002.mesh"
    
        PrincipledMaterial {
            id: grey_material
            baseColor: "#ff808080"
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
            grey_material,
            blue_material
        ]
    }
}