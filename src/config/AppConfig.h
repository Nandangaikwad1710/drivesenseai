#pragma once

#include <stdint.h>

/**
 * @file AppConfig.h
 * @brief Application, protocol, and timing configuration for DriveSense OBD.
 */

// ==========================================
// Device Identity & Versioning
// ==========================================
#define DEVICE_NAME                 "DriveSense_OBD"
#define FIRMWARE_VERSION            "1.0.0"
#define HARDWARE_VERSION            "1.0"
#define PROTOCOL_VERSION            "1.0"

// ==========================================
// Operational Modes
// ==========================================
// When true, simulates realistic ECU responses without requiring a physical CAN bus.
// This allows mobile app development and BLE testing anywhere.
#ifndef SIMULATION_MODE
#define SIMULATION_MODE             true
#endif

// Strictly enforce Read-Only OBD diagnostics to guarantee vehicle safety
#define OBD_READ_ONLY_MODE          true

// ==========================================
// Polling Engine Rates (Milliseconds)
// ==========================================
// High Priority: Fast-changing dynamic values (Speed, RPM, Throttle)
#define POLLING_INTERVAL_HIGH_MS    100

// Medium Priority: Moderate-changing values (Engine Load, MAF)
#define POLLING_INTERVAL_MEDIUM_MS  500

// Low Priority: Slowly-changing thermal/reservoir values (Coolant Temp, IAT, Fuel Level)
#define POLLING_INTERVAL_LOW_MS     5000

// Main telemetry stream broadcast interval to BLE client
#define TELEMETRY_STREAM_INTERVAL_MS 200

// ==========================================
// OBD-II & CAN Communication Timeouts
// ==========================================
#define OBD_RESPONSE_TIMEOUT_MS     1000
#define OBD_MAX_RETRIES             2
#define OBD_UNSUPPORTED_RETRY_DELAY_MS 30000 // Retry previously failed PIDs only after 30s
#define OBD_CAN_BROADCAST_ID        0x7DF    // Standard 11-bit functional query ID
#define OBD_ECU_PRIMARY_RESPONSE_ID 0x7E8    // Standard 11-bit primary ECU reply ID
#define OBD_ECU_RESPONSE_MASK       0x7F8    // Mask to match 0x7E8 - 0x7EF

// ==========================================
// Telemetry Buffer Sizing
// ==========================================
// Fixed-size circular buffer to prevent unbounded memory allocation
#define TELEMETRY_BUFFER_CAPACITY   32

// FreeRTOS task queue capacities
#define TELEMETRY_QUEUE_SIZE        16
#define BLE_NOTIFY_QUEUE_SIZE       16
#define BLE_CMD_QUEUE_SIZE          8

// ==========================================
// Bluetooth Low Energy (BLE) GATT UUIDs
// ==========================================
// DriveSense Service: 9b20b22a-5a91-4c60-84a1-0e1d52033000
#define DRIVESENSE_SERVICE_UUID     "9b20b22a-5a91-4c60-84a1-0e1d52033000"

// Telemetry Notify Characteristic: 9b20b22a-5a91-4c60-84a1-0e1d52033001
#define DRIVESENSE_CHAR_TELEM_UUID  "9b20b22a-5a91-4c60-84a1-0e1d52033001"

// Command Write Characteristic: 9b20b22a-5a91-4c60-84a1-0e1d52033002
#define DRIVESENSE_CHAR_CMD_UUID    "9b20b22a-5a91-4c60-84a1-0e1d52033002"

// Status Notify Characteristic: 9b20b22a-5a91-4c60-84a1-0e1d52033003
#define DRIVESENSE_CHAR_STATUS_UUID "9b20b22a-5a91-4c60-84a1-0e1d52033003"

// BLE MTU Negotiation Target (Standard default is 23; max ESP32 is 517)
#define BLE_PREFERRED_MTU           512

// ==========================================
// System Health & Watchdog
// ==========================================
#define WATCHDOG_TIMEOUT_SECONDS    10
#define SYSTEM_HEALTH_LOG_INTERVAL_MS 15000
#define SERIAL_BAUD_RATE            115200
