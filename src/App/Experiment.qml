import QtQuick.Controls.Universal

import QtQuick3D
import QtQuick3D.Helpers

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import Components
import Mock
import Core


Item{
    RowLayout{
        anchors.fill: parent
        anchors.margins: Style.margin
        Scene{
            Layout.margins: Style.margin
        }
        Rectangle {
            width: 2    
            Layout.fillHeight: true
            Layout.margins: Style.margin
            border.color: "lightgray"
            border.width: 1
        }

        ColumnLayout{
            Rectangle{
                id: timer
                property var start_time: new Date()
                property var time_format: "mm:ss"
                Layout.preferredWidth: 280
                Layout.preferredHeight: 60
                color: "transparent"
                border.color: clock.running ? Universal.color( Universal.Orange) :  "lightgray" 
                Text{
                    id: timer_label
                    anchors.margins: Style.margin
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    font.pointSize: 20
                    color: "white"
                    text: Qt.formatTime(new Date(0),timer.time_format)
                     Timer{
                        id: clock
                        interval: 1
                        repeat: true
                        running: false
                        onTriggered:
                        {
                            parent.text = Qt.formatTime(new Date(new Date() - timer.start_time) ,timer.time_format)
                        }
                    }
                }
                function start(){
                    timer.start_time = new Date()
                    clock.start()
                    
                }
                function stop(){
                    clock.stop()
                }
                function reset() {
                    clock.stop()
                    timer.start_time = new Date(0)
                    timer_label.text = Qt.formatTime(new Date(0), time_format)
                }

                Connections{
                    target: task
                    function onStateChanged(){
                        if (task.state == TaskState.NotStarted) timer.reset() // resets timer when another task is loaded
                        if (task.state == TaskState.Done) {
                            timer.stop()
                            taskDoneDialog.open()
                        }          
                    }
                }
            }

            Layout.margins: Style.margin
            Rectangle{
                Layout.fillHeight: true
            }
            ButtonDefault {
                id: button
                Layout.preferredWidth: 280
                Layout.preferredHeight: 100
                defaultText: task.state == TaskState.Running ? "Assembly step \n completed" : "Start"
                disabledText: "Waiting..."

                state: { if(task.state != TaskState.Running) return "DEFAULT";
                         else if (has_human_op()) return "DEFAULT";
                         return "DISABLED";
                }

                function has_human_op(){
                    for(const op of task.operations){
                        if(op.state == OperationState.HumanNext) return true;
                    }
                    return false;
                }
                onClicked: {
                    if(task.state == TaskState.Running){
                    enabled = false
                    task.human_op_completed() 
                    button_timer.start()
                    } 
                    else {
                    task.start()
                    timer.start()
                    }
                }
                Timer{
                    id: button_timer
                    interval: 700
                    onTriggered: parent.enabled = true
                }
            }
        }
    }
    InterruptDialog{
        visible: task.state == TaskState.Interrupted
        onAccepted: task.human_returned_from_interrupt();
    }

    Task{
        id: task
        settings: Settings
        connect_to_hardware: false
    }

    MessageDialog {
        id: taskDoneDialog
        text: "Task Completed"
    }
}