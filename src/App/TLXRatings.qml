import QtQuick
import QtQuick.Controls

import QtQuick.Controls.Universal

import Components
import Core

Column{
	id: descriptions
	spacing: 10

	Repeater{
		id: sliders
		model: questionaire
		delegate:
		Item{
			height: childrenRect.height
			width: childrenRect.width
			Label{
				id: factor_name
				text: label
				font.pointSize: headerSize
				font.bold: true
			}
			Label{
				id: desc
				text: description
				anchors.left: factor_name.left
				anchors.top: factor_name.bottom

				font.pointSize: textSize
				wrapMode: Text.WordWrap
				width: 500
			}
			TLXSlider{
				anchors.left: desc.right
				anchors.verticalCenter: desc.verticalCenter
				anchors.leftMargin: 100
				pointSize: textSize - 2
				stepSize: 1
				onValueChanged: 
				{
					selected = value
					if(!Style.formValidationDisabled)
						confirmButton.enabled = !window.hasDefaultValue(sliders.model, -1)
				}
				swapLabels: index == 3
			}
		}
	}
	ButtonDefault{
		id: confirmButton
		anchors.right: parent.right
		anchors.margins: 0
		font.pointSize: textSize
		defaultText: "Confirm"
		width: 150
		enabled: Style.formValidationDisabled
		onClicked: {
			let questions = {};
			for(let i = 0; i< questionaire.count; i++){
				let entry = questionaire.get(i);
				questions[entry.label] = entry.selected;
			}
			let message = new Object();
			message.type = "NASA TLX ratings";
			message.value = questions;
			Logger.message(JSON.stringify(message))
			currQuestion.source = "TLXWeights.qml"
		}
	}
}
