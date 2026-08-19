import QtQuick

import QtQuick.Dialogs 
import QtQuick.Controls
import QtQuick.Controls.Universal

import Core

ApplicationWindow {
    id: window
    minimumWidth: 1280
    minimumHeight: 720
    visible: true
    Universal.background: Universal.Steel
    Universal.foreground: "white"
    
    
	property int textSize: 15
	property int headerSize: textSize + 5
    
    menuBar: MenuBar {
        font.pointSize: 12
        Menu {
            title: "&File"
            MenuItem {
                text: "&Open..."
                onTriggered: fileDialog.open()
            }
        }
        Menu {
            title: "&Tools"
            MenuItem {
                text: "&Options"
                onTriggered: dialog.open()
            }
            MenuItem {
                text: "NASA TLX"
                onTriggered: pageStack.push("NASA_TLX.qml")
            }
            MenuItem {
                text: "Perfomance and Interaction Quality"
                onTriggered: pageStack.push("PerformanceInteraction.qml")
            }
            MenuItem {
                text: "ATI"
                onTriggered: pageStack.push("ATI.qml")
            }
            MenuItem {
                text: "Experience"
                onTriggered: pageStack.push("Experience.qml")
            }
            MenuItem {
                text: "SUS"
                onTriggered: pageStack.push("SUS.qml")
            }
            MenuItem{
                text: "Demographics"
                onTriggered: pageStack.push("Demographics.qml")
            }
        }

    }
    
    FileDialog {
        objectName: "fileDialog"
        id: fileDialog
        nameFilters: ["JSON files (*.json)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            Settings.load_json(selectedFile)
        }
    }
    
    SettingsDialog{
        id: dialog
    }

    StackView{
        id: pageStack
        anchors.fill: parent
        initialItem: Experiment{}
    }

    function hasDefaultValue(model, defaultValue){
		for(let i = 0; i< model.count; i++){
			if( model.get(i).selected == defaultValue) return true;
		}
		return false;
	}
}
