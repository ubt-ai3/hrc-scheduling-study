import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtQuick.Controls.Universal

import Components
import Core

Item{
	id: root
	width: childrenRect.width
	height: childrenRect.height
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
				anchors.margins: Style.margin
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
			}
		}
	}
	Item{
		id: weights
		anchors.margins: Style.margin
		anchors.verticalCenter: descriptions.verticalCenter
		anchors.left: descriptions.right
		height: childrenRect.height
		width: childrenRect.width
		
		Text {
			id: instructionText
			text: "Click on the factor that represents the more important contributor to workload for the task"
			width: 300
			height: 100
			wrapMode: Text.WordWrap
			horizontalAlignment: Text.AlignHCenter
			font.pointSize: 14
			anchors.left: parent.left
			anchors.leftMargin: 50
			
		}
		Row{
			id: row
			anchors.top: instructionText.bottom
			Button{
				width: 40
				height: list.childrenRect.height
				text: "arrow_back_ios"

				enabled: list.currentIndex != 0
				onClicked: {list.currentIndex = list.currentIndex - 1}
				anchors.verticalCenter: parent.verticalCenter

				font.family: Style.iconFont.family
				font.pixelSize: textSize

				background: Rectangle{
					anchors.fill: parent
					color: "white"
				}
			}
			ListView{
				id: list
				width: 350
				height: 300
				model: shuffle(comparisions)
				clip: true
				orientation: ListView.Horizontal
				highlightRangeMode: ListView.StrictlyEnforceRange 
				delegate: Item{
					width: 350
					height: 300
					TLXButton{
						text: first
						Layout.alignment: Qt.AlignHCenter 
						anchors.horizontalCenter: or.horizontalCenter
						anchors.bottom: or.top
						onClicked: {
							selected = text; 
							if(! Style.formValidationDisabled)
								confirmButton.enabled = !window.hasDefaultValue(list.model, "")	
						}

					}

					Label{
						id: or
						text: qsTr("or")
						font.pointSize: textSize
						anchors.centerIn: parent
					}

					TLXButton{
						text: second
						anchors.horizontalCenter: or.horizontalCenter
						anchors.top: or.bottom
						onClicked: {
							selected = text; 
							if(! Style.formValidationDisabled)
								confirmButton.enabled = !window.hasDefaultValue(list.model, "")	
						}

					}
				}
				displaced: Transition {
			        NumberAnimation { properties: "x"; duration: 1}
			    }
			}
			Button{
				width: 40
				height: list.childrenRect.height
				text: "arrow_forward_ios"
				
				enabled: list.currentIndex != list.count -1
				onClicked: {list.currentIndex = list.currentIndex + 1}
				anchors.verticalCenter: parent.verticalCenter

				font.family: Style.iconFont.family
				font.pixelSize: textSize

				background: Rectangle{
					anchors.fill: parent
					color: "white"
				}
			}
			
		}
		PageIndicator {
			anchors.top: row.bottom
			anchors.horizontalCenter: row.horizontalCenter 
			currentIndex: list.currentIndex
			count: list.count
		}
	}
	ButtonDefault{
		id: confirmButton
		font.pointSize: textSize
		defaultText: "Confirm"
		width: 150
		anchors.right: weights.right
		anchors.bottom: descriptions.bottom
		anchors.margins: 0
		enabled: Style.formValidationDisabled

		onClicked: {
			let m = new Object();
			m.type = "NASA TLX weights"
			m.weights = [];
			for(let i = 0; i< list.model.count; i++){
				m.weights.push(list.model.get(i))
			}
			Logger.message(JSON.stringify(m))
			pageStack.pop()
		}
	}

	function shuffle(model){
	    let currentIndex = model.count, temporaryValue, randomIndex;
	
	    // While there remain elements to shuffle...
	    while (0 !== currentIndex) {
	        // Pick a remaining element...
	        randomIndex = Math.floor(Math.random() * currentIndex)
	        currentIndex -= 1
	        // And swap it with the current element.
	        // the dictionaries maintain their reference so a copy should be made
	        temporaryValue = JSON.parse(JSON.stringify(model.get(currentIndex)))
	        model.set(currentIndex, model.get(randomIndex))
	        model.set(randomIndex, temporaryValue);
	    }
	    return model;
	}
}