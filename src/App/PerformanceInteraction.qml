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
						message.type = "Performance and Interaction Quality results";
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
			description: "1. The human-robot team worked fluently together"
			selected: -1
		}
		ListElement{
			description: "2. The robot was unintelligent"
			selected: -1
		}
		ListElement{
			description: "3. The robot and I were working towards the same goal"
			selected: -1
		}
		ListElement{
			description: "4. The robot was uncooperative"
			selected: -1
		}
		ListElement{
			description: "5. The robot contributed to the fluency of the collaboration"
			selected: -1
		}
		ListElement{
			description: "6. The human was the most important member of the team"
			selected: -1
		}
		ListElement{
			description: "7. The robot was trustworthy"
			selected: -1
		}
		ListElement{
			description: "8. The robot was committed to the success of the team"
			selected: -1
		}
		ListElement{
			description: "9. The robot had an important contribution to the success of the team"
			selected: -1
		}
		ListElement{
			description: "10. I needed to adapt my movements to the robots movements"
			selected: -1
		}
		ListElement{
			description: "11. The robot reacted flexible to changes in task execution"
			selected: -1
		}
		ListElement{
			description: "12. I had the feeling that the robot is a team partner"
			selected: -1
		}
		ListElement{
			description: "13. During the whole process I always knew what I was requested to do"
			selected: -1
		}
		ListElement{
			description: "14. During the whole process I always knew what the robot was going to do"
			selected: -1
		}
		ListElement{
			description: "15. Cooperating with the robot enables me to accomplish the task more quickly"
			selected: -1
		}
		ListElement{
			description: "16. Cooperating with the robot increases my productivity"
			selected: -1
		}
	}

}