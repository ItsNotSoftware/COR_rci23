#include "ui.h"

#include "node.h"
#include "server.h"

#define CHECK_BOUNDS(x, min, max)                   \
    if (x < min || x > max) {                       \
        printf("[Error]: %s out of bounds!\n", #x); \
        return;                                     \
    }

extern ServerInfo server;
extern MasterNode master_node;
extern ServerInfo server;

void command_join(int ring, int id) {
    CHECK_BOUNDS(ring, 0, 999);
    CHECK_BOUNDS(id, 0, 99);

    // Check nodes in ring
    char msg[BUFFER_SIZE] = {0};
    sprintf(msg, "NODES %d", ring);
    server_send_msg(&server, msg);

    char *answer = server_receive_msg(&server);

    char *token = strtok(answer, "\n");
    if (token == NULL) {
        perror("[ERROR]: Server did not respond!\n");
        return;
    }

    // Check if id is available
    bool id_not_available[100] = {0};
    int i = -1, ip1, ip2, ip3, ip4, port;

    while ((token = strtok(NULL, "\n")) != NULL) {
        // Extract the first number of squence
        if (sscanf(token, "%d %d.%d.%d.%d %d", &i, &ip1, &ip2, &ip3, &ip4, &port) == 6) {
            id_not_available[i] = true;
        }
    }

    if (id_not_available[id]) {
        for (int j = 0; j < 100; j++) {
            if (!id_not_available[j]) {
                id = j;
                break;
            }
        }
    }

    // Register node
    sprintf(msg, "REG %03d %02d %s %s", ring, id, master_node.self.ip, master_node.self.port);
    server_send_msg(&server, msg);

    answer = server_receive_msg(&server);

    master_node.self.id = id;
    master_node.ring = ring;

    master_node.prev = master_node.self;
    master_node.next = master_node.self;
    master_node.second_next = master_node.self;

    master_node.prev.tcp.active = false;
    master_node.next.tcp.active = false;
    master_node.second_next.tcp.active = false;

    if (i != -1)  // Nodes in ring
    {
        char new_ip[STR_SIZE] = {0};
        char new_port[STR_SIZE] = {0};

        sprintf(new_ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
        sprintf(new_port, "%d", port);

        connect_to_node(i, new_ip, new_port, true);
    }
}

void cmd_show_topology() {
    printf(
        "\n------------------ST----------------------\n"
        "| This node (%02d): \n"
        "|     Id -> %d \n"
        "|     Ip -> %s \n"
        "|     Port -> %s \n"
        "| Predecessor (%02d): \n"
        "|     Id -> %d \n"
        "|     Ip -> %s \n"
        "|     Port -> %s \n"
        "| Successor (%02d): \n"
        "|     Id -> %d \n"
        "|     Ip -> %s \n"
        "|     Port -> %s \n"
        "| Second Successor (%02d): \n"
        "|     Id -> %d \n"
        "|     Ip -> %s \n"
        "|     Port -> %s \n"
        "------------------END---------------------\n",
        master_node.self.id, master_node.self.id, master_node.self.ip, master_node.self.port,
        master_node.prev.id, master_node.prev.id, master_node.prev.ip, master_node.prev.port,
        master_node.next.id, master_node.next.id, master_node.next.ip, master_node.next.port,
        master_node.second_next.id, master_node.second_next.id, master_node.second_next.ip,
        master_node.second_next.port);
}

void process_command(int n_args, char args[5][256]) {
    if (strcmp(args[0], "j") == 0 && n_args == 3) {
        command_join(atoi(args[1]), atoi(args[2]));
    } else if (strcmp(args[0], "dj") == 0 && n_args == 5) {
    } else if (strcmp(args[0], "c") == 0 && n_args == 1) {
    } else if (strcmp(args[0], "rc") == 0 && n_args == 1) {
    } else if (strcmp(args[0], "st") == 0 && n_args == 1) {
        cmd_show_topology();
    } else if (strcmp(args[0], "sr") == 0 && n_args == 2) {
    } else if (strcmp(args[0], "sp") == 0 && n_args == 2) {
    } else if (strcmp(args[0], "sf") == 0 && n_args == 1) {
    } else if (strcmp(args[0], "m") == 0 && n_args == 3) {
    } else if (strcmp(args[0], "l") == 0 && n_args == 1) {
        server_disconnect(&server);
    } else if (strcmp(args[0], "x") == 0 && n_args == 1) {
        server_disconnect(&server);
        exit(0);
    } else {
        printf("Invalid command\n");
    }
}

void read_console() {
    char buffer[BUFFER_SIZE] = {0};
    char args[5][STR_SIZE] = {0};
    fgets(buffer, sizeof(buffer), stdin);
    fflush(stdin);
    buffer[strlen(buffer) - 1] = '\0';  // Remove '\n' from buffer

    int n_args = 0;
    char *token = strtok(buffer, " ");
    while (token != NULL) {
        strcpy(args[n_args++], token);
        token = strtok(NULL, " ");
    }

    process_command(n_args, args);
}
