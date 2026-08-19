import QtQuick.Controls.Universal
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import Components
import Core

MyDialog {
    id: root

    title: "Settings"

    //enable deffered loading of dialog contents
    contentItem: Loader{sourceComponent: settingsDialogContents}
    standardButtons:  Dialog.Cancel | Dialog.Ok

    onAccepted: contentItem.item.accept()
    onOpened: contentItem.item.reset()
    onRejected: contentItem.item.reset()


    Component{
        id: settingsDialogContents
        Rectangle{
            Universal.background: "white"
            CheckBox {
                id: botbox
                anchors.margins: Style.margin
                anchors.top: parent.top
                anchors.left: parent.left
                text: "Enable scheduling for robot"
            }
            CheckBox {
                id: hbox
                anchors.margins: Style.margin
                anchors.top: botbox.bottom
                anchors.left: parent.left
                text: "Enable scheduling for human"
                checked: Settings.human_actor
            }            
            CheckBox {
                id: reschedule_box
                anchors.margins: Style.margin
                anchors.top: hbox.bottom
                anchors.left: parent.left
                text: "Reschedule after interruption"
                checked: Settings.reschedule_after_interrupt
            }
            TextField {
                id: interrups
                width: 60
                anchors.margins: Style.margin
                anchors.left: parent.left
                anchors.top: reschedule_box.bottom
            }
            Label{
                anchors.margins: Style.margin
                anchors.left: interrups.right
                anchors.verticalCenter : interrups.verticalCenter
                text: "Interruptions after these assembly steps"
            }
            TextField{
                id: path
                anchors.margins: Style.margin
                anchors.left: parent.left
                anchors.top: interrups.bottom
                anchors.right: parent.right
                rightPadding: button.width +10
            }
            Button{
                id: button
                anchors.margins: Style.margin
                anchors.rightMargin: 22
                anchors.top: interrups.bottom
                anchors.right: parent.right
                
                height: path.height
                font.family: Style.iconFont.family
                font.pixelSize: 20
                text: "folder_open"
                background: Item{}

                onClicked: fileDialog.open()
            }

            FileDialog{
                id: fileDialog
                onAccepted: path.text = selectedFile;
            }

            function reset(){
                botbox.checked = Settings.robot_actor;
                hbox.checked = Settings.human_actor;
                interrups.text = JSON.stringify(Settings.interrupt_human_after)
                path.text= Settings.task_description
                reschedule_box.checked = Settings.reschedule_after_interrupt
            }
            function accept() {Settings.robot_actor = botbox.checked;
                Settings.human_actor = hbox.checked;
                Settings.interrupt_human_after = JSON.parse(interrups.text)
                Settings.task_description = path.text;
                Settings.reschedule_after_interrupt = reschedule_box.checked;
                Settings.completedWrite();
            }
        }
    }
}
