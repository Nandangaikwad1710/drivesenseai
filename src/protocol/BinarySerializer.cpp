#include "BinarySerializer.h"
#include <string.h>

size_t BinarySerializer::serialize(const TelemetryData& data, char* outBuffer, size_t maxLen) {
    if (!outBuffer || maxLen < sizeof(BinaryTelemetryPacket)) {
        return 0;
    }

    BinaryTelemetryPacket pkt = {};
    pkt.header[0] = 0xAA;
    pkt.header[1] = 0x55;

    uint16_t flags = 0;
    if (data.speedAvailable) { flags |= (1 << 0); pkt.speedX10 = (uint16_t)(data.speed * 10.0f); }
    if (data.rpmAvailable) { flags |= (1 << 1); pkt.rpmX4 = (uint16_t)(data.rpm * 4.0f); }
    if (data.throttleAvailable) { flags |= (1 << 2); pkt.throttleX10 = (uint16_t)(data.throttlePosition * 10.0f); }
    if (data.engineLoadAvailable) { flags |= (1 << 3); pkt.engineLoadX10 = (uint16_t)(data.engineLoad * 10.0f); }
    if (data.coolantTemperatureAvailable) { flags |= (1 << 4); pkt.coolantTempX10 = (int16_t)(data.coolantTemperature * 10.0f); }
    if (data.intakeAirTemperatureAvailable) { flags |= (1 << 5); pkt.intakeTempX10 = (int16_t)(data.intakeAirTemperature * 10.0f); }
    if (data.mafAvailable) { flags |= (1 << 6); pkt.mafX100 = (uint16_t)(data.massAirFlow * 100.0f); }
    if (data.fuelLevelAvailable) { flags |= (1 << 7); pkt.fuelLevelX10 = (uint16_t)(data.fuelLevel * 10.0f); }

    pkt.availableFlags = flags;
    pkt.timestampMs = (uint32_t)data.timestamp;

    // Calculate XOR checksum
    uint8_t chk = 0;
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&pkt);
    for (size_t i = 0; i < sizeof(BinaryTelemetryPacket) - 1; ++i) {
        chk ^= raw[i];
    }
    pkt.checksum = chk;

    memcpy(outBuffer, &pkt, sizeof(BinaryTelemetryPacket));
    return sizeof(BinaryTelemetryPacket);
}
