#include "JSONSerializer.h"
#include <stdio.h>
#include <inttypes.h>

size_t JSONSerializer::serialize(const TelemetryData& data, char* outBuffer, size_t maxLen) {
    if (!outBuffer || maxLen == 0) {
        return 0;
    }

    char speedBuf[16];
    char rpmBuf[16];
    char loadBuf[16];
    char throttleBuf[16];
    char coolantBuf[16];
    char iatBuf[16];
    char mafBuf[16];
    char fuelBuf[16];

    if (data.speedAvailable) snprintf(speedBuf, sizeof(speedBuf), "%.1f", data.speed);
    else snprintf(speedBuf, sizeof(speedBuf), "null");

    if (data.rpmAvailable) snprintf(rpmBuf, sizeof(rpmBuf), "%.0f", data.rpm);
    else snprintf(rpmBuf, sizeof(rpmBuf), "null");

    if (data.engineLoadAvailable) snprintf(loadBuf, sizeof(loadBuf), "%.1f", data.engineLoad);
    else snprintf(loadBuf, sizeof(loadBuf), "null");

    if (data.throttleAvailable) snprintf(throttleBuf, sizeof(throttleBuf), "%.1f", data.throttlePosition);
    else snprintf(throttleBuf, sizeof(throttleBuf), "null");

    if (data.coolantTemperatureAvailable) snprintf(coolantBuf, sizeof(coolantBuf), "%.1f", data.coolantTemperature);
    else snprintf(coolantBuf, sizeof(coolantBuf), "null");

    if (data.intakeAirTemperatureAvailable) snprintf(iatBuf, sizeof(iatBuf), "%.1f", data.intakeAirTemperature);
    else snprintf(iatBuf, sizeof(iatBuf), "null");

    if (data.mafAvailable) snprintf(mafBuf, sizeof(mafBuf), "%.2f", data.massAirFlow);
    else snprintf(mafBuf, sizeof(mafBuf), "null");

    if (data.fuelLevelAvailable) snprintf(fuelBuf, sizeof(fuelBuf), "%.1f", data.fuelLevel);
    else snprintf(fuelBuf, sizeof(fuelBuf), "null");

    int written = snprintf(outBuffer, maxLen,
        "{\"timestamp\":%" PRIu64 ","
        "\"speed\":%s,"
        "\"rpm\":%s,"
        "\"engine_load\":%s,"
        "\"throttle_position\":%s,"
        "\"coolant_temperature\":%s,"
        "\"intake_air_temperature\":%s,"
        "\"mass_air_flow\":%s,"
        "\"fuel_level\":%s}",
        data.timestamp,
        speedBuf,
        rpmBuf,
        loadBuf,
        throttleBuf,
        coolantBuf,
        iatBuf,
        mafBuf,
        fuelBuf
    );

    if (written < 0 || (size_t)written >= maxLen) {
        return 0; // Truncation or error
    }

    return (size_t)written;
}
