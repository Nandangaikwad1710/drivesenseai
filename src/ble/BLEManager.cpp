#include "BLEManager.h"
#include "../system/Logger.h"
#include "../system/ErrorManager.h"
#include "../config/AppConfig.h"
#include <string.h>

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

class DriveSenseServerCallbacks : public BLEServerCallbacks {
public:
    DriveSenseServerCallbacks(BLEManager* mgr) : m_mgr(mgr) {}

    void onConnect(BLEServer* pServer) override {
        m_mgr->handleClientConnected();
    }

    void onDisconnect(BLEServer* pServer) override {
        m_mgr->handleClientDisconnected();
        // Immediately restart advertising for auto-reconnection
        pServer->startAdvertising();
        Logger::logBLE("Advertising restarted for auto-reconnection");
    }

private:
    BLEManager* m_mgr;
};

class DriveSenseCommandCallbacks : public BLECharacteristicCallbacks {
public:
    DriveSenseCommandCallbacks(BLEManager* mgr) : m_mgr(mgr) {}

    void onWrite(BLECharacteristic* pChar) override {
        String val = pChar->getValue();
        if (val.length() > 0) {
            m_mgr->processIncomingCommand(val.c_str());
        }
    }

private:
    BLEManager* m_mgr;
};
#endif

BLEManager::BLEManager()
    : m_isConnected(false),
      m_state(BLEState::UNINITIALIZED),
      m_cmdCallback(nullptr),
      m_cbContext(nullptr),
      m_bleServer(nullptr),
      m_telemCharacteristic(nullptr),
      m_cmdCharacteristic(nullptr),
      m_statusCharacteristic(nullptr) {}

BLEManager::~BLEManager() {}

bool BLEManager::init(const char* deviceName) {
    Logger::logBLE("Initializing Bluetooth Low Energy Peripheral: %s", deviceName);

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    BLEDevice::init(deviceName);
    BLEDevice::setMTU(BLE_PREFERRED_MTU);

    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new DriveSenseServerCallbacks(this));
    m_bleServer = pServer;

    // Create Main Telemetry Service
    BLEService* pService = pServer->createService(BLE_SERVICE_UUID_MAIN);

    // 1. Telemetry Notify Characteristic
    BLECharacteristic* pTelemChar = pService->createCharacteristic(
        BLE_CHAR_UUID_TELEM,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTelemChar->addDescriptor(new BLE2902());
    m_telemCharacteristic = pTelemChar;

    // 2. Command Write Characteristic
    BLECharacteristic* pCmdChar = pService->createCharacteristic(
        BLE_CHAR_UUID_CMD,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR
    );
    pCmdChar->setCallbacks(new DriveSenseCommandCallbacks(this));
    m_cmdCharacteristic = pCmdChar;

    // 3. Status Notify Characteristic
    BLECharacteristic* pStatusChar = pService->createCharacteristic(
        BLE_CHAR_UUID_STATUS,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusChar->addDescriptor(new BLE2902());
    m_statusCharacteristic = pStatusChar;

    pService->start();

    // Start Advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID_MAIN);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Functions well with iOS
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    m_state = BLEState::ADVERTISING;
    Logger::logBLE("BLE Advertising active, waiting for DriveSense Mobile App...");
    return true;
#else
    m_state = BLEState::ADVERTISING;
    Logger::logBLE("Simulated BLE stack initialized");
    return true;
#endif
}

void BLEManager::setCommandCallback(CommandReceivedCallback cb, void* context) {
    m_cmdCallback = cb;
    m_cbContext = context;
}

void BLEManager::handleClientConnected() {
    m_isConnected = true;
    m_state = BLEState::CONNECTED;
    Logger::logBLE("Mobile Application Connected");
}

void BLEManager::handleClientDisconnected() {
    m_isConnected = false;
    m_state = BLEState::DISCONNECTED;
    Logger::logBLE("Mobile Application Disconnected");
    ErrorManager::recordError(ErrorCode::BLE_DISCONNECTED);
}

bool BLEManager::sendTelemetryNotification(const char* jsonPayload, size_t length) {
    if (!m_isConnected || !jsonPayload || length == 0) {
        return false;
    }

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    if (!m_telemCharacteristic) return false;
    BLECharacteristic* pChar = static_cast<BLECharacteristic*>(m_telemCharacteristic);
    pChar->setValue((uint8_t*)jsonPayload, length);
    pChar->notify();
    return true;
#else
    return true;
#endif
}

bool BLEManager::sendStatusNotification(const char* jsonPayload, size_t length) {
    if (!m_isConnected || !jsonPayload || length == 0) {
        return false;
    }

#if defined(ARDUINO) && !defined(DRIVESENSE_TEST_SUITE)
    if (!m_statusCharacteristic) return false;
    BLECharacteristic* pChar = static_cast<BLECharacteristic*>(m_statusCharacteristic);
    pChar->setValue((uint8_t*)jsonPayload, length);
    pChar->notify();
    return true;
#else
    return true;
#endif
}

void BLEManager::processIncomingCommand(const char* payload) {
    if (!payload) return;
    Logger::logBLE("Command received: %s", payload);

    CommandType cmdType = CommandType::UNKNOWN;

    if (strstr(payload, BLECmd::START_TELEMETRY)) {
        cmdType = CommandType::START_TELEMETRY;
    } else if (strstr(payload, BLECmd::STOP_TELEMETRY)) {
        cmdType = CommandType::STOP_TELEMETRY;
    } else if (strstr(payload, BLECmd::GET_DEVICE_INFO)) {
        cmdType = CommandType::GET_DEVICE_INFO;
    } else if (strstr(payload, BLECmd::GET_SUPPORTED_PIDS)) {
        cmdType = CommandType::GET_SUPPORTED_PIDS;
    } else if (strstr(payload, BLECmd::GET_CONNECTION_STATUS)) {
        cmdType = CommandType::GET_CONNECTION_STATUS;
    } else if (strstr(payload, BLECmd::SET_SIMULATION_MODE)) {
        cmdType = CommandType::SET_SIMULATION_MODE;
    }

    if (m_cmdCallback) {
        m_cmdCallback(cmdType, payload, m_cbContext);
    }
}
