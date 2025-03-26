#include "save_socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define FTP_PORT 21
#define BUFFER_SIZE 1024

typedef enum
{
    NONE_MODE,
    ACTIVE_MODE,
    PASSIVE_MODE
} data_transfer_mode;

data_transfer_mode data_trans_mode = NONE_MODE;

void send_response(int client_sock, const char *response) 
{
    send(client_sock, response, strlen(response), 0);
}

int connect_to_active_mode(struct sockaddr_in *client_data_addr) 
{
    int data_sock = socket_s(AF_INET, SOCK_STREAM, 0);
    connect_s(data_sock, (struct sockaddr*)client_data_addr, sizeof(*client_data_addr));

    return data_sock;
}

int create_data_connection(int client_sock, int pasv_listen_sock, struct sock_addr_in *client_data_addr)
{   
    int data_sock = -1;

    if (data_trans_mode = NONE_MODE)
    {
        send_response(client_sock, "425 No data connection mode set.\r\n");
    }
    else if (data_trans_mode == ACTIVE_MODE) 
    {
        data_sock = connect_to_active_mode(&client_data_addr);
    } 
    else if (data_trans_mode == PASSIVE_MODE) 
    {
        data_sock = accept(pasv_listen_sock, NULL, NULL);
    } 
    
    if (data_sock == -1)
    {
        send_response(client_sock, "425 Can't open data connection.\r\n");
    }

    return data_sock;
}

int accept_pasv_connection(int pasv_listen_sock) 
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int data_sock = accept(pasv_listen_sock, (struct sockaddr*)&client_addr, &client_len);
    
    if (data_sock < 0) {
        return -1;
    }
    
    return data_sock;
}

int pasv(int client_sock) 
{
    int pasv_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (pasv_listen_sock < 0) {
        send_response(client_sock, "425 Cannot create passive socket.\r\n");
        return -1;
    }

    struct sockaddr_in pasv_addr = { 0 };
    pasv_addr.sin_family = AF_INET;
    pasv_addr.sin_addr.s_addr = INADDR_ANY;
    pasv_addr.sin_port = 0;

    if (bind(pasv_listen_sock, (struct sockaddr*)&pasv_addr, sizeof(pasv_addr)) < 0)
    {
        send_response(client_sock, "425 Cannot bind passive socket.\r\n");
        close(pasv_listen_sock);
        return -1;
    }

    socklen_t addrlen = sizeof(pasv_addr);
    if (getsockname(pasv_listen_sock, (struct sockaddr*)&pasv_addr, &addrlen) < 0)
    {
        send_response(client_sock, "425 Cannot get socket name.\r\n");
        close(pasv_listen_sock);
        return -1;
    }

    if (listen(pasv_listen_sock, 1) == -1)
    {
        send_response(client_sock, "425 Cannot listen on passive socket.\r\n");
        close(pasv_listen_sock);
        return -1;
    }

    int p1 = ntohs(pasv_addr.sin_port) / 256;
    int p2 = ntohs(pasv_addr.sin_port) % 256;
    //char ip_str[16] = "192,168,119,16"; // Локальный IP (для тестов) 
    char ip_str[16] = "127,0,0,1"; //TODO: use dynamic ip adress 

    char response[64];
    snprintf(response, sizeof(response), "227 Entering Passive Mode (%s,%d,%d).\r\n", ip_str, p1, p2);
    send_response(client_sock, response);

    data_trans_mode = PASSIVE_MODE;
    return pasv_listen_sock;
}

int type(int client_sock, char *buffer)
{
    if (strncmp(buffer, "TYPE I", 6) == 0) 
    {
        send_response(client_sock, "200 Type set to I (binary).\r\n");
    } 
    else if (strncmp(buffer, "TYPE A", 6) == 0) 
    {
        send_response(client_sock, "200 Type set to A (ASCII).\r\n");
    } 
    else 
    {
        send_response(client_sock, "500 Syntax error, command unrecognized.\r\n");
    }

    return 0;
}

int cwd(int client_sock, char *buffer)
{
    char path[BUFFER_SIZE];
    if (sscanf(buffer, "CWD %s", path) == 1)
    {
        if (chdir(path) == 0)
        {
            send_response(client_sock, "250 Requested file action okay, completed.\r\n");
        }
        else
        {
            send_response(client_sock, "550 Failed to change directory.\r\n");
        }
    }
    else
    {
        send_response(client_sock, "500 Syntax error in parameters or arguments.\r\n");
    }

    return 0;
}

int pwd(int client_sock)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char response[BUFFER_SIZE];
        int len = snprintf(response, sizeof(response), "257 \"%s\" is the current directory.\r\n", cwd);

        if (len >= (int)sizeof(response)) {
            send_response(client_sock, "550 Current directory path is too long.\r\n");
        } else {
            send_response(client_sock, response);
        }
    }
    else
    {
        send_response(client_sock, "550 Failed to get current directory.\r\n");
    }
    return 0;
}

// Обработка команды USER
int user(int client_sock) 
{
    send_response(client_sock, "331 User name okay, need password.\r\n");
    return 0;
}

// Обработка команды PASS
int pass(int client_sock)
{
    send_response(client_sock, "230 User logged in, proceed.\r\n");
    return 0;
}

int quit(int client_sock) 
{
    send_response(client_sock, "221 Goodbye.\r\n");
    return 1;
}

