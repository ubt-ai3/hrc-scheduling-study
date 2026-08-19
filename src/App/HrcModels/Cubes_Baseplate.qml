import QtQuick
import QtQuick3D

Node{
    Model {
        id: pick_Place_BasePlate
        scale.x: 1
        scale.y: 1
        scale.z: 1
        source: "./meshes/pick_Place_BasePlate.mesh"
    
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