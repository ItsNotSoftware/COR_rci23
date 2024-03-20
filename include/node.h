#ifndef NODE_H
#define NODE_H

#include "headers.h"

#define MSG_LEN 515

typedef struct {
    int fd;
    socklen_t addrlen;
    struct sockaddr_in addr;
    struct addrinfo *res;
    bool active;
} TcpConnection;

typedef struct {
    int id;
    char ip[STR_SIZE];
    char port[STR_SIZE];

    TcpConnection tcp;
} Node;

typedef struct {
    int ring;

    Node self;

    Node prev;
    Node next;
    Node second_next;

} MasterNode;

TcpConnection tcp_connection_create(const char *ip, const char *port);
void tcp_server_create(const char *ip, const char *port);
void connect_to_node(int id, char *ip, char *port, bool is_join);
bool tcp_receive_msg(TcpConnection *conn, char *msg);
void tcp_send_msg(TcpConnection *conn, char *msg);
void tcp_server_close();
void recieve_node();
void process_node_msg(Node *sender, char *msg);

#endif