#include "node.h"

#include "fd_handler.h"

extern MasterNode master_node;
int tcp_server_fd;

void tcp_server_create(const char *ip, const char *port) {
    TcpConnection *server = &master_node.self.tcp;
    int errcode;

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    tcp_server_fd = server->fd;
    printf("tcp_server_fd: %d\n", tcp_server_fd);

    if (tcp_server_fd < 0) {
        perror("[ERROR]: Unable to create TCP server");
        exit(1);
    }

    server->addr.sin_family = AF_INET;
    server->addr.sin_addr.s_addr = inet_addr(ip);
    server->addr.sin_port = htons(atoi(port));

    errcode = bind(tcp_server_fd, (struct sockaddr *)&server->addr, sizeof(server->addr));
    if (errcode < 0) {
        perror("[ERROR]: Unable to bind TCP server");
        exit(1);
    }

    errcode = listen(tcp_server_fd, MAX_NUMBER_OF_NODES);
    if (errcode < 0) {
        perror("[ERROR]: Unable to listen TCP server");
        exit(1);
    }
    fd_add(tcp_server_fd);

    printf("[INFO]: TCP server created successfully\n");
}

void tcp_server_close() {
    close(tcp_server_fd);
    printf("[INFO]: TCP server closed successfully\n");
}

TcpConnection tcp_connection_create(const char *ip, const char *port) {
    int errcode;
    TcpConnection new = {0};

    new.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (new.fd < 0) {
        perror("[ERROR]: Unable to create TCP connection");
        exit(1);
    }

    new.addr.sin_family = AF_INET;
    new.addr.sin_port = htons(atoi(port));
    new.addr.sin_addr.s_addr = inet_addr(ip);

    errcode = connect(new.fd, (struct sockaddr *)&new.addr, sizeof(new.addr));
    if (errcode < 0) {
        perror("[ERROR]: Unable to connect to TCP server");
        exit(1);
    }
    fd_add(new.fd);
    new.active = true;

    printf("[INFO]: TCP connection %s %s \n", ip, port);

    return new;
}

void tcp_connection_accept(TcpConnection *conn) {
    int addr_len = sizeof(conn->addr);
    conn->fd = accept(tcp_server_fd, (struct sockaddr *)&conn->addr, (socklen_t *)&addr_len);
    if (conn->fd < 0) {
        perror("[ERROR]: Unable to accept TCP connection");
        exit(1);
    }

    fd_add(conn->fd);
    conn->active = true;

    printf("[INFO]: TCP connection accepted successfully\n");
}

void tcp_send_msg(TcpConnection *conn, char *msg) {
    int n = 0;
    int nleft = strlen(msg);
    msg[nleft++] = '\n';

    while (nleft > 0) {
        n = write(conn->fd, msg, strlen(msg));
        if (n < 0) {
            perror("[ERROR]: Unable to send message");
            exit(1);
        }

        nleft -= n;
        msg += n;
    }
}

void tcp_receive_msg(TcpConnection *conn, char *msg) {
    int n, total_bytes_recieved = 0;

    while (1) {
        n = read(conn->fd, msg, sizeof(STR_SIZE));

        if (n == 0) {
            printf("[INFO]: Connection closed by node\n");
            conn->active = false;
            fd_remove(conn->fd);
            close(conn->fd);
            break;
        } else if (n < 0) {
            perror("[ERROR]: Unable to read message");
            exit(1);
        }

        total_bytes_recieved += n;

        if (msg[total_bytes_recieved - 1] == '\n') {
            msg[total_bytes_recieved - 1] = '\0';
            return;
        }
    }
}

void connect_to_node(int id, char *ip, char *port) {
    Node *next = &master_node.next;

    next->tcp = tcp_connection_create(ip, port);
    next->id = id;
    strcpy(next->ip, ip);
    strcpy(next->port, port);

    char msg[STR_SIZE] = {0};
    sprintf(msg, "ENTRY %02d %s %s", master_node.self.id, master_node.self.ip,
            master_node.self.port);
    tcp_send_msg(&next->tcp, msg);
    printf("Sent: %s\n", msg);
}

void recieve_node() {
    char msg[STR_SIZE] = {0};
    Node *prev = &master_node.prev;
    Node *next = &master_node.next;

    tcp_connection_accept(&prev->tcp);
    tcp_receive_msg(&prev->tcp, msg);
    printf("Recieved: %s\n", msg);

    int id, ip1, ip2, ip3, ip4, port;
    sscanf(msg, "ENTRY %02d %d.%d.%d.%d %d", &id, &ip1, &ip2, &ip3, &ip4, &port);

    prev->id = id;
    sprintf(prev->ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    sprintf(prev->port, "%d", port);
    prev->tcp.active = true;

    printf("[INFO]: Node %d connected\n", prev->id);

    sprintf(msg, "SUCC %02d %s %s", next->id, next->ip, next->port);
    tcp_send_msg(&prev->tcp, msg);

    // TODO: implement complete logic
    next->id = id;
    next->tcp = prev->tcp;
    strcpy(next->ip, prev->ip);
    strcpy(next->port, prev->port);
    next->tcp.active = true;
}