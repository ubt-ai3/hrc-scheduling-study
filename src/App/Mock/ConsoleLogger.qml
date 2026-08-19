import QtQuick
import QtQuick3D

import App.Enums

Node{
	property var operations
	property var startTimes
	onOperationsChanged: {
		startTimes = ({}) // empty dict

		operations.forEach( op => {
			op.onStateChanged.connect(() => opStatusChanged(op))
		});
	}
	function opStatusChanged(op){
		if(op.state == OperationState.HumanNext ||op.state == OperationState.RobotNext){
			startTimes[op] = new Date().getTime()
		} else if ( op.state == OperationState.Closed){
			console.log("Operation completed in ",  new Date().getTime() - startTimes[op], " ms");
		}
	}
}