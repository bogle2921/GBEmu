#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#define MAX_LOG_SIZE (10 * 1024 * 1024)
#define MAX_LOG_FILES 3
#define LOG_DIR "logs"

typedef struct {
    FILE* file;
    char filename[256];
    LogLevel level;
    bool enabled;
} LogContext;

// SINGLETON
static LogContext log_contexts[LOG_COUNT];

// EACH PART GETS ITS OWN LOG FILE ON ROTATION
static const char* component_names[] = {
    "main",
    "cpu",
    "bus", 
    "graphics",
    "cart",
    "dma",
    "timer",
    "interrupt"
};

// ROTATE IF LOG FILE GREATER THAN 10MB, ROTATING UP TO 3 FILES
static void check_rotate_log(LogComponent component) {
    LogContext* ctx = &log_contexts[component];
    
    struct stat st;
    if (stat(ctx->filename, &st) == 0) {
        if (st.st_size > MAX_LOG_SIZE) {
            fclose(ctx->file);
            
            char reserved_name[512];
            snprintf(reserved_name, sizeof(reserved_name), "%s.reserved.log", ctx->filename);
            
            // IF RESERVED FILE DOESN'T EXIST, CREATE IT
            if (stat(reserved_name, &st) != 0) {
                rename(ctx->filename, reserved_name);
            } else {
                // OTHERWISE DO NORMAL ROTATION
                char old_name[512], new_name[512];
                for (int i = MAX_LOG_FILES-1; i >= 1; i--) {
                    snprintf(old_name, sizeof(old_name), "%s.%d.log", ctx->filename, i);
                    snprintf(new_name, sizeof(new_name), "%s.%d.log", ctx->filename, i+1);
                    rename(old_name, new_name);
                }
                
                // MOVE CURRENT TO .1.LOG
                snprintf(new_name, sizeof(new_name), "%s.1.log", ctx->filename);
                rename(ctx->filename, new_name);
            }
            
            ctx->file = fopen(ctx->filename, "a");
        }
    }
}

// INIT IN MAIN
void logger_init(LogLevel global_level) {
    #ifdef _WIN32
        mkdir(LOG_DIR);
    #else
        mkdir(LOG_DIR, 0777);
    #endif

    #ifdef DEBUG
    printf("INITIALIZING ALL LOGGERS WITH LEVEL: %d\n", global_level);
    #endif

    for (int i = 0; i < LOG_COUNT; i++) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s.log", LOG_DIR, component_names[i]);

        log_contexts[i].file = fopen(filepath, "a");
        strncpy(log_contexts[i].filename, filepath, sizeof(log_contexts[i].filename)-1);
        log_contexts[i].level = global_level;
        log_contexts[i].enabled = true;
    }
}

// CLOSE IN MAIN
void logger_cleanup(void) {
    for (int i = 0; i < LOG_COUNT; i++) {
        if (log_contexts[i].file) {
            fclose(log_contexts[i].file);
            log_contexts[i].file = NULL;
        }
    }
}

// DONT USE, USE MACROS, SEE HEADER FILE
void log_message(LogComponent component, LogLevel level, const char* fmt, ...) {
    LogContext* ctx = &log_contexts[component];
    
    if (!ctx->enabled || level > ctx->level || !ctx->file) {
        return;
    }

    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    const char* level_str = "UNKNOWN";
    switch(level) {
        case LOG_ERROR: level_str = "ERROR"; break;
        case LOG_WARN:  level_str = "WARN"; break;
        case LOG_INFO:  level_str = "INFO"; break;
        case LOG_DEBUG: level_str = "DEBUG"; break;
        case LOG_TRACE: level_str = "TRACE"; break;
    }

    va_list args;
    va_start(args, fmt);
    char message[4096];
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    #ifdef DEBUG
    printf("[%s][%s] %s\n", timestamp, level_str, message);
    #endif
    
    fprintf(ctx->file, "[%s][%s] %s\n", timestamp, level_str, message);
    fflush(ctx->file);

    check_rotate_log(component);
}

