#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>

// EACH GETS ITS OWN FILE, FIRST ARG OF LOG MACROS IS ONE OF THESE
typedef enum {
    LOG_MAIN,
    LOG_CPU,
    LOG_BUS,
    LOG_GRAPHICS,
    LOG_CART,
    LOG_DMA,
    LOG_TIMER,
    LOG_INTERRUPT,
    LOG_COUNT
} LogComponent;

// LOG LEVELS, SHOULD MOSTLY USE TRACE
typedef enum {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_TRACE
} LogLevel;

// CALL IN MAIN
void logger_init(LogLevel global_level);
void logger_cleanup(void);

// LOGGING MACROS - USE THESE
#define LOG_ERROR(component, ...) log_message(component, LOG_ERROR, __VA_ARGS__)
#define LOG_WARN(component, ...)  log_message(component, LOG_WARN, __VA_ARGS__)
#define LOG_INFO(component, ...)  log_message(component, LOG_INFO, __VA_ARGS__)
#define LOG_DEBUG(component, ...) log_message(component, LOG_DEBUG, __VA_ARGS__)
#define LOG_TRACE(component, ...) log_message(component, LOG_TRACE, __VA_ARGS__)

// DONT USE THIS
void log_message(LogComponent component, LogLevel level, const char* fmt, ...);

#endif