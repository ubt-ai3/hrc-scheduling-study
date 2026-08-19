
import QtQuick.Controls
import QtQuick.Controls.Universal

import Components

Dialog{
	id: root
	width: 400
    height: 400
	x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    modal: true 
	closePolicy: Popup.CloseOnEscape
    Universal.foreground: "black"
}