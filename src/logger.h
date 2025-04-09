#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

void log_info(const char *msg);
void log_error(const char *msg);
void log_infof(const char *fmt, ...);
void log_errorf(const char *fmt, ...);

#endif