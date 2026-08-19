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
								model: 6
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
						message.type = "ATI results";
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
			description: "1. I like to occupy myself in greater detail with technical systems"
			selected: -1
		}
		ListElement{
			description: "2. I like testing the functions of new technical systems."
			selected: -1
		}
		ListElement{
			description: "3. I predominantly deal with technical systems because I have to"
			selected: -1
		}
		ListElement{
			description: "4. When I have a new technical system in front of me, I try it out intensively"
			selected: -1
		}
		ListElement{
			description: "5. I enjoy spending time becoming acquainted with a new technical system"
			selected: -1
		}
		ListElement{
			description: "6. It is enough for me that a technical system works; I don\u0027t care how or why"
			selected: -1
		}
		ListElement{
			description: "7. I try to understand how a technical system exactly works"
			selected: -1
		}
		ListElement{
			description: "8. It is enough for me to know the basic functions of a technical system"
			selected: -1
		}
		ListElement{
			description: "9. I try to make full use of the capabilities of a technical system"
			selected: -1
		}
	}

}