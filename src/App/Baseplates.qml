import QtQuick3D

Node{
    property var plates: []

    property var instances: []

    property var debug

    ModelLoader{
        id: loader
    }

    onPlatesChanged: {
        while(instances.length > 0){
            instances.pop().destroy();
        }
        // load Object Models

        plates.forEach( plate =>{
            // create model based on Kind
            var instance = loader.load_baseplate(plate.kind, baseplates);
            
            //store instance
            instance.position = Qt.binding(function() {return loader.to_gui_coordinates(plate.position)});
            instance.eulerRotation.y = Qt.binding(function() { return plate.rotation * (180/Math.PI)});
            instances.push(instance);
        });
    }

}