int port(int client_sock, char *buffer, struct sockaddr_in *client_data_addr) 
{
    int p1, p2, ip1, ip2, ip3, ip4;

    if (sscanf(buffer, "PORT %d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &p1, &p2) == 6) 
    {
        int data_port = (p1 << 8) + p2;  
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip1, ip2, ip3, ip4);  

        client_data_addr->sin_family = AF_INET;
        client_data_addr->sin_port = htons(data_port);
        inet_pton(AF_INET, ip_str, &client_data_addr->sin_addr);  

        send_response(client_sock, "200 PORT command successful.\r\n");
        data_trans_mode = ACTIVE_MODE;
        return 0;
    } 

    send_response(client_sock, "500 Syntax error in parameters or arguments.\r\n");
    return -1;
}

int list(int client_sock, int data_sock) 
{
    send_response(client_sock, "150 Here comes the directory listing.\r\n");

    FILE *fp = popen("ls", "r");
    if (fp)
    {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            send(data_sock, line, strlen(line), 0);
        }
        pclose(fp);
    }

    close(data_sock);
    send_response(client_sock, "226 Directory send OK.\r\n");

    return 0;
}

int stor(int client_sock, int data_sock, char *buffer) 
{
    char filename[BUFFER_SIZE];
    if (sscanf(buffer, "STOR %s", filename) == 1)
    {
        FILE *file = fopen(filename, "wb");
        if (file) 
        {
            send_response(client_sock, "150 File status okay; about to open data connection.\r\n");

            char data_buffer[BUFFER_SIZE];
            int bytes_received;
            while ((bytes_received = recv(data_sock, data_buffer, sizeof(data_buffer), 0)) > 0) 
            {
                fwrite(data_buffer, 1, bytes_received, file);
            }

            fclose(file);
            send_response(client_sock, "226 File transfer complete.\r\n");
        } 
        else 
        {
            send_response(client_sock, "550 Failed to open file.\r\n");
        }
    } 
    else
    {
        send_response(client_sock, "500 Syntax error in parameters or arguments.\r\n");
    }

    return 0;
}

int retr(int client_sock, int data_sock, char *buffer)
{
    char filename[BUFFER_SIZE];
    if (sscanf(buffer, "RETR %s", filename) == 1)
    {
        FILE *file = fopen(filename, "rb");
        if (file) 
        {
            send_response(client_sock, "150 File status okay; about to open data connection.\r\n");

            char data_buffer[BUFFER_SIZE];
            int bytes_read;
            while ((bytes_read = fread(data_buffer, 1, sizeof(data_buffer), file)) > 0)
            {
                send(data_sock, data_buffer, bytes_read, 0);
            }

            fclose(file);
            send_response(client_sock, "226 File transfer complete.\r\n");
        } 
        else
        {
            send_response(client_sock, "550 File not found.\r\n");
        }
    } 
    else
    {
        send_response(client_sock, "500 Syntax error in parameters or arguments.\r\n");
    }

    return 0;
}

void close_data_connection(int data_sock)
{
    if (data_sock != -1)
    {
        close(data_sock);
        data_trans_mode = NONE_MODE;
    }
}

void handle_client(int client_sock) 
{
    char buffer[BUFFER_SIZE] = { 0 };
    int quit_res = 0;
    struct sockaddr_in client_data_addr = { 0 };
    int pasv_listen_sock = -1;
    int data_sock = -1;         

    send_response(client_sock, "220 FTP Server Ready\r\n");

    while (!quit_res) 
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
            user(client_sock);
        } 
        else if (strncmp(buffer, "PASS ", 5) == 0) 
        {
            pass(client_sock);
        } 
        else if (strncmp(buffer, "QUIT", 4) == 0) 
        {
            quit_res = quit(client_sock);
        }
        else if (strncmp(buffer, "PORT ", 5) == 0) 
        {
            port(client_sock, buffer, &client_data_addr);
        } 
        else if (strncmp(buffer, "LIST", 4) == 0) 
        {
            data_sock = create_data_connection(client_sock, pasv_listen_sock, &client_data_addr);
            if (data_sock == -1)
                continue;

            list(client_sock, data_sock);
            close_data_connection(data_sock);
        }
        else if (strncmp(buffer, "STOR", 4) == 0) 
        {
            data_sock = create_data_connection(client_sock, pasv_listen_sock, &client_data_addr);
            if (data_sock == -1)
                continue;

            stor(client_sock, data_sock, buffer);
            close_data_connection(data_sock);
        } 
        else if (strncmp(buffer, "RETR", 4) == 0) 
        {
            data_sock = create_data_connection(client_sock, pasv_listen_sock, &client_data_addr);
            if (data_sock == -1)
                continue;

            retr(client_sock, data_sock, buffer);
            close_data_connection(data_sock);
        }
        else if (strncmp(buffer, "CWD", 3) == 0) 
        {
            cwd(client_sock, buffer);
        }
        else if (strncmp(buffer, "PWD", 3) == 0) 
        {
            pwd(client_sock);
        }
        else if (strncmp(buffer, "TYPE", 4) == 0) 
        {
            type(client_sock, buffer);
        }
        else if (strncmp(buffer, "PASV", 4) == 0) 
        {
            pasv_listen_sock = pasv(client_sock);
        }
        else 
        {
            send_response(client_sock, "500 Command not recognized.\r\n");
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
