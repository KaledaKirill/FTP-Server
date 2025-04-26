#include <sys/socket.h>

#include "network.h"
#include "session.h"
#include "commands.h"
#include "logger.h"
#include "auth.h"
#include "config.h"

#define CONFIG_FILE "./configs.conf"

static int init_server(config *config, network_config *net_config)
{
    if (config_init(CONFIG_FILE, config) != 0)
    {
        log_error("Failed to initialize configs");
        return -1;
    }

    if (auth_init(config->users_file) != 0) 
    {
        log_error("Failed to initialize authentication module");
        return -1;
    }

    if (logger_init(config->log_file) != 0)
    {
        log_error("Failed to initialize logger");
        return -1;
    }

    net_config->ip_addr = config->ip_addr;
    net_config->port = config->port;
    if (init_network(net_config) != 0) 
    {
        log_error("Failed to initialize network");
        return -1;
    }

    return 0;
} 

int main() 
{
    config config = { 0 };
    network_config net_config = { 0 };

    if (init_server(&config, &net_config) != 0)
    {
        log_error("Failed to initialize server");
        return 1;
    }

    while (1) 
    {
        int client_sock = accept_client(&net_config);
        if (client_sock < 0)
            continue;

        ftp_session session;
        init_session(&session, client_sock, &config);
        send_response(&session, "220 FTP Server Ready\r\n");

        char buffer[BUFFER_SIZE];
        while (1) 
        {
            int bytes = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytes <= 0) 
            {
                log_infof("Client %d disconnected", client_sock);
                break;
            }
            
            buffer[bytes] = '\0';
            log_infof("Received from client %d: %s", client_sock, buffer);
            if (process_command(&session, buffer) == 1) 
                break;
        
        }

        close_session(&session);
    }

    auth_cleanup();
    logger_cleanup();
    close_network(&net_config);
    return 0;
}