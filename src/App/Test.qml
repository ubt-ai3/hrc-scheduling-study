import QtQuick 2.15
import QtQuick.Controls 2.15


Window {
    width: 1000
    height: 500
    SettingsDialog{
        id: dialog
    }

    Item {
        id: __materialLibrary__
    }

    SettingsDialog {
        id: settingsDialog
        x: -100
        y: 305
    }
}
