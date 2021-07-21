#include "uavcan_manager.hpp"

uavcan_stm32::CanInitHelper<UavcanManager::RxQueueSize> UavcanManager::can;

bool UavcanManager::init() {
	if (!isInited) {
		int res = can.init(UavcanManager::BitRate);
		if (res < 0)
			while (true) {}
		isInited = true;
	}

	uavcan_stm32::SystemClock& sysClk = uavcan_stm32::SystemClock::instance();
	node = new uavcan::Node<0>(can.driver, sysClk, uavcanNodeAllocator);
	node->setNodeID(10);    
	node->setName("org.arkrobotics");
	
	uavcan::protocol::SoftwareVersion sw_version;  // Standard type uavcan.protocol.SoftwareVersion
	sw_version.major = 1;
	node->setSoftwareVersion(sw_version);
	uavcan::protocol::HardwareVersion hw_version;  // Standard type uavcan.protocol.HardwareVersion
	hw_version.major = 1;
	node->setHardwareVersion(hw_version);

	kvPub = new uavcan::Publisher<uavcan::protocol::debug::KeyValue>(*node);
	kvPub->init();
	kvPub->setTxTimeout(uavcan::MonotonicDuration::fromMSec(1000));
	kvPub->setPriority(uavcan::TransferPriority::MiddleLower);

	if (node->start() < 0) { /* TODO Caleb handle this error properly */ }

	node->setModeOperational();

	return true;
}
void UavcanManager::start() {
	auto wrapper = [](void* ctx) {
	    ((UavcanManager*)ctx)->uavcan_server_thread();
	};

	osThreadDef(can_server_thread_def, wrapper, osPriorityNormal, 0, stack_size_ / sizeof(StackType_t));
 	threadId = osThreadCreate(osThread(can_server_thread_def), this);
	
    	node->setHealthOk();
	isStarted = true;
}

void UavcanManager::uavcan_server_thread() {
    // Start back here figure out what is needed to properly send the heartbeat and make sure the timings are correct for the baud rate.
    while (true) {
        if (node->spinOnce() < 0)
            while (true) {}

        uavcan::protocol::debug::KeyValue message;
        message.value = 100;
        message.key = "caleb";
    
        if (kvPub->broadcast(message) < 0) {
            while (true) {}
        }
    }
}