#include <stdarg.h>

#include "logger.h"

void log_info(const char *msg) 
{
    fprintf(stderr, "INFO: %s\n", msg);
}

void log_error(const char *msg) 
{
    fprintf(stderr, "ERROR: %s: %s\n", msg, strerror(errno));
}

void log_infof(const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "INFO: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_errorf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, ": %s\n", strerror(errno));
    va_end(args);
}