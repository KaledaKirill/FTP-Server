#include "save_socket.h"

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>

int socket_s(int domain, int type, int protocol)
{
    int res = socket(domain, type, protocol);
    if (res == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    return res;
}

int bind_s(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int res = bind(sockfd, addr, addrlen);
    if (res == -1)
    {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return res;
}

int listen_s(int sockfd, int backlog)
{
    int res = listen(sockfd, backlog);
    if (res == -1)
    {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return res;
}


int accept_s(int sockfd, struct sockaddr *addr , socklen_t *addrlen)
{
    int res = accept(sockfd, addr, addrlen);
    if (res == -1)
    {
        perror("Accept failed");
        close(sockfd);
        exit(EXIT_FAILURE); 
    }

    return res;
}