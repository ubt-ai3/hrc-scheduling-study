import QtQuick
import QtQuick.Controls

import QtQuick.Controls.Universal

import Components

Item{
	id: root

	ScrollView {
		anchors.fill: parent
		contentHeight: currQuestion.height  + 3 * Style.margin
		contentWidth: width
		Pane{
			anchors.top:parent.top
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.topMargin: Style.margin

			leftPadding: Style.margin
			rightPadding: Style.margin
			bottomPadding:  Style.margin

			Universal.background: "white"
			Universal.foreground: "black"
			Loader{
				id: currQuestion
				source: "TLXRatings.qml"
			}
		}
	}


	ListModel {
		id: comparisions
		ListElement{
			first: "Effort"
			second: "Performance"
			selected: ""
		}
		ListElement{
			first: "Temporal Demand"
			second: "Frustration"
			selected: ""
		}
		ListElement{
			first: "Temporal Demand"
			second: "Effort"
			selected: ""
		}
		ListElement{
			first: "Physical Demand"
			second: "Frustration"
			selected: ""
		}
		ListElement{
			first: "Performance"
			second: "Frustration"
			selected: ""
		}
		ListElement{
			first: "Physical Demand"
			second: "Temporal Demand"
			selected: ""
		}
		ListElement{
			first: "Physical Demand"
			second: "Performance"
			selected: ""
		}
		ListElement{
			first: "Temporal Demand"
			second: "Mental Demand"
			selected: ""
		}
		ListElement{
			first: "Frustration"
			second: "Effort"
			selected: ""
		}
		ListElement{
			first: "Performance"
			second: "Mental Demand"
			selected: ""
		}
		ListElement{
			first: "Performance"
			second: "Temporal Demand"
			selected: ""
		}
		ListElement{
			first: "Mental Demand"
			second: "Effort"
			selected: ""
		}
		ListElement{
			first: "Mental Demand"
			second: "Physical Demand"
			selected: ""
		}
		ListElement{
			first: "Effort"
			second: "Physical Demand"
			selected: ""
		}
		ListElement{
			first: "Frustration"
			second: "Mental Demand"
			selected: ""
		}
	}
	ListModel{
		id: questionaire
		ListElement{
			label: "Mental Demand"
			description: "How much mental and perceptual activity was required (e.g. thinking, deciding, calculating, remembering, looking, searching, etc)? Was the task easy or demanding, simple or complex, exacting or forgiving?"
			selected: -1
		}
		ListElement{
			label: "Physical Demand"
			description: "How much physical activity was required (e.g. pushing, pulling, turning, controlling, activating, etc)? Was the task easy or demanding, slow or brisk, slack or strenuous, restful or laborious?"
			selected: -1
		}
		ListElement{
			label: "Temporal Demand"
			description: "How much time pressure did you feel due to the rate of pace at which the tasks or task elements occurred? Was the pace slow and leisurely or rapid and frantic?"
			selected: -1
		}
		ListElement{
			label: "Performance"
			description: "How successful do you think you were in accomplishing the goals of the task set by the experimenter (or yourself)? How satisfied were you with your performance in accomplishing these goals?"
			selected: -1
		}
		ListElement{
			label: "Effort"
			description: "How hard did you have to work (mentally and physically) to accomplish your level of performance?"
			selected: -1
		}
		ListElement{
			label: "Frustration"
			description: "How insecure, discouraged, irritated, stressed and annoyed versus secure, gratified, content, relaxed and complacent did you feel during the task?"
			selected: -1
		}
	}
}