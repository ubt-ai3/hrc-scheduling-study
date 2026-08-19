import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls.Universal

import Components
import ColorCalibration

ApplicationWindow {
    id: root
    width: 1280
    height: 720
    visible: true

    Universal.background: Universal.Steel
    Universal.foreground: "white"
         
    FileDialog{
        id: saveDialog
        fileMode: FileDialog.SaveFile;
        nameFilters: [ "JSON files (*.json)"]
        onAccepted: 
        {
            let content = new Object()
            for(let i = 0; i< colorBox.model.length; i++){
                let color = colorBox.model[i]
                let t = thresholds_model.t_list[i]
                content["hsv_" + color + "_lo"] = [t.h_min, t.s_min, t.v_min]
                content["hsv_" + color + "_hi"] = [t.h_max, t.s_max, t.v_max]
            }
            FileWriter.write_json(selectedFile, JSON.stringify(content))
        }
    }

    FileDialog{
        id: loadDialog
        fileMode: FileDialog.OpenFile;
        nameFilters: [ "JSON files (*.json)"]
        defaultSuffix: ".json"
        onAccepted: {
            let content = JSON.parse(FileWriter.read_file(selectedFile))
            for(let i = 0; i< colorBox.model.length; i++){
                let color = colorBox.model[i]
                let t = thresholds_model.t_list[i]
                let low = content["hsv_" + color + "_lo"]
                let high = content["hsv_" + color + "_hi"]
                
                t.h_min = low[0]; t.s_min = low[1]; t.v_min = low[2]
                t.h_max = high[0]; t.s_max = high[1]; t.v_max = high[2]
            }
            root.update_sliders(thresholds_model.t_list[thresholds_model.selectedColor])
        }

    }

    QtObject{
        id: thresholds_model
        property int selectedColor: 0
        property list<HSVThresholds> t_list: [HSVThresholds{}, HSVThresholds{}, HSVThresholds{}]
        property HSVThresholds currentThresholds: t_list[selectedColor]
    }

    menuBar: MenuBar {
        Menu {
            title: "&File"
            MenuItem {
                text: "&Open..."
                onTriggered: loadDialog.open()
            }
            MenuItem{
                text: "&Save"                
                onTriggered: saveDialog.open()
            }
        }
    }

    FilteredCamera{
        id: filter
        thresholds: thresholds_model.currentThresholds
        onThresholdsChanged: root.update_sliders(thresholds)      
        
        anchors.left: parent.left
        anchors.right: sliders.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        anchors.margins: Style.margin

    }

    Column{
        id: sliders
        anchors.right: parent.right
        anchors.margins: Style.margin
        spacing: 15
        MySlider{
            id: h_min_slider
            from: 0; to: 180
            value: 0
            onValueChanged: filter.thresholds.h_min = value
        }
        MySlider{
            id: h_max_slider
            from: 0; to: 180
            value: 180
            onValueChanged: filter.thresholds.h_max = value

        }
         MySlider{
            id: s_min_slider
            from: 0; to: 255
            value: 0
            onValueChanged: filter.thresholds.s_min = value
        }
        MySlider{      
            id: s_max_slider   
            from: 0; to: 255
            value: 255
            onValueChanged: filter.thresholds.s_max = value
        } MySlider{    
            id: v_min_slider 
            from: 0; to: 255
            value: 0
            onValueChanged: filter.thresholds.v_min = value
        }
        MySlider{
            id: v_max_slider
            from: 0; to: 255
            value: 255
            onValueChanged: filter.thresholds.v_max = value
        }

        ComboBox {
            id: colorBox
            anchors.right: v_max_slider.right
            currentIndex: thresholds_model.selectedColor
            width: 200
            model: ["blue", "green", "orange"]
            onCurrentIndexChanged: thresholds_model.selectedColor = currentIndex
            background: Rectangle{
                color: colorBox.currentText
                border.width: 1
            }
            delegate: ItemDelegate {
                text: colorBox.model[index]
                implicitWidth: colorBox.width
                implicitHeight: colorBox.height  
                background: Rectangle{
                    color: colorBox.model[index]
                    border.width: 1
                }
            }

        }
    }

    function update_sliders( t){
        h_min_slider.value = t.h_min            
        h_max_slider.value = t.h_max            
        s_min_slider.value = t.s_min       
        s_max_slider.value = t.s_max            
        v_min_slider.value = t.v_min            
        v_max_slider.value = t.v_max   
    }
}