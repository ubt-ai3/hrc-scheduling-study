import QtQuick.Controls.Universal

import QtQuick.Controls
import QtQuick

Button{
    id: root
    property string defaultText : "default"
    property string disabledText : "disabled"
    state: "DEFAULT"

    implicitHeight: 40
    implicitWidth: 100

    anchors.margins: Style.margin
    font.pointSize: 20
    background: Rectangle {
        anchors.fill: parent
        opacity: root.enabled ? 1 : 0.3
        color: Universal.color(root.down ?  Universal.Emerald: Universal.Green)
    }

    states: [
        State{name: "DEFAULT"; PropertyChanges{ root.text: defaultText}},
        State{name: "DISABLED"
            PropertyChanges{ 
                root.text: disabledText
                root.enabled: false
            }
        } 
    ]
}
