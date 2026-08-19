pragma Singleton
import QtQuick
import QtQml

QtObject {
    property int margin: 15
    property bool formValidationDisabled: false
    
    property alias iconFont: fontLoader.font

    property var fontLoader: FontLoader{
        id: fontLoader
        source: "./MaterialIconsOutlined-Regular.otf"
    }
}