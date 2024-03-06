#ifndef SERVER_H
#define SERVER_H

#include "headers.h"

#define SERVER_PORT "59000"
#define SERVER_IP "193.136.138.142"

typedef struct {
    int fd;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
} ServerInfo;

ServerInfo server_connect();
void server_send_msg(ServerInfo *info, const char *msg);
char *server_receive_msg(ServerInfo *info);
// void server_disconnect();

#endif