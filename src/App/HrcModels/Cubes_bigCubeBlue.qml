import QtQuick
import QtQuick3D

Node{
    Model {
        id: bigCube
        scale.x: 1
        scale.y: 1
        scale.z: 1
        source: "./meshes/bigCube.mesh"
    
        PrincipledMaterial {
            id: blue_002_material
            baseColor: "#ff0002e6"
            roughness: 0.4
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
    
        PrincipledMaterial {
            id: red_002_material
            baseColor: "#ff9b0000"
            roughness: 0.5
            cullMode: Material.NoCulling
            alphaMode: PrincipledMaterial.Opaque
        }
        materials: [
            blue_002_material,
            red_002_material
        ]
    }
}