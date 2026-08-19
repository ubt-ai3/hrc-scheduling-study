import QtQuick
import QtQuick3D

Node{
    Model {
        id: gear_001
        scale.x: 1
        scale.y: 1
        scale.z: 1
        source: "./meshes/gear_001.mesh"
    
        PrincipledMaterial {
            id: red_material
            baseColor: "#ff9b0000"
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
            red_material,
            green_material,
            red_material
        ]
    }
}