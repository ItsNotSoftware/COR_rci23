#include "ui.h"

#include "fd_handler.h"
#include "forwarding_tables.h"
#include "node.h"
#include "server.h"

extern int n_chords;

#define CHECK_BOUNDS(x, min, max)                   \
    if (x < min || x > max) {                       \
        printf("[Error]: %s out of bounds!\n", #x); \
        return;                                     \
    }

extern ServerInfo server;
extern MasterNode master_node;
extern ServerInfo server;

void cmd_join(int ring, int id) {
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

    tables_init();

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
        "|     Fd -> %d \n"
        "| Predecessor (%02d): \n"
        "|     Id -> %d \n"
        "|     Ip -> %s \n"
        "|     Port -> %s \n"
        "|     Fd -> %d \n"
        "| Successor (%02d): \n"
        "|     Id -> %d \n"
        "|     Ip -> %s \n"
        "|     Port -> %s \n"
        "|     Fd -> %d \n"
        "| Second Successor (%02d): \n"
        "|     Id -> %d \n"
        "|     Ip -> %s \n"
        "|     Port -> %s \n"
        "|\n"
        "| (%d) -> [%d] -> (%d) -> (%d) \n"
        "|\n",
        master_node.self.id, master_node.self.id, master_node.self.ip, master_node.self.port,
        master_node.self.tcp.fd, master_node.prev.id, master_node.prev.id, master_node.prev.ip,
        master_node.prev.port, master_node.prev.tcp.fd, master_node.next.id, master_node.next.id,
        master_node.next.ip, master_node.next.port, master_node.next.tcp.fd,
        master_node.second_next.id, master_node.second_next.id, master_node.second_next.ip,
        master_node.second_next.port, master_node.prev.id, master_node.self.id, master_node.next.id,
        master_node.second_next.id);
    printf("| Chords:\n");

    if (master_node.owned_chord.id != -1) {
        printf("|     Owned -> %d\n", master_node.owned_chord.id);
    } else {
        printf("|     Owned-> -\n");
    }

    printf("|     Connected -> ");
    for (int i = 0; i < n_chords; i++) {
        printf("%d ", master_node.chords[i].id);
    }

    printf("\n------------------END---------------------\n");
}

void cmd_chord() {
    // Check nodes in ring
    char msg[BUFFER_SIZE] = {0};
    char ip[20], port_s[20];
    sprintf(msg, "NODES %d", master_node.ring);
    server_send_msg(&server, msg);

    char *answer = server_receive_msg(&server);

    char *token = strtok(answer, "\n");
    if (token == NULL) {
        perror("[ERROR]: Server did not respond!\n");
        return;
    }

    // Check if ids on ring
    int id, ip1, ip2, ip3, ip4, port;

    while ((token = strtok(NULL, "\n")) != NULL) {
        // Extract the first number of squence
        if (sscanf(token, "%d %d.%d.%d.%d %d", &id, &ip1, &ip2, &ip3, &ip4, &port) == 6) {
            bool can_join =
                id != master_node.self.id && id != master_node.prev.id && id != master_node.next.id;

            if (can_join) {
                sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
                sprintf(port_s, "%d", port);

                create_chord(id, ip, port_s);

                printf("Chord created with %d\n", id);
                return;
            }
        }
    }

    printf("No available nodes to create chord\n");
}

void cmd_show_path(int dest) {
    int *path = get_shortest_path(dest);

    if (path[0] == NONE) {
        printf("\nNo path found\n\n");
        return;
    }

    printf("\nPath to %d: ", dest);
    for (int i = 0; i < MAX_NUMBER_OF_NODES; i++) {
        if (path[i] == NONE) {
            break;
        }

        printf("%d -> ", path[i]);
    }
    printf("\n\n");
}

void process_command(int n_args, char args[5][256]) {
    if (strcmp(args[0], "j") == 0 && n_args == 3) {
        cmd_join(atoi(args[1]), atoi(args[2]));

    } else if (strcmp(args[0], "dj") == 0 && n_args == 5) {
    } else if (strcmp(args[0], "c") == 0 && n_args == 1) {
        if (master_node.owned_chord.id != -1) {
            printf("Already have a chord\n");
            return;
        }

        cmd_chord();

    } else if (strcmp(args[0], "rc") == 0 && n_args == 1) {
        if (master_node.owned_chord.id == -1) {
            printf("No chord to remove\n");
            return;
        }

        fd_remove(master_node.owned_chord.tcp.fd);
        close(master_node.owned_chord.tcp.fd);
        master_node.owned_chord.id = -1;

    } else if (strcmp(args[0], "st") == 0 && n_args == 1) {
        cmd_show_topology();

    } else if (strcmp(args[0], "sr") == 0 && n_args == 2) {
    } else if (strcmp(args[0], "sp") == 0 && n_args == 2) {
        cmd_show_path(atoi(args[1]));
    } else if (strcmp(args[0], "sf") == 0 && n_args == 1) {
    } else if (strcmp(args[0], "m") == 0 && n_args == 3) {
    } else if (strcmp(args[0], "l") == 0 && n_args == 1) {
        server_disconnect(&server);

        if (master_node.next.id != master_node.self.id) {
            DEBUG("Closing next");
            close(master_node.next.tcp.fd);
            fd_remove(master_node.next.tcp.fd);
        }

        if (master_node.prev.id != master_node.self.id) {
            DEBUG("Closing prev");
            close(master_node.prev.tcp.fd);
            fd_remove(master_node.prev.tcp.fd);
        }

        if (master_node.owned_chord.id != -1) {
            close(master_node.owned_chord.tcp.fd);
            fd_remove(master_node.owned_chord.tcp.fd);
        }

        for (int i = 0; i < n_chords; i++) {
            close(master_node.chords[i].tcp.fd);
            fd_remove(master_node.chords[i].tcp.fd);
        }

        master_node.next = master_node.self;
        master_node.prev = master_node.self;
        master_node.second_next = master_node.self;

        master_node.owned_chord.id = -1;
        server = server_connect();
        fd_handler_init();
        fd_add(STDIN_FILENO);

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
