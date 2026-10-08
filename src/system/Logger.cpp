#include "Logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#if defined(ARDUINO)
#include <Arduino.h>
#elif defined(ESP_PLATFORM)
#include "esp_timer.h"
#include <esp_log.h>
#else
#include <chrono>
#endif

LogLevel Logger::s_minLevel = LOG_LEVEL_DEBUG;

void Logger::init(LogLevel minLevel) {
    s_minLevel = minLevel;
#if defined(ARDUINO)
    // Serial is initialized in main setup()
#endif
}

void Logger::setLogLevel(LogLevel level) {
    s_minLevel = level;
}

LogLevel Logger::getLogLevel() {
    return s_minLevel;
}

static uint32_t getSystemMillis() {
#if defined(ARDUINO)
    return millis();
#elif defined(ESP_PLATFORM)
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
#else
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
#endif
}

void Logger::logFormatted(LogLevel level, const char* tag, const char* format, va_list args) {
    if (level < s_minLevel) {
        return;
    }

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);

    uint32_t ms = getSystemMillis();
    uint32_t sec = ms / 1000;
    uint32_t msec = ms % 1000;

    // Structured logging output: [SEC.MSEC] [TAG] Message
#if defined(ARDUINO)
    Serial.printf("[%04u.%03u] [%s] %s\r\n", sec, msec, tag, buffer);
#else
    printf("[%04u.%03u] [%s] %s\n", sec, msec, tag, buffer);
    fflush(stdout);
#endif
}

void Logger::debug(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_DEBUG, tag, format, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_INFO, tag, format, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_WARN, tag, format, args);
    va_end(args);
}

void Logger::error(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_ERROR, tag, format, args);
    va_end(args);
}

void Logger::logBoot(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_INFO, "BOOT", format, args);
    va_end(args);
}

void Logger::logCAN(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_INFO, "CAN", format, args);
    va_end(args);
}

void Logger::logOBD(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_INFO, "OBD", format, args);
    va_end(args);
}

void Logger::logBLE(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_INFO, "BLE", format, args);
    va_end(args);
}

void Logger::logTelemetry(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_INFO, "TELEMETRY", format, args);
    va_end(args);
}

void Logger::logSystem(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LOG_LEVEL_INFO, "SYS", format, args);
    va_end(args);
}
