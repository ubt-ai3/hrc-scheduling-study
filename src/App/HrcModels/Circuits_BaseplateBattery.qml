import QtQuick
import QtQuick3D

Node{
    Model {
        id: basePlate_001
        rotation: Qt.quaternion(0.707106, 0, 0.707107, 0)
        scale.x: 40
        scale.y: 10
        scale.z: 40
        source: "./meshes/basePlate_001.mesh"
    
        PrincipledMaterial {
            id: grey_material
            baseColor: "#ff808080"
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
            grey_material,
            orange_material
        ]
    }
}