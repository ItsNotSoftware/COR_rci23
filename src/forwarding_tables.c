#include "forwarding_tables.h"

extern MasterNode master_node;

static int forwarding_table[100][100][MAX_NUMBER_OF_NODES];
static int shortest_path_table[100][MAX_NUMBER_OF_NODES];

/*
 * Get the last element of the array
 *
 * @param arr: the array to get the last element from
 *
 * @return: the last element of the array
 */
int *get_last_element(int *arr) {
    for (int *i = arr; i < arr + 100; i++) {
        if (*i == NONE) {
            return i;
        }
    }
    return NULL;
}
/*
 * Initializes the forwarding tables
 */
void tables_init() {
    memset(forwarding_table, NONE, sizeof(forwarding_table));
    memset(shortest_path_table, NONE, sizeof(shortest_path_table));

    forwarding_table[master_node.self.id][master_node.self.id][0] = master_node.self.id;
    shortest_path_table[master_node.self.id][0] = master_node.self.id;
}

/*
 * Resets the forwarding table entry
 *
 * @param dest: the destination node
 */
void forwarding_table_reset_entry(int dest, int src) {
    memset(forwarding_table[dest][src], NONE, sizeof(forwarding_table[dest][src]));
}

/*
 * Resets the shortest path table entry
 *
 * @param dest: the destination node
 */
void shortest_path_table_reset_entry(int dest) {
    memset(shortest_path_table[dest], NONE, sizeof(shortest_path_table[dest]));
}

/*
 * Gets the forwarding table
 *
 * @return: the forwarding table
 */
int ***get_forwarding_table() { return (int ***)forwarding_table; }

/*
 * Gets the shortest path table
 *
 * @return: the shortest path table
 */
int **get_shortest_path_table() { return (int **)shortest_path_table; }

/*
 * Gets the forwarding table entry
 *
 * @param dest: the destination node
 * @param src: the source node
 *
 * @return: the forwarding table entry
 */
int *get_forwarding(int dest, int src) { return forwarding_table[dest][src]; }

/*
 * Gets the shortest path table entry
 *
 * @param dest: the destination node
 *
 * @return: the shortest path table entry
 */
int *get_shortest_path(int dest) { return shortest_path_table[dest]; }

/*
 * Gets the first step of the shortest path
 *
 * @param dest: the destination node
 *
 * @return: the first step of the shortest path
 */
int get_first_step(int dest) { return shortest_path_table[dest][1]; }

/*
 * Updates the shortest paths
 */
void update_shortest_paths() {
    for (int dest = 0; dest < 100; dest++) {
        int new_path[MAX_NUMBER_OF_NODES];
        int shortest = 1000;
        bool updated = false;

        for (int src = 0; src < 100; src++) {
            if (forwarding_table[dest][src][0] == NONE) {
                continue;
            }

            int i;
            // Walk through the path
            for (i = 0; i < MAX_NUMBER_OF_NODES; i++) {
                if (forwarding_table[dest][src][i] == NONE) {
                    break;
                }

                new_path[i] = forwarding_table[dest][src][i];
            }

            // Update the shortest path
            if (i > 0 && i < shortest) {
                updated = true;
                shortest = i;
                memcpy(shortest_path_table[dest], new_path, sizeof(shortest_path_table[dest]));
                shortest_path_table[dest][i] = NONE;
            }
        }

        if (!updated) {
            memset(shortest_path_table[dest], NONE, sizeof(shortest_path_table[dest]));
        }
    }
}

/*
 * Updates the shortest paths
 *
 * @param dest: the destination node
 * @param src: the source node
 */
void forwarding_table_add_direct_conn(int dest, int src) {
    forwarding_table[dest][src][0] = master_node.self.id;
    forwarding_table[dest][src][1] = src;
    forwarding_table[dest][src][2] = NONE;

    shortest_path_table[dest][0] = master_node.self.id;
    shortest_path_table[dest][1] = dest;
    shortest_path_table[dest][2] = NONE;
}

/*
 * Updates the shortest paths
 *
 * @param dest: the destination node
 * @param src: the source node
 */
void send_shortest_path_table(Node *dest) {
    // Iterate over each line
    for (int i = 0; i < 100; i++) {
        if (shortest_path_table[i][0] == NONE) {
            continue;
        }

        char msg[STR_SIZE] = {0};
        sprintf(msg, "ROUTE %d %d ", master_node.self.id, i);

        // Iterate over each node in the line
        for (int j = 0; j < MAX_NUMBER_OF_NODES; j++) {
            if (shortest_path_table[i][j] == NONE) {
                break;
            }

            // Append the next node
            char next[5] = {0};
            sprintf(next, "%d-", shortest_path_table[i][j]);
            strcat(msg, next);
        }

        msg[strlen(msg) - 1] = '\0';  // Remove the last '-'
        tcp_send_msg(&dest->tcp, msg);
    }
}

/*
 * Updates the shortest paths
 *
 * @param dest: the destination node
 * @param src: the source node
 */
void update_forwarding_table(int dest, int src, char *path) {
    int i;

    if (dest == master_node.self.id) {
        return;
    }

    memset(forwarding_table[dest][src], NONE, sizeof(forwarding_table[dest][src]));
    forwarding_table[dest][src][0] = master_node.self.id;

    char *token = strtok(path, "-");
    for (i = 1; token != NULL; i++) {
        forwarding_table[dest][src][i] = atoi(token);
        token = strtok(NULL, "-");
    }
    forwarding_table[dest][src][i] = NONE;

    // update_shortest_paths();
}
