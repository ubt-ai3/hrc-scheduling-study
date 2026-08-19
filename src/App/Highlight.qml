import QtQuick3D
import QtQuick.Controls.Universal
import QtQuick


Node{
    id: root
    visible: false
    eulerRotation.x: -90
    y: 20 
    Rectangle {
         id: rect
         width: 80
         height: 80
         x: - width /2
         y: - height /2
         color: "transparent"
         border.color: Universal.color(Universal.Yellow)
         border.width: 5
         radius: width*0.5
    }
    Component{
        id: light
        Node{
            PointLight{
            }
        }
    }
    Loader3D{ sourceComponent: light; position:  Qt.vector3d((rect.width - rect.border.width) /2,0,20)}
    Loader3D{ sourceComponent: light; position:  Qt.vector3d(-(rect.width - rect.border.width) /2,0,20)}
    Loader3D{ sourceComponent: light; position:  Qt.vector3d(0,(rect.width - rect.border.width) /2,20)}
    Loader3D{ sourceComponent: light; position:  Qt.vector3d(0,-(rect.width - rect.border.width) /2,20)}
}                
