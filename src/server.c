#include "server.h"

extern char reg_ip[STR_SIZE];
extern char reg_port[STR_SIZE];

ServerInfo server_connect()
{
    ServerInfo info = {0};
    int errcode;

    info.fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (info.fd == -1)
    {
        perror("[ERROR]: Could not create socket!");
        exit(EXIT_FAILURE);
    }

    memset(&info.hints, 0, sizeof(info.hints));
    info.hints.ai_family = AF_INET;      // IPv4
    info.hints.ai_socktype = SOCK_DGRAM; // Datagram socket

    errcode = getaddrinfo(reg_ip, reg_port, &info.hints, &info.res);
    if (errcode != 0)
    {
        fprintf(stderr, "[ERROR]: Could not get address info");
        close(info.fd);
        exit(EXIT_FAILURE);
    }

    info.addrlen = info.res->ai_addrlen;

    return info;
}

void server_send_msg(ServerInfo *info, const char *msg)
{
    int n = sendto(info->fd, msg, strlen(msg), 0, info->res->ai_addr, info->res->ai_addrlen);
    if (n == -1)
    {
        perror("[ERROR]: Could not send message to server");
        freeaddrinfo(info->res);
        close(info->fd);
        exit(EXIT_FAILURE);
    }
}

char *server_receive_msg(ServerInfo *info)
{
    int n = recvfrom(info->fd, info->buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (n == -1)
    {
        perror("[ERROR]: Could not receive message from server");
        freeaddrinfo(info->res);
        close(info->fd);
        exit(EXIT_FAILURE);
    }
    info->buffer[n] = '\0';
    return info->buffer;
}

void server_disconnect(const ServerInfo *info)
{
    freeaddrinfo(info->res);
    close(info->fd);
}
