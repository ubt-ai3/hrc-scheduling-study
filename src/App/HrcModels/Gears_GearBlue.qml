import QtQuick
import QtQuick3D

Node{
    Model {
        id: gear_002
        scale.x: 1
        scale.y: 1
        scale.z: 1
        source: "./meshes/gear_002.mesh"
    
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