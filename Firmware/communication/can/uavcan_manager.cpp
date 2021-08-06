#include "uavcan_manager.hpp"
#include "freertos_vars.h"

#include <board.h>
#include <odrive_main.h>

uavcan_stm32::CanInitHelper<UavcanManager::RxQueueSize> UavcanManager::can;

UavcanManager::~UavcanManager() {
	for (auto i : nodes)
		delete i;
}

bool UavcanManager::init() {
	if (!isInited) {
		int res = can.init(UavcanManager::BitRate);
		if (res < 0)
			while (true) {}
		isInited = true;
	}

	uavcan_stm32::SystemClock& sysClk = uavcan_stm32::SystemClock::instance();

	for (int i = 0; i < AXIS_COUNT; ++i)
		nodes[i] = new UavcanAxisNode(odrv.get_axis(i).config_.can.node_id, "org.arkrobotics.odrive.axis" + i, &can, &sysClk); 
		
	return true;
}

void UavcanManager::start() {
	auto wrapper = [](void* ctx) {
	    ((UavcanManager*)ctx)->uavcan_server_thread();
	};

	osThreadDef(can_server_thread_def, wrapper, osPriorityNormal, 0, stack_size_ / sizeof(StackType_t));
 	threadId = osThreadCreate(osThread(can_server_thread_def), this);

	 for (int i = 0; i < AXIS_COUNT; ++i)
		 nodes[i]->start();
	
	isStarted = true;
}

void UavcanManager::uavcan_server_thread() {
	while (true) {
		for (int i = 0; i < AXIS_COUNT; ++i)
			nodes[i]->spin();

          osSemaphoreWait(sem_can, 10UL);
        }
}
