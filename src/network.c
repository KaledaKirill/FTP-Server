#include <arpa/inet.h>
#include <unistd.h>

#include "network.h"
#include "logger.h"

int init_network(network_config *config)
{
    if (!config)
    {
        log_error("Net config is empty!");
        return -1;
    }

    config->server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (config->server_sock < 0) 
    {
        log_error("Failed to create socket");
        return -1;
    }

    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(config->ip_addr);
    server_addr.sin_port = htons(config->port);

    int opt = 1;
    setsockopt(config->server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(config->server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        log_error("Bind failed");
        close(config->server_sock);
        return -1;
    }

    if (listen(config->server_sock, 1) < 0) 
    {
        log_error("Listen failed");
        close(config->server_sock);
        return -1;
    }

    log_infof("Server listening on %s:%d", config->ip_addr, config->port);
    return 0;
}

int accept_client(network_config *config) 
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_sock = accept(config->server_sock, (struct sockaddr*)&client_addr, &len);
    if (client_sock < 0) 
    {
        log_error("Accept failed");
        return -1;
    }
    log_infof("Client connected: %d", client_sock);
    return client_sock;
}

void close_network(network_config *config) 
{
    if (config->server_sock != -1) 
    {
        close(config->server_sock);
        log_info("Server socket closed");
    }
}