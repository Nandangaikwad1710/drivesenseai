#pragma once

#include "CANConfig.h"
#include "CANFrame.h"
#include <stdint.h>

/**
 * @file CANManager.h
 * @brief Hardware and simulation abstraction layer for MCP2515 CAN controller.
 */

class CANManager {
public:
    CANManager();
    ~CANManager();

    bool init(CANSpeed speed = CANSpeed::CAN_500KBPS, uint8_t clockMHz = 8, bool forceSimulation = false);
    bool sendFrame(const CANFrame& frame);
    bool receiveFrame(CANFrame& frame, uint32_t timeoutMs = 100);
    bool checkReceive();

    CANStatus getStatus() const { return m_status; }
    CANSpeed getSpeed() const { return m_speed; }
    bool isSimulation() const { return m_isSimulation; }
    void setSimulation(bool enable) { m_isSimulation = enable; }

private:
    CANStatus m_status;
    CANSpeed m_speed;
    uint8_t m_clockMHz;
    bool m_isSimulation;
    void* m_driver = nullptr; // Pointer to MCP_CAN driver instance if compiled with Arduino
    void* getDriver() const { return m_driver; }
};
