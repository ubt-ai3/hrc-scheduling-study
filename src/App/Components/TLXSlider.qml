import QtQuick.Controls.Universal

import QtQuick
import QtQuick.Controls


Item{
    id: root
    property alias value: slider.value
    property int pointSize: 10
    property bool swapLabels: false
    property alias stepSize: slider.stepSize

    anchors.margins: Style.margin
    implicitWidth: childrenRect.width
    implicitHeight: slider.implicitHeight
    width: implicitWidth
    Label{
        x: 0
        id: from_label
        horizontalAlignment: Text.AlignRight
        text: !swapLabels ? qsTr("Very Low") : qsTr("Perfect") 
        anchors.bottom: slider.bottom
        font.pointSize: pointSize
        width: 70
    }
    Label{
        id: to_label
        text: !swapLabels ? qsTr("Very High") : qsTr("Failure") 
        anchors.left: slider.right
        anchors.bottom: slider.bottom
        font.pointSize: pointSize
    }

    Slider{
        id: slider
        width: 400
        snapMode: Slider.SnapAlways
        anchors.left: from_label.right
        stepSize: 5
        from: 0
        to: 100
        value: 50

        background: Rectangle {
            x: slider.leftPadding
            y: slider.bottomPadding + slider.availableHeight
            implicitHeight: 1
            implicitWidth: slider.availableWidth
            width: slider.availableWidth
            height: implicitHeight
            color: "black"

            Rectangle{
                x: parent.width / 2
                y: - implicitHeight
                implicitWidth: 1
                implicitHeight: 26
                width: implicitWidth
                height: slider.availableHeight 
                color: "black"
            }
            Repeater{
                model: 21
                Rectangle{
                    x: parent.width / 20 * index - offset
                    y: - height
                    implicitWidth: 1
                    implicitHeight: 26
                    width: implicitWidth
                    height: slider.availableHeight * 0.66
                    color: "black"
                    property int offset: index < 12 ? 0 : 1
                }
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 4
            implicitHeight: 26
            height: slider.availableHeight
            color: "red"
        }

    }
}