/**
 * @file ble-service.js
 * @brief Web Bluetooth API integration for DriveSense OBD.
 */

class BLEService {
    constructor() {
        this.SERVICE_UUID = '9b20b22a-5a91-4c60-84a1-0e1d52033000';
        this.CHAR_TELEM_UUID = '9b20b22a-5a91-4c60-84a1-0e1d52033001';
        this.CHAR_CMD_UUID = '9b20b22a-5a91-4c60-84a1-0e1d52033002';
        this.CHAR_STATUS_UUID = '9b20b22a-5a91-4c60-84a1-0e1d52033003';

        this.device = null;
        this.server = null;
        this.telemChar = null;
        this.cmdChar = null;
        this.statusChar = null;
        this.isConnected = false;

        this.onTelemetryCallback = null;
        this.onStatusCallback = null;
        this.onConnectionChangeCallback = null;
        this.onLogCallback = null;
    }

    isSupported() {
        return !!(navigator.bluetooth && navigator.bluetooth.requestDevice);
    }

    log(msg, type = 'ble') {
        if (this.onLogCallback) {
            this.onLogCallback(`[BLE] ${msg}`, type);
        }
    }

    async connect() {
        if (!this.isSupported()) {
            throw new Error('Web Bluetooth API is not supported in this browser. Please use Google Chrome, Microsoft Edge, or switch on the Virtual ESP32.');
        }

        try {
            this.log('Requesting Bluetooth Device with service UUID...', 'sys');
            this.device = await navigator.bluetooth.requestDevice({
                filters: [
                    { namePrefix: 'DriveSense' },
                    { services: [this.SERVICE_UUID] }
                ],
                optionalServices: [this.SERVICE_UUID]
            });

            this.device.addEventListener('gattserverdisconnected', () => this.handleDisconnection());

            this.log(`Connecting to GATT Server on ${this.device.name || 'Device'}...`, 'sys');
            this.server = await this.device.gatt.connect();

            this.log('Discovering Primary DriveSense Service...', 'sys');
            const service = await this.server.getPrimaryService(this.SERVICE_UUID);

            this.log('Accessing Characteristics (Telemetry, Command, Status)...', 'sys');
            this.telemChar = await service.getCharacteristic(this.CHAR_TELEM_UUID);
            this.cmdChar = await service.getCharacteristic(this.CHAR_CMD_UUID);
            
            try {
                this.statusChar = await service.getCharacteristic(this.CHAR_STATUS_UUID);
            } catch (e) {
                this.log('Status characteristic not found, using command responses only', 'warn');
            }

            // Start Telemetry Notifications
            await this.telemChar.startNotifications();
            this.telemChar.addEventListener('characteristicvaluechanged', (e) => this.handleTelemetryNotification(e));

            // Start Status Notifications if available
            if (this.statusChar) {
                await this.statusChar.startNotifications();
                this.statusChar.addEventListener('characteristicvaluechanged', (e) => this.handleStatusNotification(e));
            }

            this.isConnected = true;
            this.log('Connected successfully to DriveSense OBD Peripheral!', 'obd');

            if (this.onConnectionChangeCallback) {
                this.onConnectionChangeCallback(true);
            }

            return true;
        } catch (error) {
            this.log(`Connection failed: ${error.message}`, 'err');
            this.disconnect();
            throw error;
        }
    }

    disconnect() {
        if (this.device && this.device.gatt && this.device.gatt.connected) {
            this.device.gatt.disconnect();
        }
        this.handleDisconnection();
    }

    handleDisconnection() {
        const wasConnected = this.isConnected;
        this.isConnected = false;
        this.server = null;
        this.telemChar = null;
        this.cmdChar = null;
        this.statusChar = null;

        if (wasConnected) {
            this.log('Device Disconnected.', 'warn');
        }

        if (this.onConnectionChangeCallback) {
            this.onConnectionChangeCallback(false);
        }
    }

    async sendCommand(commandString) {
        if (!this.isConnected || !this.cmdChar) {
            throw new Error('Not connected to device');
        }

        try {
            const encoder = new TextEncoder();
            const data = encoder.encode(commandString);
            this.log(`TX -> ${commandString}`, 'tx');
            await this.cmdChar.writeValue(data);
            return true;
        } catch (error) {
            this.log(`Failed to send command ${commandString}: ${error.message}`, 'err');
            throw error;
        }
    }

    handleTelemetryNotification(event) {
        const value = event.target.value;
        const decoder = new TextDecoder('utf-8');
        const jsonString = decoder.decode(value);

        try {
            const data = JSON.parse(jsonString);
            if (this.onTelemetryCallback) {
                this.onTelemetryCallback(data, jsonString);
            }
        } catch (e) {
            this.log(`Corrupt telemetry packet received: ${jsonString}`, 'err');
        }
    }

    handleStatusNotification(event) {
        const value = event.target.value;
        const decoder = new TextDecoder('utf-8');
        const text = decoder.decode(value);

        this.log(`RX Status -> ${text}`, 'rx');
        if (this.onStatusCallback) {
            this.onStatusCallback(text);
        }
    }
}

window.bleService = new BLEService();
