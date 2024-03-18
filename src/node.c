#include "node.h"

#include "fd_handler.h"

extern MasterNode master_node;
int tcp_server_fd;

int string_to_args(char *str, char args[4][STR_SIZE]) {
    int n_args = 0;
    char *token = strtok(str, " ");
    while (token != NULL) {
        strcpy(args[n_args++], token);
        token = strtok(NULL, " ");
    }

    return n_args;
}

void tcp_server_create(const char *ip, const char *port) {
    TcpConnection *server = &master_node.self.tcp;
    int errcode;

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    tcp_server_fd = server->fd;

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
}

void tcp_server_close() { close(tcp_server_fd); }

TcpConnection tcp_connection_create(const char *ip, const char *port) {
    int errcode;
    TcpConnection new = {0};

    new.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (new.fd < 0) {
        perror("[ERROR]: Unable to create TCP connection");
        exit(1);
    }

    fd_add(new.fd);
    new.addr.sin_family = AF_INET;
    new.addr.sin_port = htons(atoi(port));
    new.addr.sin_addr.s_addr = inet_addr(ip);

    errcode = connect(new.fd, (struct sockaddr *)&new.addr, sizeof(new.addr));
    if (errcode < 0) {
        perror("[ERROR]: Unable to connect to TCP server");
        exit(1);
    }
    new.active = true;

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
}

void tcp_send_msg(TcpConnection *conn, char *msg) {
    printf("\n\t[Sending]: %s\n", msg);

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
        n = read(conn->fd, msg, STR_SIZE);

        if (n == 0) {
            WARNING("Connection closed by peer");
            conn->active = false;
            fd_remove(conn->fd);
            close(conn->fd);
            return;
        } else if (n < 0) {
            ERROR("Unable to read from socket");
            exit(1);
        }

        total_bytes_recieved += n;

        if (msg[total_bytes_recieved - 1] == '\n') {
            msg[total_bytes_recieved - 1] = '\0';

            printf("\n\t[Recieved]: %s\n", msg);
            return;
        }
    }
}

void connect_to_node(int id, char *ip, char *port, bool is_join) {
    char msg[STR_SIZE] = {0};
    Node *next = &master_node.next;
    Node *prev = &master_node.prev;

    // Close connection prev next comnnection to handle new node
    if (next->id != master_node.self.id && next->id != prev->id) {
        close(next->tcp.fd);
        fd_remove(next->tcp.fd);
    }

    next->tcp = tcp_connection_create(ip, port);
    next->id = id;
    strcpy(next->ip, ip);
    strcpy(next->port, port);

    // Make node my successor
    master_node.next.id = id;
    strcpy(master_node.next.ip, ip);
    strcpy(master_node.next.port, port);
    master_node.next.tcp.active = true;

    if (is_join) {
        sprintf(msg, "ENTRY %02d %s %s", master_node.self.id, master_node.self.ip,
                master_node.self.port);
        tcp_send_msg(&next->tcp, msg);

        // Make node my predecessor
        master_node.prev = master_node.next;
    } else {
        sprintf(msg, "PRED %02d", master_node.self.id);
        tcp_send_msg(&next->tcp, msg);
    }
}

void recieve_node() {
    char msg[STR_SIZE] = {0};
    char args[5][STR_SIZE] = {0};

    Node *prev = &master_node.prev;
    Node *next = &master_node.next;
    TcpConnection new_conn = {0};

    if (prev->id != master_node.self.id && prev->id != next->id) {
        close(prev->tcp.fd);
        fd_remove(prev->tcp.fd);
    }

    tcp_connection_accept(&new_conn);
    tcp_receive_msg(&new_conn, msg);
    string_to_args(msg, args);

    if (strcmp(args[0], "ENTRY") == 0) {
        bool node_alone = next->id == master_node.self.id && prev->id == master_node.self.id;

        // Inform predecessor of new node
        if (!node_alone) {
            sprintf(msg, "ENTRY %02d %s %s", atoi(args[1]), args[2], args[3]);
            tcp_send_msg(&prev->tcp, msg);
        }

        // Update Pred to new node
        prev->id = atoi(args[1]);
        strcpy(prev->ip, args[2]);
        strcpy(prev->port, args[3]);
        prev->tcp.active = true;
        prev->tcp = new_conn;

        // Update new node secomd sucessor
        if (!node_alone) {
            sprintf(msg, "SUCC %02d %s %s", next->id, next->ip, next->port);
            tcp_send_msg(&prev->tcp, msg);
        }

        // Update Succ to new node if this node was alone in the net
        if (node_alone) {
            *next = *prev;

            // update new node predecessor to myself
            sprintf(msg, "PRED %02d %s %s", master_node.self.id, master_node.self.ip,
                    master_node.self.port);
            tcp_send_msg(&next->tcp, msg);
        }
    } else if (strcmp(args[0], "PRED") == 0) {
        prev->id = atoi(args[1]);
        prev->tcp.active = true;
    }
}

void process_node_msg(Node *sender, char *msg) {
    char args[4][STR_SIZE] = {0};  // msg1
    char *msg1 = NULL, *msg2 = NULL;

    // Split message if 2 messages come together
    char *divider = strchr(msg, '\n');

    if (divider != NULL) {
        *divider = '\0';
        msg2 = divider + 1;
    }
    msg1 = msg;

    // Process one message at a time
    for (msg = msg1; msg != msg2; msg = msg2) {
        if (msg == NULL) return;  // No message to process

        int n_args = string_to_args(msg, args);
        if (n_args == 0) return;

        if (strcmp(args[0], "SUCC") == 0) {
            fflush(stdout);

            master_node.second_next.id = atoi(args[1]);
            strcpy(master_node.second_next.ip, args[2]);
            strcpy(master_node.second_next.port, args[3]);

        } else if (strcmp(args[0], "PRED") == 0) {
            master_node.prev.id = atoi(args[1]);
            strcpy(master_node.prev.ip, "_");
            strcpy(master_node.prev.port, "_");

        } else if (strcmp(args[0], "ENTRY") == 0) {
            TcpConnection prev_backup = master_node.prev.tcp;

            connect_to_node(atoi(args[1]), args[2], args[3], false);

            // Inform new node of me
            sprintf(msg, "PRED %02d", master_node.self.id);
            tcp_send_msg(&master_node.next.tcp, msg);

            // Inform predecessor of new node
            sprintf(msg, "SUCC %02d %s %s", master_node.next.id, master_node.next.ip,
                    master_node.next.port);
            tcp_send_msg(&prev_backup, msg);
        }
    }
}
