#include "fd_handler.h"
#include "headers.h"
#include "node.h"
#include "server.h"
#include "ui.h"

MasterNode master_node = {0};
ServerInfo server;

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
    } else {
        return NULL;
    }
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
                        if (node->id == master_node.next.id) {
                            connect_to_node(master_node.second_next.id, master_node.second_next.ip,
                                            master_node.second_next.port, false);
                        } else {
                            fd_remove(node->tcp.fd);
                        }
                        continue;
                    }

                    process_node_msg(node, msg);
                }
            }
        }
    }

    return 0;
}