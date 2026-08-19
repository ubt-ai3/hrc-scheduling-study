import QtQuick
import QtQuick.Controls

import QtQuick.Controls.Universal

import Components
import Core


Item{
	id: root
	property int indent: 50
	ScrollView {
		anchors.fill: parent
		contentHeight: pane.height + Style.margin
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

			contentHeight: form.height
			contentWidth: form.width

			
			font.pointSize: textSize

			Column{
				id: form
				spacing: 30
				anchors.topMargin: Style.margin
				rightPadding: indent
				Column{
					ButtonGroup{
						id: gender
					}
					Label{
						text: "What is your current gender?"
						width: 500
						font.pointSize: headerSize
					}
					Row{
						x: indent
						Repeater{
							delegate: RadioButton { 
								text: modelData
								ButtonGroup.group: gender
							}
							model: ["male", "female",  "divers"]
						}	
					}
				}
				Column{
					Label{
						font.pointSize: headerSize
						text: "How old are you?"
						width: 500
					}
					TextField {
						id: age
						x: 50
						validator: IntValidator{bottom: 0; top: 200;}
						font.pointSize: textSize
						width: 50
					}
				}
				Column{
					Label{
						font.pointSize: headerSize
						text: "Please indicate your current highest level of education"
					}
					ButtonGroup{
						id: completedEducation
					}
					Repeater{
						model: educationLevels
						delegate: RadioButton{
							x: indent
							text: name
							ButtonGroup.group: completedEducation
						}
					}
				}
				Column{
					Label{
						font.pointSize: headerSize
						text: "Are you currently pursuing a higher level of education"
					}
					ButtonGroup{
						id: currentEducation
					}
					Repeater{
						model: educationLevels
						delegate: RadioButton{
							x: indent
							text: name
							ButtonGroup.group: currentEducation
						}
					}
				}
				Column{
					Label{
						font.pointSize: headerSize
						text: "What are your fields of education/ profession?"
					}
					ButtonGroup{
						id: fields
						exclusive: false
					}
					Repeater{
						model: fieldsOfEducation
						delegate: CheckBox{
							x: indent
							text: name
							ButtonGroup.group: fields
						}
					}
				}
				Column{
					Label{
						font.pointSize: headerSize
						text: "How much prior experience do you have working with robots?"
					}
					ButtonGroup{
						id: robotExperience
					}
					Row{
						x: indent
						Repeater{
							model: ["none", "little", "some", "a lot"]
							delegate: RadioButton{
								x: indent
								text: modelData
								ButtonGroup.group: robotExperience
							}
						}
					}
					TextArea{
						id: robotExperienceComment
						x: indent
						placeholderText: "Please elaborate on your robot experience, if applicable."
						height: 70
						width: 600
					}

				}
				Column{
					Label{
						font.pointSize: headerSize
						text: "Do you have any further comments?"
					}
					TextArea{
						id: comments
						x: indent
						height: 200
						width: 600
					}
				}


				ButtonDefault{
					id: confirmButton
					anchors.right: parent.right
					anchors.margins: 0
					font.pointSize: textSize
					defaultText: "Confirm"
					width: 150
					enabled: true
					onClicked: {
						let questions = new Object();
						
						questions.gender = gatherSelections(gender.buttons)
						questions.age = age.acceptableInput ? parseInt(age.text) : -1
						questions.completedEducation = gatherSelections(completedEducation.buttons)
						questions.currentEducation = gatherSelections(currentEducation.buttons)
						questions.fields = gatherSelections(fields.buttons)
						questions.robotExperience = gatherSelections(robotExperience.buttons)
						questions.robotExperienceComment = robotExperienceComment.text
						questions.comments = comments.text
						let message = new Object();
						message.type = "Demographics results";
						message.value = questions;
						Logger.message(JSON.stringify(message))
						pageStack.pop(null)
					}
					function gatherSelections( buttonList){
						let selected = []
						for (var i = 0; i < buttonList.length; i++) {
							let b = buttonList[i]
							if (b.checked) selected.push(b.text)
						}						
						return selected
					}
				}				
			}
		}
	}

	ListModel{
		id: fieldsOfEducation
		ListElement{ name: "Generic programmes and qualifications" }
		ListElement{ name: "Education" }
		ListElement{ name: "Arts and humanities" }
		ListElement{ name: "Social sciences, journalism and information" }
		ListElement{ name: "Business, administration and law" }
		ListElement{ name: "Natural sciences, mathematics and statistics" }
		ListElement{ name: "Information and Communication Technologies" }
		ListElement{ name: "Engineering, manufacturing and construction" }
		ListElement{ name: "Agriculture, forestry, fisheries and veterinary" }
		ListElement{ name: "Health and welfare" }
		ListElement{ name: "Services" }
	}

	ListModel{
		id: educationLevels
		ListElement{ name: "Early childhood education"}
		ListElement{ name: "Primary education"}
		ListElement{ name: "Lower secondary education"}
		ListElement{ name: "Upper secondary education"}
		ListElement{ name: "Post-secondary non-tertiary education"}
		ListElement{ name: "Short-cycle tertiary education"}
		ListElement{ name: "Bachelor's or equivalent level"}
		ListElement{ name: "Master's or equivalent level"}
		ListElement{ name: "Doctoral or equivalent level"}
		ListElement{ name: "Not elsewhere classified"}
	}	
}








































