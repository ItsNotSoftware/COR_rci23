#include "fd_handler.h"
#include "headers.h"
#include "node.h"
#include "server.h"
#include "ui.h"

MasterNode master_node;
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
            printf("i: %d, ", i);
            if (fd_is_set(i)) {
                printf("fd: %d\n", i);
                if (fd_is_user_cmd(i)) {
                    read_console();
                } else if (fd_is_new_connection(i)) {
                    printf("New connection\n");
                    recieve_node();
                } else  // A Node is sending message
                {
                    char msg[STR_SIZE] = {0};

                    Node *node = fd_to_node(i);
                    if (node == NULL) {
                        continue;
                    }

                    tcp_receive_msg(&node->tcp, msg);
                    printf("Message from %s: %s\n", node->ip, msg);

                    int id, ip1, ip2, ip3, ip4, port;
                    sscanf(msg, "SUCC %d %d.%d.%d.%d %d", &id, &ip1, &ip2, &ip3, &ip4, &port);
                    master_node.second_next.id = id;
                    sprintf(master_node.second_next.ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
                    sprintf(master_node.second_next.port, "%d", port);
                }
            }
        }
    }

    // tpc_server_close(master_node.self.tcp);
    return 0;
}