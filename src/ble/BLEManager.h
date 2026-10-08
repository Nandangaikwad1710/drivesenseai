#pragma once

#include "BLECommands.h"
#include "BLEService.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @file BLEManager.h
 * @brief Bluetooth Low Energy peripheral manager for DriveSense OBD.
 */

enum class BLEState : uint8_t {
    UNINITIALIZED = 0,
    ADVERTISING = 1,
    CONNECTED = 2,
    DISCONNECTED = 3
};

// Callback type for incoming mobile commands
typedef void (*CommandReceivedCallback)(CommandType cmd, const char* rawPayload, void* context);

class BLEManager {
public:
    BLEManager();
    ~BLEManager();

    bool init(const char* deviceName = DEVICE_NAME);
    void setCommandCallback(CommandReceivedCallback cb, void* context = nullptr);

    bool isConnected() const { return m_isConnected; }
    BLEState getState() const { return m_state; }

    bool sendTelemetryNotification(const char* jsonPayload, size_t length);
    bool sendStatusNotification(const char* jsonPayload, size_t length);

    void processIncomingCommand(const char* payload);

    // Static callback hooks for ESP32 BLE callbacks
    void handleClientConnected();
    void handleClientDisconnected();

private:
    bool m_isConnected;
    BLEState m_state;
    CommandReceivedCallback m_cmdCallback;
    void* m_cbContext;

    void* m_bleServer = nullptr;
    void* m_telemCharacteristic = nullptr;
    void* m_cmdCharacteristic = nullptr;
    void* m_statusCharacteristic = nullptr;

    void* getBLEServer() const { return m_bleServer; }
    void* getTelemChar() const { return m_telemCharacteristic; }
    void* getCmdChar() const { return m_cmdCharacteristic; }
    void* getStatusChar() const { return m_statusCharacteristic; }
};
