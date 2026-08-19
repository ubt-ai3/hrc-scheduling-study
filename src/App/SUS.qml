import QtQuick
import QtQuick.Controls

import QtQuick.Controls.Universal

import Components
import Core


Item{
	id: root

	property int textSize: 15
	property int headerSize: textSize + 5
	ScrollView {
		anchors.fill: parent
		contentHeight: pane.height
		contentWidth: width
		Pane{
			id: pane
			anchors.top:parent.top
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.topMargin: Style.margin

			leftPadding: Style.margin
			rightPadding: Style.margin
			bottomPadding:  Style.margin

			Universal.background: "white"
			Universal.foreground: "black"

			contentHeight: descriptions.height + minLabel.height+ Style.margin
			contentWidth: descriptions.width


			Label{
				id: minLabel
				text: "Strongly\nagree"
				font.pointSize: textSize
				anchors.right: descriptions.right			
			}
			Label{
				text: "Strongly\ndisagree"
				anchors.top: minLabel.top
				anchors.left: descriptions.left
				anchors.leftMargin: 600
				font.pointSize: textSize
			}

			Column{
				id: descriptions
				spacing: 30
				anchors.top: minLabel.bottom
				anchors.topMargin: Style.margin
				Repeater{
					id: sliders
					model: questionaire
					delegate:
					Item{
						height: childrenRect.height
						width: childrenRect.width
						Label{
							id: desc
							text: description
							font.pointSize: textSize
							wrapMode: Text.WordWrap
							width: 500
						}
						Row{
							anchors.left: desc.right
							anchors.top: desc.top
							anchors.leftMargin: 100
							spacing: -1
							Repeater{
								model: 5
								delegate: Item {
									width: childrenRect.width
									height: childrenRect.height
									TLXButton{
										id: b;  width: 60; height: 30
										checked: selected == index + 1
										onClicked: {
											selected = index + 1
											if(! Style.formValidationDisabled)
												confirmButton.enabled = !window.hasDefaultValue(questionaire, -1)	
										}
									}
									Label{ 
										id: l; anchors.top:b.bottom; anchors.horizontalCenter: b.horizontalCenter
										text: index + 1
										font.pointSize: textSize - 2
									}
								}
							}
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
						let questions = [];
						for(let i = 0; i< questionaire.count; i++){
							let entry = JSON.parse(JSON.stringify(questionaire.get(i)));
							entry.index = i +1
							questions.push(entry)
						}
						let message = new Object();
						message.type = "SUS results";
						message.value = questions;
						Logger.message(JSON.stringify(message))
						pageStack.pop()
					}
				}
			}
		}
	}
	ListModel{
		id: questionaire
		ListElement{
			description: "1. I think that I would like to use this system frequently"
			selected: -1
		}
		ListElement{
			description: "2. I found the system unnecessarily complex"
			selected: -1
		}
		ListElement{
			description: "3. I thought the system was easy to use"
			selected: -1
		}
		ListElement{
			description: "4. I think that I would need the support of a technical person to be able to use this system"
			selected: -1
		}
		ListElement{
			description: "5. I found the various functions in this system were well integrated"
			selected: -1
		}
		ListElement{
			description: "6. I thought there was too much inconsistency in this system"
			selected: -1
		}
		ListElement{
			description: "7. I would imagine that most people would learn to use this system very quickly"
			selected: -1
		}
		ListElement{
			description: "8. I found the system very cumbersome to use"
			selected: -1
		}
		ListElement{
			description: "9. I felt very confident using the system"
			selected: -1
		}
		ListElement{
			description: "10. I needed to learn a lot of things before I could get going with this system"
			selected: -1
		}
	}

}