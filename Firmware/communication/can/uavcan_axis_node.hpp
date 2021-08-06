#ifndef __UAVCAN_AXIS_NODE__
#define __UAVCAN_AXIS_NODE__

#include <uavcan/uavcan.hpp>
#include <uavcan/node/node.hpp>
#include <uavcan/equipment/actuator/ArrayCommand.hpp>
#include <uavcan/equipment/actuator/Command.hpp>
#include <uavcan_stm32/uavcan_stm32.hpp>

#include <board.h>

// #define NODE_MEMORY_ALLOC 16384
#define NODE_MEMORY_ALLOC 2048

class UavcanAxisNode {
	public:
                static constexpr int RxQueueSize = 64;

		UavcanAxisNode(
			uavcan::NodeID id,
			const std::string& nodeName,
			uavcan_stm32::CanInitHelper<RxQueueSize> * can,
	    		uavcan::ISystemClock* uavcanSysClock);
		~UavcanAxisNode();
		void start();
		void spin();
	
	private:
		int nodeId;
		uavcan::Node<NODE_MEMORY_ALLOC> node;

                typedef uavcan::MethodBinder<
			UavcanAxisNode*,
			void (UavcanAxisNode::*)(const uavcan::ReceivedDataStructure<uavcan::equipment::actuator::ArrayCommand>&) const> positionCommandBinder;

		uavcan::Subscriber<uavcan::equipment::actuator::ArrayCommand, positionCommandBinder>* positionCommandSub;

		void handlePositionCommand(const uavcan::ReceivedDataStructure<uavcan::equipment::actuator::ArrayCommand>& commandArray) const;
};

#endif