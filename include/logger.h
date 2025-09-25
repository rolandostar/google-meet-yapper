#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Log levels
#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4
#define LOG_LEVEL_VERBOSE 5

// Default log level if not defined
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Log prefixes for identification
#define LOG_PREFIX_ERROR   "[ERROR] "
#define LOG_PREFIX_WARN    "[WARN]  "
#define LOG_PREFIX_INFO    "[INFO]  "
#define LOG_PREFIX_DEBUG   "[DEBUG] "
#define LOG_PREFIX_VERBOSE "[VERB]  "

// Logging macros
#if LOG_LEVEL >= LOG_LEVEL_ERROR
  #define LOG_ERROR(format, ...) Serial.printf(LOG_PREFIX_ERROR format "\n", ##__VA_ARGS__)
#else
  #define LOG_ERROR(format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
  #define LOG_WARN(format, ...) Serial.printf(LOG_PREFIX_WARN format "\n", ##__VA_ARGS__)
#else
  #define LOG_WARN(format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
  #define LOG_INFO(format, ...) Serial.printf(LOG_PREFIX_INFO format "\n", ##__VA_ARGS__)
#else
  #define LOG_INFO(format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
  #define LOG_DEBUG(format, ...) Serial.printf(LOG_PREFIX_DEBUG format "\n", ##__VA_ARGS__)
#else
  #define LOG_DEBUG(format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
  #define LOG_VERBOSE(format, ...) Serial.printf(LOG_PREFIX_VERBOSE format "\n", ##__VA_ARGS__)
#else
  #define LOG_VERBOSE(format, ...)
#endif

// Additional convenience macros
#define LOG_INIT() do { Serial.begin(115200); delay(100); } while(0)

// Function entry/exit logging (verbose level)
#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
  #define LOG_FUNC_ENTER() LOG_VERBOSE("ENTER: %s", __FUNCTION__)
  #define LOG_FUNC_EXIT() LOG_VERBOSE("EXIT: %s", __FUNCTION__)
#else
  #define LOG_FUNC_ENTER()
  #define LOG_FUNC_EXIT()
#endif

#endif // LOGGER_H
