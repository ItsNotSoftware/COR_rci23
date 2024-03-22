#ifndef FORWARDING_TABLES_H
#define FORWARDING_TABLES_H

#include "headers.h"
#include "node.h"

#define NONE -1

void tables_init();

int ***get_forwarding_table();
int **get_shortest_path_table();
int *get_forwarding(int dest, int src);
int *get_shortest_path(int dest);
int get_first_step(int dest);

void forwarding_table_push(int dest, int src, int id);
void shortest_path_table_push(int dest, int id);
void update_shotest_path(int dest, char *path);
void update_forwarding_table(int dest, int src, char *path);
void forwarding_table_add_direct_conn(int dest, int src);

void send_shortest_path_table(Node *dest);

#endif