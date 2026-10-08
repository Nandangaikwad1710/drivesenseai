#pragma once

#include <stdint.h>

/**
 * @file HardwareConfig.h
 * @brief Centralized hardware pin and peripheral configuration for DriveSense OBD.
 * 
 * Target Board: ESP32 Development Board (e.g. NodeMCU ESP32, ESP-WROOM-32)
 * CAN Controller: Microchip MCP2515 via SPI
 * CAN Transceiver: NXP TJA1050, SN65HVD230, or equivalent 3.3V/5V transceiver
 */

// ==========================================
// MCP2515 SPI Pin Configuration
// ==========================================
#ifndef CAN_CS_PIN
#define CAN_CS_PIN          5    // Chip Select (CS / SS)
#endif

#ifndef CAN_INT_PIN
#define CAN_INT_PIN         4    // Interrupt pin (active LOW)
#endif

#ifndef CAN_SCK_PIN
#define CAN_SCK_PIN         18   // SPI Clock
#endif

#ifndef CAN_MISO_PIN
#define CAN_MISO_PIN        19   // SPI MISO (SO)
#endif

#ifndef CAN_MOSI_PIN
#define CAN_MOSI_PIN        23   // SPI MOSI (SI)
#endif

// Oscillator crystal frequency fitted on the MCP2515 module (typically 8MHz or 16MHz)
#ifndef CAN_OSC_CLOCK_MHZ
#define CAN_OSC_CLOCK_MHZ   8    // 8 for 8MHz crystal (most blue modules), 16 for 16MHz
#endif

// ==========================================
// Status Indicators & User Peripherals
// ==========================================
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN      2    // Built-in blue LED on most ESP32 dev boards
#endif

#ifndef OBD_ACTIVE_LED_PIN
#define OBD_ACTIVE_LED_PIN  15   // Optional LED for OBD active communication
#endif

#ifndef BLE_ACTIVE_LED_PIN
#define BLE_ACTIVE_LED_PIN  13   // Optional LED for BLE connection status
#endif

// ==========================================
// Optional Automotive Power Sensing
// ==========================================
#ifndef BATT_SENSE_PIN
#define BATT_SENSE_PIN      34   // ADC1 pin for 12V automotive battery voltage sensing (via divider)
#endif

#ifndef BATT_VOLTAGE_DIVIDER_RATIO
#define BATT_VOLTAGE_DIVIDER_RATIO  11.0f // e.g. 100k / 10k divider
#endif
