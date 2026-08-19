import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

import Components

Button{
	width: 300
	height: 120
	font.pointSize: textSize

	checked: selected == text

	background: Rectangle{
		color: checked ? Universal.color(Universal.Cyan) : "white"
		border.width: 1
	}
	Universal.accent: Universal.Emerald
}
