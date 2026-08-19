import QtQuick.Controls.Universal

import QtQuick3D
import QtQuick
import QtQuick3D.Particles3D

import Core

Node {
    id: root
    property var operationsData: []
    property var instances: []
    ModelLoader {
        id: loader
    }


    onOperationsDataChanged: {
        while (instances.length > 0) {
            instances.pop().destroy();
        }
        operationsData.forEach(op => {

            let start_instance = loader.load_entity(op.type, op.color, null);
            start_instance.position =Qt.binding( function() {return loader.to_gui_coordinates(op.start_position)});
            start_instance.eulerRotation.y = Qt.binding(function() { return op.start_rotation * (180 / Math.PI)});
            start_instance.visible = Qt.binding(function() {return op.display});

            let end_instance = loader.load_entity(op.type, op.color, null);
            end_instance.position = loader.to_gui_coordinates(op.end_position);
            end_instance.eulerRotation.y = op.end_rotation * (180 / Math.PI);
            end_instance.visible = false
            end_instance.opacity = 0.2

            let new_op = operation.createObject(root, {wrapped_op: op, model: start_instance, end_model: end_instance});
            instances.push(new_op );

        });
    }

    Component {
        id: operation
        Node {
            id: node
            //TODO this should be porperty Operation
            property var wrapped_op
            property Node model
            property Node end_model
            Highlight{id: h}


            states: [
                State { name: "start";when: wrapped_op.state == OperationState.Open},
                State { name: "closed";when: wrapped_op.state == OperationState.Closed},
                State { name: "robotNext";when: wrapped_op.state == OperationState.RobotNext},
                State { name: "humanNext";when: wrapped_op.state == OperationState.HumanNext}
            ]

            transitions: [
                Transition{
                    to: "humanNext"
                    SequentialAnimation{
                        PauseAnimation{ duration: 500}
                        PropertyAction{ target: end_model; property: "visible"; value: true}
                        PropertyAction{ target: h; property: "visible"; value: true}
                        SequentialAnimation{
                            loops: Animation.Infinite
                            NumberAnimation { target: end_model; property: "opacity"; to: 0.7; duration: 600; 
                                                easing.type: Easing.InQuad}
                            NumberAnimation { target: end_model; property: "opacity"; to: 0.2; duration: 600
                                                easing.type: Easing.OutQuad}
                            PauseAnimation { duration: 200}
                        }
                    }
                },
                Transition {
                    from: "robotNext";to: "closed"
                    ParallelAnimation {                        
                        Vector3dAnimation {
                            target: model;property: "position"
                            to: loader.to_gui_coordinates(wrapped_op.end_position)
                        }
                        RotationAnimation {
                            target: model;property: "eulerRotation.y"
                            to: wrapped_op.end_rotation * (180 / Math.PI)
                        }
                    }
                },
                Transition {
                    from: "humanNext"; to: "closed"
                    SequentialAnimation{
                        PropertyAction{ target: h; property: "visible"; value: false}
                        PropertyAction{ target: model; property: "visible"; value: false}
                        NumberAnimation { target: end_model; property: "opacity"; to: 1; duration: 100; 
                                                easing.type: Easing.InQuad}
                    }
                }
            ]
            //take ownership of assigned models, TODO maybe construct these here instead?
            Component.onCompleted: {
                h.position = Qt.binding(function() { let offset = Qt.vector3d(0, 45, 0); return offset.plus(model.position)})
                h.eulerRotation.y = Qt.binding(function() { return model.eulerRotation.y});
                h.scale.x = (wrapped_op.type == EntityType.LongCube) ? 2 : 1

                model.parent = this
                end_model.parent = this
            }
        }    
    }
}
