#include "fd_handler.h"
#include "forwarding_tables.h"
#include "headers.h"
#include "node.h"
#include "server.h"
#include "ui.h"

MasterNode master_node = {0};
ServerInfo server;

extern int n_chords;

char reg_ip[STR_SIZE];
char reg_port[STR_SIZE];

Node *fd_to_node(int fd) {
    if (fd == master_node.self.tcp.fd) {
        return &master_node.self;
    } else if (fd == master_node.prev.tcp.fd) {
        return &master_node.prev;
    } else if (fd == master_node.next.tcp.fd) {
        return &master_node.next;
    } else if (fd == master_node.second_next.tcp.fd) {
        return &master_node.second_next;
    } else if (fd == master_node.owned_chord.tcp.fd) {
        return &master_node.owned_chord;
    } else {
        for (int i = 0; i < n_chords; i++) {
            if (fd == master_node.chords[i].tcp.fd) {
                return &master_node.chords[i];
            }
        }
    }

    return NULL;
}

void process_argv(int argc, char **argv) {
    if (argc == 5) {
        strcpy(master_node.self.ip, argv[1]);
        strcpy(master_node.self.port, argv[2]);

        strcpy(reg_ip, argv[3]);
        strcpy(reg_port, argv[4]);
    } else if (argc == 3) {
        strcpy(master_node.self.ip, argv[1]);
        strcpy(master_node.self.port, argv[2]);
        strcpy(reg_ip, SERVER_IP);
        strcpy(reg_port, SERVER_PORT);
    } else {
        perror("[Error]: Invalid number of arguments\n");
    }
}

int main(int argc, char **argv) {
    process_argv(argc, argv);
    server = server_connect();
    master_node.owned_chord.id = -1;  // not_set

    // Adding fd's
    fd_handler_init();
    fd_add(STDIN_FILENO);

    // Servcer to accept new connections
    tcp_server_create(master_node.self.ip, master_node.self.port);

    while (true) {
        printf("Enter command: ");
        fflush(stdout);

        fd_handler_select();
        for (int i = 0; i <= get_max_fd(); i++) {
            if (fd_is_set(i)) {
                if (fd_is_user_cmd(i)) {
                    read_console();
                } else if (fd_is_new_connection(i)) {
                    INFO("New connection");
                    recieve_node();
                } else  // A Node is sending message
                {
                    char msg[STR_SIZE] = {0};

                    Node *node = fd_to_node(i);
                    if (node == NULL) {
                        WARNING("Invalid file descriptor");
                        fd_remove(i);
                        continue;
                    }

                    if (!tcp_receive_msg(&node->tcp, msg)) {  // Node disconnected
                        fd_remove(node->tcp.fd);
                        close(node->tcp.fd);

                        if (node->id == master_node.next.id) {  // is next
                            // If node is alone
                            if (master_node.next.id == master_node.prev.id) {
                                master_node.next = master_node.self;
                                master_node.second_next = master_node.self;
                                master_node.prev = master_node.self;
                                continue;
                            }

                            // If there is just 2 nodes on the net
                            if (master_node.second_next.id == master_node.prev.id) {
                                master_node.next = master_node.prev;
                                master_node.second_next = master_node.self;

                                sprintf(msg, "SUCC %02d %s %s", master_node.next.id,
                                        master_node.next.ip, master_node.next.port);
                                tcp_send_msg(&master_node.next.tcp, msg);

                                sprintf(msg, "PRED %02d", master_node.self.id);
                                tcp_send_msg(&master_node.next.tcp, msg);

                                continue;
                            }

                            connect_to_node(master_node.second_next.id, master_node.second_next.ip,
                                            master_node.second_next.port, false);

                        } else if (node->id == master_node.prev.id) {  // is prev
                            master_node.prev = master_node.next;
                            continue;
                        }

                        if (is_chord(node->id)) {
                            chord_remove(node->id);
                            continue;
                        }
                    } else {
                        process_node_msg(node, msg);
                    }
                }
            }
        }
    }

    return 0;
}