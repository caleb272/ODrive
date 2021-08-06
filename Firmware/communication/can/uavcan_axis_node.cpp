#include "uavcan_axis_node.hpp"

#include <axis.hpp>
#include <odrive_main.h>

UavcanAxisNode::UavcanAxisNode(
	uavcan::NodeID id,
	const std::string& nodeName,
	uavcan_stm32::CanInitHelper<RxQueueSize> * can,
	uavcan::ISystemClock* uavcanSysClock
) : nodeId(static_cast<int>(id.get())), node(can->driver, *uavcanSysClock) {
	node.setNodeID(id);
	node.setName(nodeName.c_str());

	positionCommandSub = new uavcan::Subscriber<uavcan::equipment::actuator::ArrayCommand, positionCommandBinder>(node);
}

UavcanAxisNode::~UavcanAxisNode() {
	delete positionCommandSub;
}

void UavcanAxisNode::start() {
	node.start();
	positionCommandSub->start(positionCommandBinder(this, &UavcanAxisNode::handlePositionCommand));

	node.setHealthOk();
	node.setModeOperational();
}

void UavcanAxisNode::spin() {
	// TODO Caleb you need to handle the errors here.
	node.spinOnce();
}

void UavcanAxisNode::handlePositionCommand(const uavcan::ReceivedDataStructure<uavcan::equipment::actuator::ArrayCommand>& commandArray) const {
	for (int commandIndex = 0; commandIndex < commandArray.commands.size(); ++commandIndex) {
		if (commandArray.commands[commandIndex].actuator_id != nodeId) continue;

		uavcan::equipment::actuator::Command command = commandArray.commands[commandIndex];
		Axis* axis = nullptr;

		for (int axisIndex = 0; axisIndex < AXIS_COUNT; ++axisIndex) {
			if (odrv.get_axis(axisIndex).config_.can.node_id != command.actuator_id)
				continue;
			axis = &odrv.get_axis(axisIndex);
			break;
		}
		
		if (axis == nullptr) { /* TODO Caleb ~ Handle this error */ return; }

		// axis.controller_.input_pos_ = can_getSignal<float>(msg, 0, 32, true);
		// axis.controller_.input_vel_ = can_getSignal<int16_t>(msg, 32, 16, true, 0.001f, 0);
		// axis.controller_.input_torque_ = can_getSignal<int16_t>(msg, 48, 16, true, 0.001f, 0);

		axis->controller_.input_pos_ = command.command_value;
		axis->controller_.input_pos_updated();
	}
}