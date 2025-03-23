#include "save_socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define FTP_PORT 21
#define BUFFER_SIZE 1024


void handle_client(int client_sock) 
{
    char buffer[BUFFER_SIZE];

    send(client_sock, "220 FTP Server Ready\r\n", 22, 0);

    while (1) 
    {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) 
        {
            break;
        }

        printf("Received: %s", buffer);

        if (strncmp(buffer, "USER ", 5) == 0) 
        {
            send(client_sock, "331 User name okay, need password.\r\n", 36, 0);
        } 
        else if (strncmp(buffer, "PASS ", 5) == 0) 
        {
            send(client_sock, "230 User logged in, proceed.\r\n", 31, 0);
        } 
        else if (strncmp(buffer, "QUIT", 4) == 0)
        {
            send(client_sock, "221 Goodbye.\r\n", 14, 0);
            break;
        } 
        else 
        {
            send(client_sock, "500 Command not recognized.\r\n", 29, 0);
        }
    }

    close(client_sock);
}

int main() 
{
    int server_sock;
    int client_sock;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket_s(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(FTP_PORT);

    bind_s(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen_s(server_sock, 5);
   
    printf("FTP server listening on port %d...\n", FTP_PORT);

    while (1) 
    {
        client_sock = accept_s(server_sock, (struct sockaddr*)&client_addr, &client_len);

        printf("Client connected\n");
        handle_client(client_sock);
    }

    close(server_sock);
    return 0;
}
