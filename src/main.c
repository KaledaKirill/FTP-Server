#include <sys/socket.h>

#include "network.h"
#include "session.h"
#include "commands.h"
#include "logger.h"

int main() 
{
    network_config net_config = 
    { 
        .port = 21, 
        .ip_addr = "127.0.0.1" 
    };

    if (init_network(&net_config) < 0) 
    {
        log_error("Failed to initialize network");
        return 1;
    }

    while (1) 
    {
        int client_sock = accept_client(&net_config);
        if (client_sock < 0)
            continue;

        ftp_session session;
        init_session(&session, client_sock);
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

    close_network(&net_config);
    return 0;
}