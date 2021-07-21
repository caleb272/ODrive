#ifndef __UAVCAN_MANAGER__
#define __UAVCAN_MANAGER__

#include <uavcan/uavcan.hpp>
#include <uavcan/node/node.hpp>
#include <uavcan/protocol/debug/KeyValue.hpp>

#include <uavcan/driver/can.hpp>
#include <uavcan_stm32/uavcan_stm32.hpp>
#include <uavcan/driver/system_clock.hpp>
#include <uavcan/helpers/heap_based_pool_allocator.hpp>

#include <cmsis_os.h>
#include <board.h>

class UavcanManager {
	public:
		bool init();
		void start();

    		class RaiiSynchronizer {};
	private:
    		osThreadId threadId;
    		const uint32_t stack_size_ = 1024;  // Bytes
    		uavcan::PoolAllocator<UAVCAN_NODE_POOL_BLOCK_SIZE, UAVCAN_NODE_POOL_BLOCK_SIZE, UavcanManager::RaiiSynchronizer> uavcanNodeAllocator;
    		uavcan::Node<0>* node;

		uavcan::Publisher<uavcan::protocol::debug::KeyValue>* kvPub;

		bool isInited = false;
		bool isStarted = false;

                static constexpr int RxQueueSize = 64;
                static constexpr std::uint32_t BitRate = 500000;

		// static uavcan_stm32::SystemClock* sysClk;
		static uavcan_stm32::CanInitHelper<RxQueueSize> can;
    		static uavcan::ICanDriver& uavcanCANDriver; 
    		static uavcan::ISystemClock& uavcanSysClock;

                void uavcan_server_thread();
};

#endif