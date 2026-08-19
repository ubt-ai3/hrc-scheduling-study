import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

import QtQml

import Components

MyDialog {
    id: root
    title: "Your help is needed!"
    modal: true

    closePolicy: Popup.NoAutoClose
    contentItem: Rectangle{
        Universal.background: "white"
        Label{
            anchors.centerIn: parent
            font.family: Style.iconFont.family
            font.weight: Style.iconFont.weight
            font.pixelSize: 200
            text: "question_answer"

            Component.onCompleted: {}
        }
    }

    footer: DialogButtonBox{
        alignment: undefined
        ButtonDefault{
            defaultText: "I'm back"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
