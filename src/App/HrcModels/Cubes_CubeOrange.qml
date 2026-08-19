import QtQuick
import QtQuick3D

Node{
    Model {
        id: pick_Place_Cube_001
        y: 59
        scale.x: 1
        scale.y: 1
        scale.z: 1
        source: "./meshes/pick_Place_Cube_001.mesh"
    
        PrincipledMaterial {
            id: red_001_material
            baseColor: "#ff9b0000"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
    
        PrincipledMaterial {
            id: orange_001_material
            baseColor: "#ffcc4d00"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
        materials: [
            red_001_material,
            orange_001_material
        ]
    }
}