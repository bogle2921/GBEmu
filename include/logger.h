#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include "config.h"

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
    LOG_TESTS,
    LOG_COUNT
} LogComponent;

// LOG LEVELS, SHOULD MOSTLY USE TRACE
typedef enum {
    LOG_TEST,
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
#if LOG_TO_FILE || LOG_TO_STDOUT
#define LOG_ERROR(component, ...) log_message(component, LOG_ERROR, __VA_ARGS__)
#define LOG_WARN(component, ...)  log_message(component, LOG_WARN, __VA_ARGS__)
#define LOG_INFO(component, ...)  log_message(component, LOG_INFO, __VA_ARGS__)
#define LOG_DEBUG(component, ...) log_message(component, LOG_DEBUG, __VA_ARGS__)
#define LOG_TRACE(component, ...) log_message(component, LOG_TRACE, __VA_ARGS__)
#else
#define LOG_ERROR(component, ...) ((void)0)
#define LOG_WARN(component, ...)  ((void)0)
#define LOG_INFO(component, ...)  ((void)0)
#define LOG_DEBUG(component, ...) ((void)0)
#define LOG_TRACE(component, ...) ((void)0)
#endif
#if LOG_VERBOSE_CPU
#define LOG_TEST(...) log_message(LOG_TESTS, LOG_TEST, __VA_ARGS__)
#else
#define LOG_TEST(...) ((void)0)
#endif

// DONT USE THIS
void log_message(LogComponent component, LogLevel level, const char* fmt, ...);

#endif