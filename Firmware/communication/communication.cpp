
/* Includes ------------------------------------------------------------------*/

#include "communication.h"

#include "interface_usb.h"
#include "interface_uart.h"
// #include "interface_can.hpp"
#include "interface_i2c.h"

#include "odrive_main.h"
#include "freertos_vars.h"
#include "utils.hpp"

#include <cmsis_os.h>
#include <memory>
//#include <usbd_cdc_if.h>
//#include <usb_device.h>
//#include <usart.h>
#include <gpio.h>

#include <type_traits>

#include <uavcan/uavcan.hpp>
#include <uavcan/node/node.hpp>
#include <uavcan/protocol/debug/KeyValue.hpp>
#include <uavcan_stm32/uavcan_stm32.hpp>

/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Global constant data ------------------------------------------------------*/
/* Global variables ----------------------------------------------------------*/

uint64_t serial_number;
char serial_number_str[13]; // 12 digits + null termination

static constexpr int RxQueueSize = 64;
static constexpr std::uint32_t BitRate = 500000;

/* Private constant data -----------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Function implementations --------------------------------------------------*/

void init_communication(void) {
    //printf("hi!\r\n");

    // Dual UART operation not supported yet
    if (odrv.config_.enable_uart_a && odrv.config_.enable_uart_b) {
        odrv.misconfigured_ = true;
    }

    if (odrv.config_.enable_uart_a && uart_a) {
        start_uart_server(uart_a);
    } else if (odrv.config_.enable_uart_b && uart_b) {
        start_uart_server(uart_b);
    }

    start_usb_server();

    if (odrv.config_.enable_i2c_a) {
        start_i2c_server();
    }

    // TODO Caleb - Setup the can driver here.
    static uavcan_stm32::CanInitHelper<RxQueueSize> can;
    can.init(BitRate);

    uavcan_stm32::SystemClock& sysClk = uavcan_stm32::SystemClock::instance();
    uavcan::Node<0>* node = odrv.canNode = new uavcan::Node<0>(can.driver, sysClk, odrv.uavcanNodeAllocator);

    node->setNodeID(10);    
    node->setName("org.arkrobotics");
    
    uavcan::protocol::SoftwareVersion sw_version;  // Standard type uavcan.protocol.SoftwareVersion
    sw_version.major = 1;
    node->setSoftwareVersion(sw_version);

    uavcan::protocol::HardwareVersion hw_version;  // Standard type uavcan.protocol.HardwareVersion
    hw_version.major = 1;
    node->setHardwareVersion(hw_version);

    uavcan::Publisher<uavcan::protocol::debug::KeyValue> kvPub(*node);
    kvPub.init();

    kvPub.setTxTimeout(uavcan::MonotonicDuration::fromMSec(1000));
    kvPub.setPriority(uavcan::TransferPriority::MiddleLower);

    if (node->start() < 0) { /* TODO Caleb handle this error properly */ }
    node->setModeOperational();

    node->getLogger().setLevel(uavcan::protocol::debug::LogLevel::DEBUG);

    // Start back here figure out what is needed to properly send the heartbeat and make sure the timings are correct for the baud rate.
    while (true) {
        if (node->spinOnce() < 0)
            while (true) {}
        // node->setHealthOk();

        uavcan::protocol::debug::KeyValue message;
        message.value = 100;
        message.key = "caleb";
    
        if (kvPub.broadcast(message) < 0) {
            while (true) {}
        }
    }
    
    // throw std::runtime_error()
    // node->spinOnce();
    
    // if (odrv.config_.enable_can_a) {
    //     odrv.can_.start_server(&hcan1);
    // }
}

#include <fibre/async_stream.hpp>


extern "C" {
int _write(int file, const char* data, int len) __attribute__((used));
}

// @brief This is what printf calls internally
int _write(int file, const char* data, int len) {
    fibre::cbufptr_t buf{(const uint8_t*)data, (const uint8_t*)data + len};

    if (odrv.config_.uart0_protocol == ODrive::STREAM_PROTOCOL_TYPE_STDOUT ||
        odrv.config_.uart0_protocol == ODrive::STREAM_PROTOCOL_TYPE_ASCII_AND_STDOUT) {
        uart0_stdout_sink.write(buf);
        if (!uart0_stdout_pending) {
            uart0_stdout_pending = true;
            osMessagePut(uart_event_queue, 3, 0);
        }
    }

    if (odrv.config_.usb_cdc_protocol == ODrive::STREAM_PROTOCOL_TYPE_STDOUT ||
        odrv.config_.usb_cdc_protocol == ODrive::STREAM_PROTOCOL_TYPE_ASCII_AND_STDOUT) {
        usb_cdc_stdout_sink.write(buf);
        if (!usb_cdc_stdout_pending) {
            usb_cdc_stdout_pending = true;
            osMessagePut(usb_event_queue, 7, 0);
        }
    }

    return len; // Always pretend that we processed everything
}


#include "../autogen/function_stubs.hpp"

ODrive& ep_root = odrv;
#include "../autogen/endpoints.hpp"
