#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "logger.h"

static FILE *log_file = NULL;

// Вспомогательная функция для получения временной метки
static void get_time_str(char *buffer, size_t buffer_size)
{
    time_t now = time(NULL);
    struct tm tm_info;
    if (localtime_r(&now, &tm_info) == NULL) 
    {
        snprintf(buffer, buffer_size, "UNKNOWN_TIME");
        return;
    }
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &tm_info);
}

int logger_init(const char *log_file_path)
{
    if (log_file != NULL) 
    {
        fprintf(stderr, "ERROR: Logger already initialized\n");
        return -1;
    }

    if (!log_file_path) 
    {
        fprintf(stderr, "ERROR: Log file path is NULL\n");
        return -1;
    }

    log_file = fopen(log_file_path, "a");
    if (log_file == NULL) 
    {
        fprintf(stderr, "ERROR: Failed to open log file %s: %s\n", log_file_path, strerror(errno));
        return -1;
    }

    char time_str[64];
    get_time_str(time_str, sizeof(time_str));
    fprintf(stderr, "[%s] INFO: Logger initialized, writing to %s\n", time_str, log_file_path);
    fprintf(log_file, "[%s] INFO: Logger initialized, writing to %s\n", time_str, log_file_path);
    fflush(log_file);

    return 0;
}

void logger_cleanup(void)
{
    if (log_file != NULL) 
    {
        char time_str[64];
        get_time_str(time_str, sizeof(time_str));
        fprintf(log_file, "[%s] INFO: Logger shutting down\n", time_str);
        fclose(log_file);
        log_file = NULL;
        fprintf(stderr, "[%s] INFO: Logger shut down\n", time_str);
    }
}

void log_info(const char *msg)
{
    if (!msg) return;

    char time_str[64];
    get_time_str(time_str, sizeof(time_str));

    fprintf(stderr, "[%s] INFO: %s\n", time_str, msg);
    if (log_file != NULL) 
    {
        fprintf(log_file, "[%s] INFO: %s\n", time_str, msg);
        fflush(log_file);
    }
}

void log_error(const char *msg)
{
    if (!msg) return;

    char time_str[64];
    get_time_str(time_str, sizeof(time_str));

    fprintf(stderr, "[%s] ERROR: %s: %s\n", time_str, msg, strerror(errno));
    if (log_file != NULL) 
    {
        fprintf(log_file, "[%s] ERROR: %s: %s\n", time_str, msg, strerror(errno));
        fflush(log_file);
    }
}

void log_infof(const char *fmt, ...)
{
    if (!fmt) return;

    char buffer[1024]; // Буфер для форматированного сообщения
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    char time_str[64];
    get_time_str(time_str, sizeof(time_str));

    fprintf(stderr, "[%s] INFO: %s\n", time_str, buffer);
    if (log_file != NULL) 
    {
        fprintf(log_file, "[%s] INFO: %s\n", time_str, buffer);
        fflush(log_file);
    }
}

void log_errorf(const char *fmt, ...)
{
    if (!fmt) return;

    char buffer[1024]; // Буфер для форматированного сообщения
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    char time_str[64];
    get_time_str(time_str, sizeof(time_str));

    fprintf(stderr, "[%s] ERROR: %s: %s\n", time_str, buffer, strerror(errno));
    if (log_file != NULL) 
    {
        fprintf(log_file, "[%s] ERROR: %s: %s\n", time_str, buffer, strerror(errno));
        fflush(log_file);
    }
}