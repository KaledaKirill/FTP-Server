#ifndef CONFIG_H
#define CONFIG_H

#define PATH_LEN 1024
#define INET_ADDRSTRLEN 16

typedef struct 
{
    char *root_dir;
    int port;
    char *ip_addr;
    char *users_file;
    char *log_file;
    char *default_cwd;
} config;

int config_init(const char *config_file, config *config);
void config_cleanup(config *config);

#endif