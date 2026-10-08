#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/**
 * @file Logger.h
 * @brief Structured thread-safe serial logging system for DriveSense OBD.
 */

enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO  = 1,
    LOG_LEVEL_WARN  = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_NONE  = 4
};

class Logger {
public:
    static void init(LogLevel minLevel = LOG_LEVEL_DEBUG);
    static void setLogLevel(LogLevel level);
    static LogLevel getLogLevel();

    static void debug(const char* tag, const char* format, ...);
    static void info(const char* tag, const char* format, ...);
    static void warn(const char* tag, const char* format, ...);
    static void error(const char* tag, const char* format, ...);

    // Convenience category loggers matching design requirements
    static void logBoot(const char* format, ...);
    static void logCAN(const char* format, ...);
    static void logOBD(const char* format, ...);
    static void logBLE(const char* format, ...);
    static void logTelemetry(const char* format, ...);
    static void logSystem(const char* format, ...);

private:
    static LogLevel s_minLevel;
    static void logFormatted(LogLevel level, const char* tag, const char* format, va_list args);
};
