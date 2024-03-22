#include "forwarding_tables.h"

extern MasterNode master_node;

static int forwarding_table[100][100][MAX_NUMBER_OF_NODES];
static int shortest_path_table[100][MAX_NUMBER_OF_NODES];

int *get_last_element(int *arr) {
    for (int *i = arr; i < arr + 100; i++) {
        if (*i == NONE) {
            return i;
        }
    }
    return NULL;
}

void tables_init() {
    memset(forwarding_table, NONE, sizeof(forwarding_table));
    memset(shortest_path_table, NONE, sizeof(shortest_path_table));

    shortest_path_table[master_node.self.id][0] = master_node.self.id;
}

void forwarding_table_reset_entry(int dest, int src) {
    memset(forwarding_table[dest][src], NONE, sizeof(forwarding_table[dest][src]));
}

void shortest_path_table_reset_entry(int dest) {
    memset(shortest_path_table[dest], NONE, sizeof(shortest_path_table[dest]));
}

int ***get_forwarding_table() { return (int ***)forwarding_table; }
int **get_shortest_path_table() { return (int **)shortest_path_table; }

int *get_forwarding(int dest, int src) { return forwarding_table[dest][src]; }
int *get_shortest_path(int dest) { return shortest_path_table[dest]; }
int get_first_step(int dest) { return shortest_path_table[dest][1]; }

void forwarding_table_push(int dest, int src, int id) {
    int *last = get_last_element(forwarding_table[dest][src]);
    if (last == NULL) {
        ERROR("Forwarding table is full\n");
        exit(1);
    }

    // If the path is empty, add the self to the start of the path
    if (last == forwarding_table[dest][src]) {
        forwarding_table[dest][src][0] = master_node.self.id;
        forwarding_table[dest][src][1] = id;
        return;
    }

    *last = id;
}

void shortest_path_table_push(int dest, int id) {
    int *last = get_last_element(shortest_path_table[dest]);

    if (last == NULL) {
        ERROR("Shortest path table is full\n");
        exit(1);
    }

    // If the path is empty, add the self to the start of the path
    if (last == shortest_path_table[dest]) {
        shortest_path_table[dest][0] = master_node.self.id;
        shortest_path_table[dest][1] = id;
        return;
    }

    *last = id;
}

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

void update_shotest_path(int dest, char *path) {
    int new_size = 1;
    int current_size =
        (get_last_element(shortest_path_table[dest]) - shortest_path_table[dest]) / sizeof(int);

    int new_route[MAX_NUMBER_OF_NODES] = {0};
    memset(new_route, NONE, sizeof(new_route));
    new_route[0] = master_node.self.id;

    // Parse the path
    char *token = strtok(path, "-");
    while (token != NULL) {
        int id = atoi(token);
        new_route[new_size++] = id;

        token = strtok(NULL, "-");
    }

    if (new_size < current_size && new_size != 1) {
        memcpy(shortest_path_table[dest], new_route, sizeof(new_route));
    }
}
