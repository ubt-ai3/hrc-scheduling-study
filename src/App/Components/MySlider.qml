import QtQuick.Controls.Universal

import QtQuick
import QtQuick.Controls

Item{
    id: root
    property alias value: slider.value
    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize
    property int pointSize: 10

    anchors.margins: Style.margin

    implicitWidth: slider.implicitWidth + from_label.implicitWidth + to_label.implicitWidth + 2* Style.margin
    implicitHeight: slider.implicitHeight
    Label{
        id: from_label
        text: slider.from
        anchors.left: parent.left
        anchors.verticalCenter: slider.verticalCenter

        font.pointSize: pointSize
    }
    Label{
        id: to_label
        text: slider.to
        anchors.right: parent.right
        anchors.verticalCenter: slider.verticalCenter

        font.pointSize: pointSize
    }

    Slider{
        id: slider
        anchors.left: from_label.right
        anchors.right: to_label.left
        stepSize: 1

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 300
            implicitHeight: 4
            width: slider.availableWidth
            height: implicitHeight
            radius: 2
            color: "white"
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 50
            implicitHeight: 26
            radius: 5
            color: slider.pressed ? "#f0f0f0": "#f6f6f6"
            border.color: "#bdbebf"
            Label{
                anchors.centerIn : parent
                text: Math.floor(slider.value)
                color: "black"
                font.pointSize: pointSize
            }

        }

    }
}