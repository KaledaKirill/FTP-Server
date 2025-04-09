#ifndef NETWORK_H
#define NETWORK_H

typedef struct 
{
    int server_sock;
    int port;
    char *ip_addr;
} network_config;

int init_network(network_config *config);
int accept_client(network_config *config);
void close_network(network_config *config);

#endif