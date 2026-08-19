import QtQuick
import QtQuick3D

Node{
    Model {
        id: gear_003
        scale.x: 1
        scale.y: 1
        scale.z: 1
        source: "./meshes/gear_003.mesh"
    
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