#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "logger.h"

#define DEFAULT_ROOT_DIR "/home/kirill/ftp_root"
#define DEFAULT_PORT 21
#define DEFAULT_IP_ADDR "127.0.0.1"
#define DEFAULT_USERS_FILE "./users.conf"
#define DEFAULT_LOG_FILE "./ftp_logs.log"
#define DEFAULT_CWD "/"

static char *strdup_s(const char *value)
{
    if (!value) 
    {
        log_error("strdup_s: Input value is NULL");
        return NULL;
    }
    char *result = strdup(value);
    if (!result) 
    {
        log_error("Memory allocation failed for config value");
    }
    return result;
}

static int default_init(config *config)
{
    config->root_dir = strdup_s(DEFAULT_ROOT_DIR);
    config->port = DEFAULT_PORT;
    config->ip_addr = strdup_s(DEFAULT_IP_ADDR);
    config->users_file = strdup_s(DEFAULT_USERS_FILE);
    config->log_file = strdup_s(DEFAULT_LOG_FILE);
    config->default_cwd = strdup_s(DEFAULT_CWD);

    if (!config->root_dir || !config->ip_addr || !config->users_file || 
        !config->log_file || !config->default_cwd) 
    {
        log_error("Failed to initialize default config values");
        config_cleanup(config);
        return -1;
    }

    return 0;
}

int config_init(const char *config_file, config *conf) 
{
    if (!conf || !config_file) 
    {
        log_error("Invalid arguments to config_init");
        return -1;
    }

    memset(conf, 0, sizeof(config));

    if (default_init(conf) == -1)
        return -1;

    FILE *file = fopen(config_file, "r");
    if (!file) 
    {
        log_infof("Failed to open config file %s: %s. Using default values.", 
                  config_file, strerror(errno));
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) 
    {
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0 || line[0] == '#') 
            continue;

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");
        if (!key || !value) 
        {
            log_infof("Invalid config line: %s", line);
            continue;
        }

        while (*key == ' ') key++;
        while (*value == ' ') value++;
        char *end = key + strlen(key) - 1;
        while (end > key && *end == ' ') *end-- = '\0';
        end = value + strlen(value) - 1;
        while (end > value && *end == ' ') *end-- = '\0';

        if (strcmp(key, "root_dir") == 0) 
        {
            free(conf->root_dir);
            conf->root_dir = strdup_s(value);
            if (!conf->root_dir) 
            {
                fclose(file);
                return -1;
            }
        } 
        else if (strcmp(key, "port") == 0) 
        {
            conf->port = atoi(value);
            if (conf->port <= 0 || conf->port > 65535) 
            {
                log_infof("Invalid port %s, using default %d", value, DEFAULT_PORT);
                conf->port = DEFAULT_PORT;
            }
        } 
        else if (strcmp(key, "ip_addr") == 0) 
        {
            free(conf->ip_addr);
            struct in_addr addr;
            if (inet_pton(AF_INET, value, &addr) != 1) 
            {
                log_infof("Invalid IP address %s, using default %s", value, DEFAULT_IP_ADDR);
                conf->ip_addr = strdup_s(DEFAULT_IP_ADDR);
            } 
            else 
            {
                conf->ip_addr = strdup_s(value);
            }
            if (!conf->ip_addr) 
            {
                fclose(file);
                return -1;
            }
        } 
        else if (strcmp(key, "users_file") == 0) 
        {
            free(conf->users_file);
            conf->users_file = strdup_s(value);
            if (!conf->users_file) 
            {
                fclose(file);
                return -1;
            }
        } 
        else if (strcmp(key, "log_file") == 0) 
        {
            free(conf->log_file);
            conf->log_file = strdup_s(value);
            if (!conf->log_file) 
            {
                fclose(file);
                return -1;
            }
        } 
        else if (strcmp(key, "default_cwd") == 0) 
        {
            free(conf->default_cwd);
            conf->default_cwd = strdup_s(value);
            if (!conf->default_cwd) 
            {
                fclose(file);
                return -1;
            }
        } 
        else 
        {
            log_infof("Unknown config key: %s", key);
        }
    }

    fclose(file);
    log_infof("Configuration loaded: root_dir=%s, port=%d, ip_addr=%s, users_file=%s, log_file=%s, default_cwd=%s",
              conf->root_dir, conf->port, conf->ip_addr, conf->users_file, 
              conf->log_file, conf->default_cwd);

    return 0;
}

void config_cleanup(config *conf) 
{
    if (!conf) 
        return;

    free(conf->root_dir);
    free(conf->ip_addr);
    free(conf->users_file);
    free(conf->log_file);
    free(conf->default_cwd);
    memset(conf, 0, sizeof(config));
}