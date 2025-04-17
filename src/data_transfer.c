#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "data_transfer.h"
#include "logger.h"

int setup_data_connection(ftp_session *session) 
{
    if (session->data_connection == PASSIVE) 
    {
        int data_conn = accept(session->data_sock, NULL, NULL);
        if (data_conn < 0) 
        {
            log_errorf("Failed to accept data connection: %s", strerror(errno)); //TODO: Divide into funcs
            close_data_connection(session);
            return -1;
        }

        close(session->data_sock);
        session->data_sock = data_conn;
        log_infof("Passive data connection established from client");
    } 
    else if (session->data_connection == ACTIVE)
    {
        session->data_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (session->data_sock < 0) 
        {
            log_errorf("Failed to create data socket");
            close_data_connection(session);
            return -1;
        }
        if (connect(session->data_sock, (struct sockaddr*)&session->data_addr, sizeof(session->data_addr)) < 0) 
        {
            log_errorf("Failed to connect to client data port %d: %s", ntohs(session->data_addr.sin_port), strerror(errno));
            close_data_connection(session);
            return -1;
        }
        log_infof("Active data connection established to client port %d", ntohs(session->data_addr.sin_port));
    }
    else if (session->data_connection == NONE)
    {
        log_errorf("No data connection mode specified (PORT or PASV required)");
        return -1;
    }
    else
    {
        log_errorf("Invalid data connection mode: %d", session->data_connection);
        return -1;
    }
    return 0;
}

int send_data(ftp_session *session, const char *data) 
{
    if (session->data_sock == -1) {
        log_errorf("No data connection established");
        return -1;
    }

    if (send(session->data_sock, data, strlen(data), 0) < 0) {
        log_errorf("Failed to send data");
        return -1;
    }

    log_infof("Sent data: %s", data);
    return 0;
}

void close_data_connection(ftp_session *session) 
{
    if (session->data_sock != -1) 
        close(session->data_sock);
    
    session->data_sock = -1;
    memset(&session->data_addr, 0, sizeof(session->data_addr));
    session->data_connection = NONE;

    log_infof("Data connection closed");
}