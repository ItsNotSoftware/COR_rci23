#include "node.h"

#include "fd_handler.h"
#include "forwarding_tables.h"

int n_chords = 0;

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

bool tcp_receive_msg(TcpConnection *conn, char *msg) {
    int n, total_bytes_recieved = 0;

    while (1) {
        n = read(conn->fd, msg, STR_SIZE);

        if (n == 0) {
            WARNING("Connection closed by peer");
            return false;
        } else if (n < 0) {
            ERROR("Unable to read from socket");
            return false;
        }

        total_bytes_recieved += n;

        if (msg[total_bytes_recieved - 1] == '\n') {
            msg[total_bytes_recieved - 1] = '\0';

            printf("\n\t[Recieved]: %s\n", msg);
            return true;
        }
    }
}

void create_chord(int id, char *ip, char *port) {
    char msg[STR_SIZE] = {0};
    Node *chord = &master_node.owned_chord;

    chord->id = id;
    strcpy(chord->ip, ip);
    strcpy(chord->port, port);
    chord->tcp = tcp_connection_create(ip, port);
    chord->tcp.active = true;

    sprintf(msg, "CHORD %02d", master_node.self.id);
    tcp_send_msg(&chord->tcp, msg);

    shortest_path_table_push(chord->id, chord->id);
    send_shortest_path_table(chord);
}

void connect_to_node(int id, char *ip, char *port, bool is_join) {
    char msg[STR_SIZE] = {0};
    Node *next = &master_node.next;
    Node *prev = &master_node.prev;

    // Just 2 nodes on the net
    if (id == master_node.prev.id) {
        master_node.next = master_node.prev;
        return;
    }

    // Close connection prev next comnnection to handle new node
    if (next->id != master_node.self.id && next->id != prev->id) {
        close(next->tcp.fd);
        fd_remove(next->tcp.fd);
    }

    next->tcp = tcp_connection_create(ip, port);
    next->id = id;
    strcpy(next->ip, ip);
    strcpy(next->port, port);
    next->tcp.active = true;

    if (is_join) {
        sprintf(msg, "ENTRY %02d %s %s", master_node.self.id, master_node.self.ip,
                master_node.self.port);
        tcp_send_msg(&next->tcp, msg);

        // Make node my predecessor
        master_node.prev = master_node.next;
    } else {
        sprintf(msg, "PRED %02d", master_node.self.id);
        tcp_send_msg(&next->tcp, msg);

        sprintf(msg, "SUCC %02d %s %s", master_node.next.id, master_node.next.ip,
                master_node.next.port);
        tcp_send_msg(&prev->tcp, msg);
    }

    shortest_path_table_push(next->id, next->id);
    send_shortest_path_table(next);
}

void recieve_node() {
    char msg[STR_SIZE] = {0};
    char args[5][STR_SIZE] = {0};

    Node *prev = &master_node.prev;
    Node *next = &master_node.next;

    TcpConnection new_conn = {0};

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

        // Close previous connection
        if (prev->id != master_node.self.id && prev->id != next->id) {
            close(prev->tcp.fd);
            fd_remove(prev->tcp.fd);
            *prev = master_node.self;
        }

        // Update Pred to new node
        prev->id = atoi(args[1]);
        strcpy(prev->ip, args[2]);
        strcpy(prev->port, args[3]);
        prev->tcp.active = true;
        prev->tcp = new_conn;

        if (node_alone) master_node.next = *prev;

        // Update new node secomd sucessor
        if (!node_alone) {
            sprintf(msg, "SUCC %02d %s %s", next->id, next->ip, next->port);
            tcp_send_msg(&prev->tcp, msg);
        }

    } else if (strcmp(args[0], "PRED") == 0) {
        // Close previous connection
        if (prev->id != master_node.self.id && prev->id != next->id) {
            close(prev->tcp.fd);
            fd_remove(prev->tcp.fd);
            *prev = master_node.self;
        }

        prev->id = atoi(args[1]);
        prev->tcp.active = true;
        prev->tcp = new_conn;

        sprintf(msg, "SUCC %02d %s %s", master_node.next.id, master_node.next.ip,
                master_node.next.port);
        tcp_send_msg(&master_node.prev.tcp, msg);
    } else if (strcmp(args[0], "CHORD") == 0) {
        Node n = {.id = atoi(args[1]), .ip = "-", .port = "-", .tcp = new_conn};
        chord_push(n);
    }

    shortest_path_table_push(prev->id, prev->id);
    send_shortest_path_table(prev);
}

void process_node_msg(Node *sender, char *msg) {
    int n_args = 0;
    char args[4][STR_SIZE] = {0};  // msg1
    char *msg2 = NULL;

    // Split message if 2 messages come together
    char *divider = strchr(msg, '\n');

    if (divider != NULL) {
        *divider = '\0';
        msg2 = divider + 1;
    }

    n_args = string_to_args(msg, args);
    if (n_args == 0) return;

    if (strcmp(args[0], "SUCC") == 0) {
        master_node.second_next.id = atoi(args[1]);
        strcpy(master_node.second_next.ip, args[2]);
        strcpy(master_node.second_next.port, args[3]);
        master_node.second_next.tcp.fd = -1;

    } else if (strcmp(args[0], "PRED") == 0) {
        master_node.prev.id = atoi(args[1]);
        strcpy(master_node.prev.ip, "-");
        strcpy(master_node.prev.port, "-");
        master_node.prev = *sender;

        sprintf(msg, "SUCC %02d %s %s", master_node.next.id, master_node.next.ip,
                master_node.next.port);
        tcp_send_msg(&master_node.prev.tcp, msg);

    } else if (strcmp(args[0], "ENTRY") == 0) {
        Node next_backup = master_node.next;
        connect_to_node(atoi(args[1]), args[2], args[3], false);
        master_node.second_next = next_backup;
    } else if (strcmp(args[0], "ROUTE") == 0) {
        update_shotest_path(atoi(args[2]), args[3]);
    }

    if (msg2 != NULL) process_node_msg(sender, msg2);  //  next message
}

void chord_push(Node node) { master_node.chords[n_chords++] = node; }

void chord_remove(int id) {
    if (master_node.owned_chord.id == id) {
        master_node.owned_chord.id = -1;
        return;
    }

    for (int i = 0; i < n_chords; i++) {
        if (master_node.chords[i].id == id) {
            // Shift all elements to left
            for (int j = i; j < n_chords - 1; j++) {
                master_node.chords[j] = master_node.chords[j + 1];
            }
            n_chords--;
            break;
        }
    }
}

bool is_chord(int id) {
    if (master_node.owned_chord.id == id) return true;

    for (int i = 0; i < n_chords; i++) {
        if (master_node.chords[i].id == id) return true;
    }
    return false;
}